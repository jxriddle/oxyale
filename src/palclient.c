//
//  palclient.c
//  oxyale
//
//  Created by Jesse Riddle on 7/18/15.
//  Copyright (c) 2015 Riddle Software. All rights reserved.
//

#include <stdlib.h>
#include <assert.h>
#include <stdio.h>

#include <libuv/uv.h>
#include <oxlcom.h>
#include <palcom.h>
#include <palclient.h>
#include <pallogon.h>
#include <palserverinfo.h>
#include <palgotoroom.h>
#include <palsaybubble.h>
#include <palsay.h>
#include <paljoinroom.h>

/*
#include <stdlib.h>
#include <assert.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#endif

#include <uv.h>
#include <oxyale.h>
#include <oxyale-palclient.h>
*/

/**************************************************************
 * Private ****************************************************
 **************************************************************/

/* uv_write_cb */
void
OXLPalClientAfterSendAndDestroyMsg_(uv_write_t *write, const int32_t status)
{
    OXLWriteBaton *owb = (OXLWriteBaton *)write->data;
    if (owb->ocb.callback) {
        owb->ocb.callback(owb->ocb.sender, owb->ocb.data, status);
    }
    OXLDestroyWriteBaton(owb);
}

/* uv_write_cb */
void
OXLPalClientAfterSendAndRetainMsg_(uv_write_t *write, const int32_t status)
{
    OXLWriteBaton *owb = (OXLWriteBaton *)write->data;
    if (owb->ocb.callback) {
        owb->ocb.callback(owb->ocb.sender, owb->ocb.data, status);
    }
    OXLFree(write);
}

static OXLWriteBaton *
OXLPalClientSendMsgInit__(OXLPalClient *client, const void *msg, const OXLCallback callback)
{
    const OXLPalMsg *palMsg = msg;
    
    OXLDumpPalMsg(palMsg);
    
    uint32_t msgSize = sizeof(OXLPalMsg) + palMsg->len;
    OXLWriteBaton *owb = OXLCreateWriteBaton(client, uv_buf_init((char *)msg, msgSize), callback);
    
    owb->write.data = owb;
    /*
     wb->cb.sender = client;
     wb->cb.data = msg;
     wb->cb.callback = callback;
     */
    
    /* Reset ping timer */
    uv_timer_again(client->txTimer);
    
    return owb;
}

static void
OXLPalClientSendAndDestroyMsg__(OXLPalClient *client, void *msg, const OXLCallback callback)
{
    OXLWriteBaton *owb = OXLPalClientSendMsgInit__(client, msg, callback);
    uv_write(&owb->write, (uv_stream_t *)client->socket, &owb->buf, 1, OXLPalClientAfterSendAndDestroyMsg_);
}

static void
OXLPalClientSendAndRetainMsg__(OXLPalClient *client, const void *msg, const OXLCallback callback)
{
    OXLWriteBaton *owb = OXLPalClientSendMsgInit__(client, msg, callback);
    uv_write(&owb->write, (uv_stream_t *)client->socket, &owb->buf, 1, OXLPalClientAfterSendAndRetainMsg_);
}

static void
OXLPalClientStopRead__(OXLPalClient *client)
{
    OXLLogDebug("Stopping socket read.");
    uv_read_stop((uv_stream_t *)client->socket);
}

/*
 * status codes:
 * 0<: logon success with warning
 * 0: logon success
 * <0: logon fail with error
 */
void
OXLPalClientLogonEvent_(void *sender, const void *data, const int32_t status)
{
    OXLLogDebug("OXLPalClientLogonEvent");
    OXLPalClient *client = (OXLPalClient *)sender;
    OXLWriteBaton *wb = (OXLWriteBaton *)data;
    
    if (status < 0) {
        OXLLogDebug("ERROR. Logon write failure. STATE_DISCONNECTED is now true.");
        client->state = STATE_DISCONNECTED;
        /* uv_close((uv_handle_t *)client->conn, NULL); */
    } else {
        if (0 < status) {
            OXLLogDebug("Warnings occurred during logon.");
        }
        OXLLogDebug("Logon write successful. STATE_CONNECTED is now true.");
        client->state = STATE_CONNECTED;
    }
    
    if (NULL != client->event.AfterPalLogonEvent) {
        client->event.AfterPalLogonEvent(client, wb->buf.base, status);
    }
}

static void
OXLPalClientLogon__(OXLPalClient *client, const uint32_t size, const uint16_t refId)
{
    OXLLogDebug("OXLPalaceLogon(client: 0x%x, size: 0x%x, refId: 0x%x)", (uint32_t)client, size, refId);
    client->currentRoom.userId = refId;
    
    OXLPalLogonMsg *logonMsg = OXLCreatePalLogonMsg(client->username,
                                                    client->wizpass,
                                                    client->desiredRoomId,
                                                    client->regCRC,
                                                    client->regCounter,
                                                    client->puidCRC,
                                                    client->puidCounter);
    
    OXLDumpPalLogonMsg(logonMsg);
    OXLDumpRawBufWithSize((byte *)logonMsg, sizeof(*logonMsg));
    
    OXLPalClientSendAndDestroyMsg__(client, logonMsg, OXLPalClientLogonEvent_);
}

