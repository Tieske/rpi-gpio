#include <stdlib.h>
#include <lauxlib.h>
#include <string.h>
#include "darksidesync_api.h"
#include "darksidesync_aux.h"

// will get the api struct (pointer) for the api version 1v0
// in case of errors it will provide a proper error message and call 
// luaL_error. In case of an error the call will not return.
void DSS_initialize(lua_State *L, DSS_cancel_1v0_t pCancel)
{
	int errcode;

	// Collect the table with the global DSS data from the register
	lua_getfield(L, LUA_REGISTRYINDEX, DSS_REGISTRY_NAME);
	if (lua_istable(L, -1) == 0)
	{
		// following call does not return
		luaL_error(L, "No DSS registry table found, make sure to require 'darksidesync' first.");
	}
	// Now get the specified API structure
	lua_getfield(L, -1, DSS_API_1v0_KEY);
	if (lua_islightuserdata (L, -1) == 0)
	{
		// the API struct wasn't found
		char str[150]; // appr 40 chars for version string, should suffice
		lua_getfield(L, -2, DSS_VERSION_KEY);
		if (lua_isstring(L, -1))
		{
			sprintf(str, "Loaded DSS version '%s' does not support API version '%s'. Make sure to use the proper DSS version.", lua_tostring(L, -1), DSS_API_1v0_KEY);
		}
		else
		{
			sprintf(str, "Loaded DSS version is unknown, it does not support API version '%s'. Make sure to use the proper DSS version.", DSS_API_1v0_KEY);
		}
		// following call does not return
		luaL_error(L, str);
	}
	DSSapi = (pDSS_api_1v0_t)lua_touserdata(L, -1);
	lua_pop(L,2); // pop apistruct and DSS global table

	// Now register ourselves
	DSSapi->reg(L, DSS_LibID, pCancel, &errcode);
	if (errcode != DSS_SUCCESS)
	{
		DSSapi = NULL;
		// The error calls below will not return
		switch (errcode) {
			case DSS_ERR_NOT_STARTED: luaL_error(L, "DSS was not started, or already stopped again.");
			case DSS_ERR_NO_CANCEL_PROVIDED: luaL_error(L, "No proper cancel method was provided when initializing DSS.");
			case DSS_ERR_OUT_OF_MEMORY: luaL_error(L, "Memory allocation error while initializing DSS");
			case DSS_ERR_ALREADY_REGISTERED: luaL_error(L, "Library already registered with DSS for this LuaState");
			default: luaL_error(L, "An unknown error occured while initializing DSS.");
		}
	}

	return;
}

// Collect the utilid, or throw Lua error if none
void* DSS_getutilid(lua_State *L)
{
	int err = DSS_SUCCESS;
	void* result = NULL;
	if (DSSapi != NULL)
	{
		result = DSSapi->getutilid(L, DSS_LibID, &err);
		if (err != DSS_SUCCESS)
		{
			luaL_error(L, "Cannot collect utilid from DSS");	// call won't return
		}
	}
	else
	{
		luaL_error(L, "DSS not started");	// call won't return
	}
	return result;
}

// Deliver data to the Lua state asynchroneously
// checks existence of the API
int DSS_deliver(void* utilid, DSS_decoder_1v0_t pDecode, DSS_return_1v0_t pReturn, void* pData)
{
	if (DSSapi != NULL)
	{
		return DSSapi->deliver(utilid, pDecode, pReturn, pData);
	}
	else
	{
		return DSS_ERR_NOT_STARTED;
	}
}

// at shutdown, this will safely unregister the library
// use lua_State param if called from userdata __gc method
// use utilid param if called from the DSS cancel() method
// one param must be provided, lua_State has highest precedence
void DSS_shutdown(lua_State *L, void* utilid)
{
	if (DSSapi != NULL) 
	{
		if ((L == NULL) && ( utilid == NULL))
		{
			// TODO: must fail hard here
		}
		// If we got a Lua state, go lookup our utilid
		if (L != NULL) utilid = DSSapi->getutilid(L, DSS_LibID, NULL);
		// Unregister
		if (utilid != NULL) DSSapi->unreg(utilid);
		DSSapi = NULL;
	}
}
