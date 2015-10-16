//
//  pallogon.c
//  oxyale
//
//  Created by Jesse Riddle on 7/21/15.
//  Copyright (c) 2015 Riddle Software. All rights reserved.
//

#include <palclient.h>
#include <pallogon.h>

void OXLInitPalLogonMsg(OXLPalLogonMsg *logonMsg,
                        const char *username,
                        const char *wizpass,
                        const uint32_t initialRoomId,
                        const uint32_t regCRC,
                        const uint32_t regCounter,
                        const uint16_t puidCRC,
                        const uint32_t puidCounter)
{
    /* oxl_net_logon_t *logon = (oxl_net_logon_t *)buf->base; */
    OXLLog("OXLInitPalLogonMsg");
    
    OXLPalLogonMsg logonMsgInit = {
        .palMsg.id = PAL_TX_LOGON_MSG,
        .palMsg.len = 0x80,
        .palMsg.ref = 0,
        .regCRC = regCRC,
        .regCounter = regCounter,
        .usernameLen = (uint8_t)strnlen(username, PAL_USERNAME_SZ_CAP),
        .flags = PAL_AUXFLAGS_AUTHENTICATE | PAL_AUXFLAGS_WIN32,
        .puidCounter = puidCounter,
        .puidCRC = puidCRC,
        .demoElapsed = 0,
        .totalElapsed = 0,
        .demoLimit = 0,
        .initialRoomId = initialRoomId,
        .ulUploadRequestedProtocolVersion = 0,
        .ulUploadCapabilities = PAL_ULCAPS_ASSETS_PALACE,
        .ulDownloadCapabilities = PAL_DLCAPS_ASSETS_PALACE |                                        PAL_DLCAPS_FILES_PALACE | PAL_DLCAPS_FILES_HTTPSRVR,
        .ul2DEngineCapabilities = 0,
        .ul2DGraphicsCapabilities = 0,
        .ul3DEngineCapabilities = 0
    };

    strncpy((char *)&logonMsgInit.username, username, PAL_USERNAME_SZ_CAP); /* PString */
    strlcpy((char *)&logonMsgInit.wizpass, wizpass, PAL_WIZ_PASS_SZ_CAP); /* CString */
    
    /* does nothing, but is logged by server */
    /* strncpy(logonCmd->reserved, "OXYALE", 6); */
    /* strncpy(logonCmd->reserved, "PC4232", 6); */
    strncpy((char *)&logonMsgInit.reserved, "OPNPAL", 6);

    memcpy(logonMsg, &logonMsgInit, sizeof(*logonMsg));
}

OXLPalLogonMsg *OXLCreatePalLogonMsg(const char *username,
                                     const char *wizpass,
                                     const uint32_t initialRoomId,
                                     const uint32_t regCRC,
                                     const uint32_t regCounter,
                                     const uint16_t puidCRC,
                                     const uint32_t puidCounter)
{
    OXLLog("OXLCreatePalLogonMsg");
    OXLPalLogonMsg *logonMsg = OXLAlloc(sizeof(*logonMsg));
    OXLInitPalLogonMsg(logonMsg,
                       username,
                       wizpass,
                       initialRoomId,
                       regCRC,
                       regCounter,
                       puidCRC,
                       puidCounter);
    return logonMsg;
}

void OXLDumpPalLogonMsg(const OXLPalLogonMsg *logonMsg)
{
    OXLLog("OXLDumpPalLogonMsg");
    /* oxl_net_logon_t *logon = (oxl_net_logon_t *)logon_buf->base; */
    /* raw */
    OXLLog("logonMsg->palMsg.id = 0x%x", logonMsg->palMsg.id);
    OXLLog("logonMsg->palMsg.len = %d", logonMsg->palMsg.len);
    OXLLog("logonMsg->palMsg.ref = 0x%x", logonMsg->palMsg.ref);
    OXLLog("logonMsg->regCRC = 0x%x", logonMsg->regCRC);
    OXLLog("logonMsg->regCounter = 0x%x", logonMsg->regCounter);
    OXLLog("logonMsg->usernameLen = %d", logonMsg->usernameLen);
    OXLLog("logonMsg->username = %s", logonMsg->username);
    OXLLog("logonMsg->flags = 0x%x", logonMsg->flags);
    OXLLog("logonMsg->puidCounter = 0x%x", logonMsg->puidCounter);
    OXLLog("logonMsg->puidCRC = 0x%x", logonMsg->puidCRC);
    OXLLog("logonMsg->demoElapsed = 0x%x", logonMsg->demoElapsed);
    OXLLog("logonMsg->totalElapsed = 0x%x", logonMsg->totalElapsed);
    OXLLog("logonMsg->demoLimit = 0x%x", logonMsg->demoLimit);
    
    /* initial room */
    OXLLog("logonMsg->roomId = %d", logonMsg->initialRoomId);
    
    /* does nothing, but is logged by server */
    OXLLog("logonMsg->reserved = %s", logonMsg->reserved);
    
    /* ignored on server */
    OXLLog("logonMsg->ulUploadRequestedProtocolVersion = 0x%x",
           logonMsg->ulUploadRequestedProtocolVersion);
    
    /* TODO upload capabilities plox */
    OXLLog("logonMsg->ulUploadCapabilities = 0x%x", logonMsg->ulUploadCapabilities);
    
    /* TODO download capabilities plox */
    OXLLog("logonMsg->ulDownloadCapabilities = 0x%x", logonMsg->ulDownloadCapabilities);
    
    /* unused */
    OXLLog("logonMsg->ul2DEngineCapabilities = 0x%x", logonMsg->ul2DEngineCapabilities);
    
    /* unused */
    OXLLog("logonMsg->ul2DGraphicsCapabilities = 0x%x", logonMsg->ul2DGraphicsCapabilities);
    
    /* unused */
    OXLLog("logonMsg->ul3DEngineCapabilities = 0x%x", logonMsg->ul3DEngineCapabilities);
}

void OXLDestroyPalLogonMsg(OXLPalLogonMsg *logonMsg)
{
    OXLFree(logonMsg);
}