static void
OXLPalClientAlternateLogon__(OXLPalClient *client, const uint32_t size, uint16_t refId)
{
    /* OXLPalAltLogonMsg *logonMsg = OXLCreatePalLogonMsg(); */
}

/* uv_timer_cb */
void
OXLPalClientTimerPingServer_(uv_timer_t *timer)
{
    OXLPalClient *client = (OXLPalClient *)timer->data;
    
    if (client->state != STATE_CONNECTED) {
        return;
    }
    
    OXLLogDebug("Pinging server.");
    
    /* We should ping the server */
    /* OXLPalClientSendAndRetainMsg(client, &kPalPingMsg, NULL); */
    OXLPalClientSendAndDestroyMsg__(client,
                                   OXLCreatePalMsg(PAL_TX_PING_MSG, client->user.id),
                                   client->event.AfterPalPingEvent);
}

static void
OXLPalClientStartTxTimer__(OXLPalClient *client)
{
    OXLLogDebug("Starting ping timer.");
    uv_timer_start(client->txTimer,
                   OXLPalClientTimerPingServer_,
                   1000 * client->idlePingTimerStartDelayInSec,
                   1000 * client->idlePingTimerIntervalInSec);
}

static void
OXLPalClientStopTxTimer__(OXLPalClient *client)
{
    OXLLogDebug("Stopping ping timer.");
    uv_timer_stop(client->txTimer);
}

static void
OXLInitPalClient__(OXLPalClient *client, uv_loop_t *uv_loop)
{
    /* stop playing audio */
    
    client->loop = uv_loop;
    
    client->isNeedingToRunSignonHandlers = TRUE;
    client->isWaitingForMore = FALSE;
    client->state = STATE_DISCONNECTED;
    
    /* strlcpy(client->currentRoom.name, "", PAL_ROOM_NAME_SZ_CAP); */
    /* OXLClearList(&client->currentRoom.users); */
    OXLListRemoveAll(&client->userList);
    OXLListRemoveAll(&client->roomList);
    
    client->puidChangedFlag = 0;
    client->puidCounter = 0xf5dc385e;
    client->puidCRC = 0xc144c580;
    client->regCounter = 0xcf07309c;
    client->regCRC = 0x5905f923;
    
    uv_timer_init(client->loop, client->txTimer);
    OXLTimerBaton *tb = (OXLTimerBaton *)client->txTimer;
    tb->timer.data = client;
    tb->data = &tb->timer;
    
    client->usernameLen = 0;
    memset(&client->username, '\0', PAL_USERNAME_SZ_CAP);
    client->wizpassLen = 0;
    memset(&client->wizpass, '\0', PAL_WIZPASS_SZ_CAP);
    
    client->event.AfterPalAllUsersEvent = NULL;
    client->event.AfterPalAuthRequestEvent = NULL;
    client->event.AfterPalConnectEvent = NULL;
    client->event.AfterPalDisconnectEvent = NULL;
    client->event.AfterPalGotoURLEvent = NULL;
    client->event.AfterPalLogonEvent = NULL;
    client->event.AfterPalPingEvent = NULL;
    client->event.AfterPalPongEvent = NULL;
    client->event.AfterPalRoomChangeEvent = NULL;
    client->event.AfterPalRoomListEvent = NULL;
    client->event.AfterPalSecurityEvent = NULL;
    client->event.AfterPalServerInfoEvent = NULL;
    client->event.AfterPalXTalkEvent = NULL;
}

void
OXLInitPalClient(OXLPalClient *client, uv_loop_t *uv_loop)
{
    OXLInitPalClient__(client, uv_loop);
}

static void
OXLSetupPalClientWithLoop__(OXLPalClient *client, uv_loop_t *uv_loop)
{
    client->txTimer = (uv_timer_t *)OXLCreateTimerBaton(client, NULL, NULL);
    
    OXLInitList(&client->roomList);
    OXLInitList(&client->userList);
    
    OXLInitPalClient__(client, uv_loop);
}

void
OXLSetupPalClientWithLoop(OXLPalClient *client, uv_loop_t *uv_loop)
{
    OXLSetupPalClientWithLoop__(client, uv_loop);
}

void
OXLSetupPalClient(OXLPalClient *client)
{
    OXLSetupPalClientWithLoop__(client, uv_default_loop());
}

static void
OXLUnsetupPalClient__(OXLPalClient *client)
{
    OXLFree(client->txTimer);
}

void
OXLUnsetupPalClient(OXLPalClient *client)
{
    OXLUnsetupPalClient__(client);
}

/* void uv_close_cb */
/* OXLPalClientAfterDisconnect_(uv_handle_t *handle) */

void /* OXLCallback */
OXLPalClientDisconnectEvent_(void *sender, const void *data, const int32_t status)
{
    OXLPalClient *client = (OXLPalClient *)sender;
    if (status < 0) {
        OXLLogError("Error %d while disconnecting from Palace", status);
    } else {
        if (0 < status) {
            /* completed with warnings */
        }
        OXLLogInfo("Disconnected from Palace with status %d", status);
    }
    
    client->state = STATE_DISCONNECTED;
}

