#ifndef darksidesync_aux_h
#define darksidesync_aux_h

#include "darksidesync_api.h"


// static pointer to itself, uniquely identifies this library
static void* DSS_LibID = &DSS_LibID;
// Static pointer to the DSS api, will be set by the initialize function below
static pDSS_api_1v0_t DSSapi = NULL;	// Static no issue, because version 1v0 is same in all libraries



void DSS_initialize(lua_State *L, DSS_cancel_1v0_t pCancel);
void* DSS_getutilid(lua_State *L);
int DSS_deliver(void* utilid, DSS_decoder_1v0_t pDecode, DSS_return_1v0_t pReturn, void* pData);
void DSS_shutdown(lua_State *L, void* utilid);

#endif  /* darksidesync_aux_h */