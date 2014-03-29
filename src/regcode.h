#pragma once
#ifndef _OPAL_REGCODE_H_
#define _OPAL_REGCODE_H_

/* public interface */

typedef struct {
    int32_t counter;
    int32_t crc;
} opalRegcode_t;

extern opalRegcode_t *OPAL_MakeRegcode( void );
extern opalRegcode_t *OPAL_MakeRegcodeFromString( const char *s );
extern void OPAL_PrintRegcode( const opalRegcode_t *regcode );
extern void OPAL_FreeRegcode( opalRegcode_t *regcode );

#endif /* _OPAL_REGCODE_H_ */