static void
OXLPalClientDisconnect__(OXLPalClient *client)
{
    /* OXLLogDebug("Disconnecting from Palace %s", client->servername); */
    OXLLogDebug("Disconnecting from Palace %s", client->hostname);
    /* OXLPalClient *client = handle->data; */
    /* oxl_client_t *client = (oxl_client_t *)handle->data; */
    
    /* Stop ping timer */
    OXLPalClientStopTxTimer__(client);
    
    /* Stop reading from socket */
    OXLPalClientStopRead__(client);
    
    client->socket->data = client;
    OXLCloseSocket(client, client->socket, OXLPalClientDisconnectEvent_);
}

/**************************************************************
 * Private Event Handlers *************************************
 **************************************************************/

static void
OXLPalClientHandleAllUsers__(OXLPalClient *client, const uint32_t msgLen, const uint16_t msgRef, const uv_buf_t *buf)
{
    if (NULL != client->event.AfterPalAllUsersEvent) {
        client->event.AfterPalAllUsersEvent(client, buf->base, msgLen);
    }
}

static void
OXLPalClientHandleAltLogon__(OXLPalClient *client, const uint32_t msgLen, const uint16_t msgRef, const uv_buf_t *buf)
{
    OXLPalLogonMsg *altLogonMsg = (OXLPalLogonMsg *)buf->base;
    if (altLogonMsg->puidCounter != client->puidCounter ||
        altLogonMsg->puidCRC != client->puidCRC) {
        
        OXLLogDebug("PUID changed by server.");
        client->puidCRC = altLogonMsg->puidCRC;
        client->puidCounter = altLogonMsg->puidCounter;
        client->puidChangedFlag = 1;
    }
    
    /* emit alt_logon received event signal */
    /* client->event.PalEventAltLogon(client); */
}

/*
 static void
 OXLPalClientHandleAuthenticate_(OXLPalClient *client, OXLCallback callback)
 {
 callback(client, NULL, 1);
 }
 */

static void
OXLPalClientHandleTerminate__(OXLPalClient *client, const uint32_t msgLen, const uint16_t msgRef)
{
    OXLPalClientDisconnect__(client);
    
    OXLLogDebug("HandleTerminate");
    switch (msgRef) {
        case K_KilledByPlayer: /* 0x04 */
        case K_KilledBySysop: /* 0x07 */
            OXLLogDebug("You have been killed.");
            break;
        case K_BanishKill: /* 0x0d */
            OXLLogDebug("You have been kicked off the site.");
            break;
        case K_DeathPenaltyActive: /* 0x0b */
            OXLLogDebug("Your death penalty is still active.");
            break;
        case K_Banished: /* 0x0c */
            OXLLogDebug("You are not currently allowed on this site.");
            break;
        case K_Unresponsive: /* 0x06 */
            OXLLogDebug("Your connection was terminated due to inactivity");
            break;
        case K_Flood: /* 0x03 */
            OXLLogDebug("Your connection was terminated due to flooding.");
            break;
        case K_ServerFull: /* 0x08 */
            OXLLogDebug("This palace is currently full. Please try again later.");
            break;
        case K_NoGuests: /* 0x0e */
            OXLLogDebug("Guests are not currently allowed on this site.");
            break;
        case K_ServerDown: /* 0x05 */
            OXLLogDebug("This palace was shut down by its operator. Try again later.");
            break;
        case K_InvalidSerialNumber: /* 0x09 */
            OXLLogDebug("You have an invalid serial number.");
            break;
        case K_DuplicateUser: /* 0x0a */
            OXLLogDebug("There is another user using your serial number.");
            break;
        case K_DemoExpired: /* 0x0f */
            OXLLogDebug("Your free demo has expired.");
            break;
        case K_Unknown: /* 0x10 */
            OXLLogDebug("Unknown error?");
            break;
        case K_CommError: /* 0x02 */
            OXLLogDebug("There has been a communications error.");
            break;
        default:
            OXLLogDebug("ERROR Unknown [PUIDChangedFlag: 0x%x, msgLen: 0x%x, msgRef: 0x%x]",
                        client->puidChangedFlag, msgLen, msgRef);
            break;
    }
    
    if (!client->puidChangedFlag) {
        /* PUID unchanged. */
        OXLLogDebug("Connection dropped.");
    }
    
    /* emit connection error received event signal */
    if (NULL != client->event.AfterPalDisconnectEvent) {
        client->event.AfterPalDisconnectEvent(client, NULL, -1);
    }
}

static void
OXLPalClientHandlePong__(OXLPalClient *client, const uint32_t msgLen, const uint16_t msgRef)
{
    if (NULL != client->event.AfterPalPongEvent) {
        client->event.AfterPalPongEvent(client, NULL, 0);
    }
}

static void
OXLPalClientHandleRoomList__(OXLPalClient *client, const uint32_t msgLen, const uint16_t msgRef, const uv_buf_t *buf)
{
    int32_t i;
    OXLList *roomList;
    OXLPalRoom *room = NULL; /* data; */
    
    OXLListRemoveAll(&client->roomList);
    for (i = 0; i < msgLen; i++) {
        room = NULL; /* next room */
        
        OXLListAdd(&client->roomList, OXLCreateListItem(room));
    }
    
    if (NULL != client->event.AfterPalRoomListEvent) {
        client->event.AfterPalRoomListEvent(client, buf->base, msgLen);
    }
}

static void
OXLPalClientHandleServerInfo__(OXLPalClient *client, const uint32_t msgLen, const uint16_t msgRef, const uv_buf_t *buf)
{
    OXLPalServerInfoMsg *serverInfo = (OXLPalServerInfoMsg *)buf;
    client->permissions = serverInfo->permissions;
    strlcpy(client->servername, serverInfo->servername, PAL_SERVER_NAME_SZ_CAP);
    
    /* TODO emit handle server info received event signal  */
    if (NULL != client->event.AfterPalServerInfoEvent) {
        client->event.AfterPalServerInfoEvent(client, NULL, 0);
    }
}

static void
OXLPalClientHandleServerVersion__(OXLPalClient *client, const uint32_t msgLen, const uint16_t msgRef, const uv_buf_t *buf)
{
    OXLLogDebug("HandleServerVersion");
    
    client->serverVersion = msgRef;
    OXLLogDebug("Palace server ver %u", client->serverVersion);
    
    /* emit palace server version received event signal */
    if (NULL != client->event.AfterPalServerVerEvent) {
        client->event.AfterPalServerVerEvent(client, buf, 0);
    }
    
}

static void
OXLPalClientHandleRecvUserStatus__(OXLPalClient *client, const uint32_t msgLen, const uint16_t msgRef, const uv_buf_t *buf)
{
    OXLPalUser *user = (OXLPalUser *)buf->base;
    
    /* if (-1 < client->user.id) { */
    if (client->user.id) {
        client->user.flags = user->flags;
    }
    /* TODO emit handle user status received event signal  */
}

static void
OXLPalClientHandleMaxUsersLoggedOn__(OXLPalClient *client, const uint32_t msgLen, const uint16_t msgRef, const uv_buf_t *buf)
{
    /* TODO emit user logged on and max received event signal */
}

static void
OXLPalClientHandleXTalk__(OXLPalClient *client, const uint32_t msgLen, const uint16_t msgRef, const uv_buf_t *buf)
{
    char *cipherText = buf->base;
    char *plainText = OXLAlloc(strnlen(buf->base, OXL_MAX_STR_SZ_CAP)); /* buf->len? */
    OXLPalCryptoDecrypt(client->crypto, cipherText, plainText);
    OXLLogDebug("plainText: %s", plainText);
    
    if (NULL != client->event.AfterPalXTalkEvent) {
        client->event.AfterPalXTalkEvent(client, plainText, 0);
    }
    
    OXLFree(plainText);
}

/**************************************************************
 * End Private Event Handlers *********************************
 **************************************************************/

static void
OXLPalClientHandshake__(OXLPalClient *client, const ssize_t nread, const uv_buf_t buf)
{
    OXLLogDebug("Handshake");
    int32_t status;
    
    if (0 < nread) {
        OXLLogDebug("Successfully read %ld bytes of data.", nread);
        OXLPalMsg *msg = (OXLPalMsg *)buf.base;
        uint32_t msgId = msg->id;
        /* OXLDumpPalMsg(msg); */
        
        /* endian test follows in switch statement */
        /* *(int *)buf->base; */
        uint32_t msgLen;
        uint16_t msgRef;
        
        switch (msgId) {
            case PAL_RX_UNKNOWN_SERVER:
                OXLLogDebug("Received MSG_TROPSER during handshake. Execution stops here.");
                status = -2; /* OXLPAL_ERR_TROPSER */
                break;
            case PAL_RX_LITTLE_ENDIAN_SERVER: /* MSG_DIYIT */
                OXLLogDebug("Server is little endian.");
                /* TODO will a server EVER send little endian data? */
                /* Big endian is standard for network communication. */
                client->isServerBigEndian = FALSE;
                /* TODO need to accomodate for endianness? */
                /* i.e. little endian server, big endian client. */
                msgLen = msg->len;
                msgRef = msg->ref;
                OXLPalClientLogon__(client, msgLen, msgRef);
                status = 0;
                break;
            case PAL_RX_BIG_ENDIAN_SERVER: /* MSG_TIYID */
                OXLLogDebug("Server is big endian.");
                client->isServerBigEndian = TRUE;
                msgLen = msg->len; /* ntohl(header->msgLen); */
                msgRef = msg->ref; /* ntohl(header->msgRef); */
                OXLPalClientLogon__(client, msgLen, msgRef);
                status = 0;
                break;
            default:
                OXLLogDebug("Unexpected error while logging in.");
                /* OXLDumpBuf(buf); */
                /* OXLPalClientDisconnect_(client); */
                status = -1; /* OXLPAL_ERR_GENERAL */
                break;
        } /* end switch */
        /* msgID = req->data */
    } else {
        /* TODO possible error? no bytes read. */
        status = -1; /* OXLPAL_ERR_GENERAL */
    }
    
    if (NULL != client->event.AfterPalHandshakeEvent) {
        client->event.AfterPalHandshakeEvent(client, buf.base, status);
    }
}

void /* uv_read_cb */
OXLPalClientReadEvent_(uv_stream_t *stream, const ssize_t nread, const uv_buf_t *buf)
{
    OXLPalMsg *msg = (OXLPalMsg *)buf->base;
    OXLPalClient *client = (OXLPalClient *)stream->data;
    
    OXLLogDebug("OXLPalClientReadEvent");
    if (nread == 0) {
        OXLLogDebug("Read ZERO bytes from server.");
    } else if (nread < 0) { /* error */
        OXLLogDebug("Read %s bytes. ERROR reading data, terminating connection.", nread);
        /* uv_close((uv_handle_t *)stream, NULL); */
        OXLPalClientDisconnect__(client);
    } else if (0 < nread) { /* else ? */
        /* we need to collect payloads then route the message (on a worker thread?) */
        /* we need to do this depending on the state of the connection */
        OXLLogDebug("Successfully read %ld bytes of data", nread);
        
        OXLLogDebug("Read the following:");
        OXLDumpBufWithSize(*buf, nread);
        
        if (client->state == STATE_HANDSHAKING) {
            OXLLogDebug("STATE_HANDSHAKING is true. Performing handshake.");
            OXLPalClientHandshake__(client, nread, *buf);
        } else if (client->state == STATE_CONNECTED) {
            /* fprintf(stderr,
             "--- DEBUG: reading data with STATE_CONNECTED true.\n"); */
            
            if (client->msg.id == 0) {
                /* fprintf(stderr, "--- DEBUG: client->msgId == 0\n"); */
                if (PAL_NET_HEADER_LEN <= nread) {
                    /* *((int *)buf->base)); */
                    client->msg.id = msg->id;
                    /* *((int *)(buf->base + sizeof client->msgId))); */
                    client->msg.len = msg->len;
                    /* *((int *)(buf->base + */
                    /* sizeof client->msgId + sizeof client->msgLen))); */
                    client->msg.ref = msg->ref;
                    /* oxl_header_t *header = malloc(nread);
                     memcpy(header, buf->base, nread);
                     free(header); */ /* do something with it instead */
                } else {
                    return; /* TODO ? */
                }
            }
            
            if (nread < msg->len) {
                return; /* TODO ? */
            }
            
            int32_t msgId = msg->id; /* resolve indirection only once */
            int32_t msgLen = msg->len;
            int32_t msgRef = msg->ref;
            
            switch (msgId) {
                case PAL_RX_ALTERNATE_LOGON_REPLY:
                    OXLLogDebug("Read message ALTERNATE_LOGON_REPLY.");
                    OXLPalClientHandleAltLogon__(client, msgLen, msgRef, buf);
                    break;
                case PAL_RX_CONNECTION_ERROR:
                    OXLLogDebug("Read message CONNECTION_ERROR.");
                    OXLPalClientHandleTerminate__(client, msgLen, msgRef);
                    break;
                case PAL_RX_SERVER_VERSION:
                    OXLLogDebug("Read message SERVER_VERSION.");
                    OXLPalClientHandleServerVersion__(client, msgLen, msgRef, buf);
                    break;
                case PAL_RX_SERVER_INFO:
                    OXLLogDebug("DEBUG: Read message SERVER_INFO.");
                    OXLPalClientHandleServerInfo__(client, msgLen, msgRef, buf);
                    break;
                case PAL_RX_USER_STATUS:
                    OXLLogDebug("Read message USER_STATUS.");
                    OXLPalClientHandleRecvUserStatus__(client, msgLen, msgRef, buf);
                    break;
                case PAL_RX_USER_LOGGED_ON_AND_MAX:
                    OXLLogDebug("Read message USER_LOGGED_ON_AND_MAX.");
                    OXLPalClientHandleMaxUsersLoggedOn__(client, msgLen, msgRef, buf);
                    break;
                case PAL_RX_GOT_HTTP_SERVER_LOCATION:
                    OXLLogDebug("Read message GOT_HTTP_SERVER_LOCATION.");
                    break;
                case PAL_RX_GOT_ROOM_DESCRIPTION:
                    OXLLogDebug("Read message GOT_ROOM_DESCRIPTION.");
                    break;
                case PAL_RX_GOT_ROOM_DESCRIPTION_ALT:
                    OXLLogDebug("Read message GOT_ROOM_DESCRIPTION_ALT.");
                    break;
                case PAL_RX_GOT_USER_LIST:
                    OXLLogDebug("Read message GOT_USER_LIST.");
                    break;
                case PAL_RX_GOT_REPLY_OF_ALL_USERS:
                    OXLLogDebug("Read message GOT_REPLY_OF_ALL_USERS.");
                    OXLPalClientHandleAllUsers__(client, msgLen, msgRef, buf);
                    break;
                case PAL_RX_GOT_ROOM_LIST:
                    OXLLogDebug("Read message GOT_ROOM_LIST.");
                    OXLPalClientHandleRoomList__(client, msgLen, msgRef, buf);
                    break;
                case PAL_RX_ROOM_DESCEND:
                    OXLLogDebug("Read message ROOM_DESCEND.");
                    break;
                case PAL_RX_USER_NEW:
                    OXLLogDebug("Read message USER_NEW.");
                    break;
                case PAL_RX_PONG_MSG:
                    OXLLogDebug("Read message PONG.");
                    OXLPalClientHandlePong__(client, msgLen, msgRef);
                    break;
                case PAL_RX_XTALK:
                    OXLLogDebug("Read message XTALK.");
                    OXLPalClientHandleXTalk__(client, msgLen, msgRef, buf);
                    break;
                case PAL_RX_XWHISPER:
                    OXLLogDebug("Read message XWHISPER.");
                    break;
                case PAL_RX_TALK:
                    OXLLogDebug("Read message TALK.");
                    break;
                case PAL_RX_WHISPER:
                    OXLLogDebug("Read message WHISPER.");
                    break;
                case PAL_RX_ASSET_INCOMING:
                    OXLLogDebug("Read message ASSET_INCOMING.");
                    break;
                case PAL_RX_ASSET_QUERY:
                    OXLLogDebug("Read message ASSET_QUERY.");
                    break;
                case PAL_RX_MOVEMENT:
                    OXLLogDebug("Read message MOVEMENT.");
                    break;
                case PAL_RX_USER_COLOR:
                    OXLLogDebug("Read message USER_COLOR.");
                    break;
                case PAL_RX_USER_FACE:
                    OXLLogDebug("Read message USER_FACE.");
                    break;
                case PAL_RX_USER_PROP:
                    OXLLogDebug("Read message USER_PROP.");
                    break;
                case PAL_RX_USER_DESCRIPTION:
                    OXLLogDebug("Read message USER_DESCRIPTION.");
                    break;
                case PAL_RX_USER_RENAME:
                    OXLLogDebug("Read message USER_RENAME.");
                    break;
                case PAL_RX_USER_LEAVING:
                    OXLLogDebug("Read message USER_LEAVING.");
                    break;
                case PAL_RX_USER_EXIT_ROOM:
                    OXLLogDebug("USER_EXIT_ROOM.");
                    break;
                case PAL_RX_PROP_MOVE:
                    OXLLogDebug("Read message PROP_MOVE.");
                    break;
                case PAL_RX_PROP_DELETE:
                    OXLLogDebug("DEBUG: Read message PROP_DELETE.");
                    break;
                case PAL_RX_PROP_NEW:
                    OXLLogDebug("Read message PROP_NEW.");
                    break;
                case PAL_RX_DOOR_LOCK:
                    OXLLogDebug("Read message DOOR_LOCK.");
                    break;
                case PAL_RX_DOOR_UNLOCK:
                    OXLLogDebug("Read message DOOR_UNLOCK.");
                    break;
                case PAL_RX_PICT_MOVE:
                    OXLLogDebug("Read message PICT_MOVE.");
                    break;
                case PAL_RX_SPOT_STATE:
                    OXLLogDebug("Read message SPOT_STATE.");
                    break;
                case PAL_RX_SPOT_MOVE:
                    OXLLogDebug("Read message SPOT_MOVE.");
                    break;
                case PAL_RX_DRAW_MSG:
                    OXLLogDebug("Read message DRAW_MSG.");
                    break;
                case PAL_RX_INCOMING_FILE:
                    OXLLogDebug("Read message INCOMING_FILE.");
                    break;
                case PAL_RX_NAV_ERROR:
                    OXLLogDebug("Read message NAV_ERROR.");
                    /* OXLPalClientHandleNavError(client, msgLen, msgRef, buf); */
                    break;
                case PAL_RX_AUTHENTICATE:
                    OXLLogDebug("Read message AUTHENTICATE.");
                    break;
                case PAL_RX_BLOWTHRU:
                    OXLLogDebug("Read message BLOWTHRU.");
                    break;
                default:
                    OXLLogDebug("Received unmatched message: "
                                "[msgId = 0x%x, "
                                "msgLen = 0x%x, "
                                "msgRef = 0x%x]\n",
                                msgId,
                                msgLen,
                                msgRef);
                    break;
            }
            client->msg.id = 0;
        } else if (client->state == STATE_DISCONNECTED) {
            OXLLogDebug("STATE_DISCONNECTED. Ignoring data.");
        }
    }
    
    OXLLogDebug("OXLPalClientReadEvent: Freeing buf->base");
    if (buf->base) {
        OXLFree(buf->base);
    }
}

static void
OXLPalClientStartRead__(OXLPalClient *client)
{
    OXLLogDebug("Starting to read from socket.");
    uv_read_start((uv_stream_t *)client->socket, OXLAllocBuf, OXLPalClientReadEvent_);
}

void /* OXLCallback */
OXLPalClientOpenSocketEvent_(void *sender, const void *data, const int32_t status)
{
    OXLPalClient *client = sender;
    
    if (status < 0) {
        /* error */
    } else {
        if (0 < status) {
            /* warnings generated */
        }
        
        /* socket opened */
        client->socket = (uv_tcp_t *)data;
        client->socket->data = client;
        
        client->state = STATE_HANDSHAKING;
        /* Start ping timer */
        OXLPalClientStartTxTimer__(client);
        OXLPalClientStartRead__(client);
        /* OXLLogDebug("Socket opened to %s", client->hostname); */
    }
    
    if (NULL != client->event.AfterPalOpenSocketEvent) {
        client->event.AfterPalOpenSocketEvent(sender, data, status);
    }
}

static void
OXLPalClientOpenSocket__(OXLPalClient *client, struct addrinfo *ai)
{
    OXLOpenSocket(client, ai, OXLPalClientOpenSocketEvent_);
}

void /* OXLCallback */
OXLPalClientResolveHostnameEvent_(void *sender, const void *data, const int32_t status)
{
    OXLPalClient* client = sender;
    const struct addrinfo *ai = data;
    
    if (status < 0) {
        /* failed to resolve */
    } else {
        if (0 < status) {
            /* completed with warnings */
        }
        OXLPalClientOpenSocket__(client, (struct addrinfo *)ai);
    }
    
    if (NULL != client->event.AfterPalResolveEvent) {
        client->event.AfterPalResolveEvent(client, data, status);
    }
}

void
OXLPalClientConnect_(OXLPalClient *client,
                     const char *username,
                     const char *wizpass,
                     const char *hostname,
                     const uint16_t port,
                     const uint16_t initialRoomId,
                     const uint32_t idlePingTimerStartDelayInSec,
                     const uint32_t idlePingTimerIntervalInSec)
{
    /* size_t usernameLen = strlen(username) + 1; */
    /* size_t wizpassLen = strlen(wizpass) + 1; */
    
    /* client->username = calloc(z, PAL_MAX_USERNAME_LEN); */
    /* sizeof *client->username); */
    client->idlePingTimerStartDelayInSec = idlePingTimerStartDelayInSec;
    client->idlePingTimerIntervalInSec = idlePingTimerIntervalInSec;
    
    strncpy(client->username, username, PAL_USERNAME_SZ_CAP);
    strlcpy(client->wizpass, wizpass, PAL_WIZPASS_SZ_CAP);
    client->desiredRoomId = initialRoomId;
    
    strlcpy(client->hostname, hostname, OXL_MAX_STR_SZ_CAP);
    client->port = port;
    
    char portString[OXL_IP_PORT_SZ_CAP];
    OXLInt2Str(port, portString, OXL_IP_PORT_SZ_CAP);
    /*
    if (port == 9998) {
        OXLLogDebug("Connecting to palace://%s as %s", hostname, username);
    } else {
        OXLLogDebug("Connecting to %s:%n as %s", hostname, portString, username);
    }
     */
    
    /* resolve host */
    OXLResolveHostname(client, hostname, port, OXLPalClientResolveHostnameEvent_);
    
    if (NULL != client->event.AfterPalConnectEvent) {
        client->event.AfterPalConnectEvent(client, NULL, 0);
    }
}

/**************************************************************
 * Private ****************************************************
 **************************************************************/

/* uv_connect_cb */
/*
void
OXLPalClientConnectEvent_(uv_connect_t *connect, const int32_t status)
{
    OXLLogDebug("OXLPalClientAfterConnect");
    OXLPalClient *client = (OXLPalClient *)connect->data;
    
    if (status) { */ /* error condition */ /*
        OXLLogDebug("Error: %d. %s\n", status, uv_strerror(status));
        OXLPalClientDisconnect__(client);
    } else {
        */
        
        /* OXLLogDebug("Connected successfully to %s", client->servername); */
        
        /*
    }
    
    if (NULL != client->event.AfterPalConnectEvent) {
        client->event.AfterPalConnectEvent(client, NULL, status);
    }
}
*/

void /* OXLCallback */
OXLPalAfterLogon_(void *sender, const void *data, const int32_t status)
{
    OXLPalClient *client = (OXLPalClient *)sender;
    OXLPalLogonMsg *logonMsg = (OXLPalLogonMsg *)data;
    
    client->usernameLen = logonMsg->usernameLen;
    strncpy(client->username, logonMsg->username, PAL_USERNAME_SZ_CAP); /* PString */
    strlcpy(client->wizpass, logonMsg->wizpass, PAL_WIZPASS_SZ_CAP); /* CString */
    client->desiredRoomId = logonMsg->initialRoomId;
    
    OXLDestroyPalLogonMsg(logonMsg);
}

static void
OXLClientPalLogon__(OXLPalClient *client,
                    const char *username,
                    const char *wizpass,
                    const int16_t roomId)
{
    OXLPalLogonMsg *logonMsg = OXLCreatePalLogonMsg(username,
                                                    wizpass,
                                                    roomId,
                                                    client->regCRC,
                                                    client->regCounter,
                                                    client->puidCRC,
                                                    client->puidCounter);
    
    OXLLogDebug("Before DumpPalLogonMsg");
    OXLDumpPalLogonMsg(logonMsg);
    OXLLogDebug("After DumpPalLogonMsg");
    
    OXLPalClientSendAndDestroyMsg__(client, logonMsg, OXLPalClientLogonEvent_);
}


/***********************************************
 * Public **************************************
 ***********************************************/

void
OXLPalClientDisconnect(OXLPalClient *client)
{
    OXLPalClientDisconnect__(client);
}

void
OXLPalClientSetParams(OXLPalClient *client,
                      const char *username,
                      const char *wizpass,
                      const char *hostname,
                      const uint16_t port,
                      const uint16_t initialRoomId,
                      const uint32_t idlePingTimerStartDelayInSec,
                      const uint32_t idlePingTimerIntervalInSec)
{
    client->usernameLen = strnlen(username, PAL_USERNAME_SZ_CAP);
    strncpy(client->username, username, PAL_USERNAME_SZ_CAP);
    strlcpy(client->wizpass, wizpass, PAL_WIZPASS_SZ_CAP);
    client->desiredRoomId = initialRoomId;
    
    strlcpy(client->hostname, hostname, OXL_MAX_STR_SZ_CAP);
    client->port = port;
    
    client->idlePingTimerStartDelayInSec = idlePingTimerStartDelayInSec;
    client->idlePingTimerIntervalInSec = idlePingTimerIntervalInSec;
}

OXLPalClient *
OXLCreatePalClientWithLoop(uv_loop_t *uv_loop)
{
    OXLPalClient *client = OXLAlloc(sizeof(*client));
    /*
    client->userList = OXLCreateList();
    client->roomList = OXLCreateList();
     */
    OXLInitList(&client->userList);
    OXLInitList(&client->roomList);
    
    OXLSetupPalClient(client);
    return client;
}

OXLPalClient *
OXLCreatePalClient(void)
{
    return OXLCreatePalClientWithLoop(uv_default_loop());
}

void
OXLPalClientLogon(OXLPalClient *client)
{
    OXLPalClientSendAndDestroyMsg(client,
                                  OXLCreatePalLogonMsg(client->username,
                                                       client->wizpass,
                                                       client->desiredRoomId,
                                                       client->regCRC,
                                                       client->regCounter,
                                                       client->puidCRC,
                                                       client->puidCounter),
                                  client->event.AfterPalLogonEvent);
}

void
OXLDestroyPalClient(OXLPalClient *client)
{
    /* OXLPalCryptoDestroy(&client->crypto); */
    /* uv_loop_close(client->loop); */
    /* free(client->loop); */
    OXLUnsetupPalClient(client);
    
    /*
    OXLDestroyList(client->roomList);
    OXLDestroyList(client->userList);
     */
    OXLListRemoveAll(&client->roomList);
    OXLListRemoveAll(&client->userList);
    
    OXLFree(client);
}

void
OXLSetPalUsername(OXLPalClient *client, const char *username)
{
    strlcpy(client->username, username, PAL_USERNAME_SZ_CAP);
}

void
OXLPalClientSendAndRetainMsg(OXLPalClient *client, const void *msg, const OXLCallback callback)
{
    OXLPalClientSendAndRetainMsg__(client, msg, callback);
}

void
OXLPalClientSendAndDestroyMsg(OXLPalClient *client, void *msg, const OXLCallback callback)
{
    OXLPalClientSendAndDestroyMsg__(client, msg, callback);
}

void
OXLPalClientHandshake(OXLPalClient *client, const ssize_t nread, const uv_buf_t buf)
{
    OXLPalClientHandshake__(client, nread, buf);
}

void
OXLPalClientConnectWithPingTimer(OXLPalClient *client,
                                 const char *username,
                                 const char *wizpass,
                                 const char *hostname,
                                 const uint16_t port,
                                 const uint16_t initialRoomId,
                                 const uint32_t idlePingTimerStartDelayInSec,
                                 const uint32_t idlePingTimerIntervalInSec)
{
    OXLPalClientConnect_(client,
                         username,
                         wizpass,
                         hostname,
                         port,
                         initialRoomId,
                         idlePingTimerStartDelayInSec,
                         idlePingTimerIntervalInSec);
}

void
OXLPalClientConnect(OXLPalClient *client,
                    const char *username,
                    const char *wizpass,
                    const char *hostname,
                    const uint16_t port,
                    const uint16_t initialRoomId)
{
    OXLPalClientConnect_(client,
                         username,
                         wizpass,
                         hostname,
                         port,
                         initialRoomId,
                         100 /* 100 sec delay */,
                         100 /* 100 sec periodic ping */);
}

void
OXLPalClientSay(OXLPalClient *client, const char *plainText)
{
    if (client->state != STATE_CONNECTED || strnlen(plainText, OXL_MAX_STR_SZ_CAP) <= 0) {
        return;
    }
    
    OXLPalClientSendAndDestroyMsg__(client,
                                   OXLCreatePalSayMsg(client, plainText),
                                   client->event.AfterPalXTalkEvent);
}

static void
OXLPalClientJoinRoom_(OXLPalClient *client, const uint16_t gotoRoomId)
{
    uint32_t userId = 0; /* stub */
    OXLPalClientSendAndDestroyMsg__(client, OXLCreatePalGotoRoomMsg(userId, gotoRoomId), client->event.AfterPalJoinRoomEvent);
}

static void
OXLPalClientLeaveRoom_(OXLPalClient *client, const OXLPalRoom *room)
{
    /* Let all iptscrae ON LEAVE event handlers execute */
    
    
    /* emit leave room event signal */
    if (NULL != client->event.AfterPalLeaveRoomEvent) {
        client->event.AfterPalLeaveRoomEvent(client, &client->currentRoom, 0);
    }
}

void
OXLPalClientGotoRoom(OXLPalClient *client, uint16_t gotoRoomId)
{
    if (client->state != STATE_CONNECTED || client->currentRoom.id == gotoRoomId) {
        return;
    }
    
    OXLPalClientLeaveRoom_(client, &client->currentRoom);
    OXLPalClientJoinRoom_(client, gotoRoomId);
}

void
OXLPalClientGetRoomList(OXLPalClient *client)
{
    if (client->state != STATE_CONNECTED) {
        return;
    }
    
    OXLPalClientSendAndDestroyMsg__(client,
                                   OXLCreatePalMsg(PAL_TX_REQUEST_ROOM_LIST_MSG, client->user.id),
                                    client->event.AfterPalRequestRoomListEvent);
    
}

void
OXLPalClientGetUserList(OXLPalClient *client)
{
    if (client->state != STATE_CONNECTED) {
        return;
    }
    
    OXLPalClientSendAndDestroyMsg__(client,
                                   OXLCreatePalMsg(PAL_TX_REQUEST_USER_LIST_MSG, client->user.id),
                                   client->event.AfterPalRequestUserListEvent);
}
