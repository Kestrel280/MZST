#ifndef MZST_NVSUTIL_H
#define MZST_NVSUTIL_H

#include "esp_system.h"

// Defined in NVSUtil.c
// Using these instead of #defines to reduce code size. Unnecessary optimization
extern const char* NVS_NTWK_SSID_KEY;
extern const char* NVS_NTWK_PSWD_KEY;
extern const char* NVS_SERVER_IP_KEY;
extern const char* NVS_SERVER_PORT_KEY;
extern const char* NVS_MODULE_ID_KEY;
extern const char* NVS_VERSION_KEY;

void nvsInit();                                     // Initial NVS setup -- required before calling any other NVSUtil function
bool nvsGetInt(const char* key, uint32_t* out);     // Retrieve int32 value from NVS, stores it in 'out'. Returns 'false' on error (incl. key not found).
bool nvsGetStr(const char* key, char** pOut);       // Retrieve string from NVS, stores it in heap and places pointer in 'pOut'. Returns 'false' if key not found.
bool nvsSetInt(const char* key, uint32_t val);      // Set int32 value on NVS. Returns 'false' on error. Must call nvsCommit() afterwards (recommended to batch nvsSet...() calls before nvsCommit()).
bool nvsSetStr(const char* key, const char* val);   // Set string on NVS. Returns 'false' on error. Must call nvsCommit() afterwards (recommended to batch nvsSet...() calls before nvsCommit()).
bool nvsCommit();                                   // Commits any changes specified by nvsSet...() functions.
void nvsDump();                                     // (unimplemented) Prints all key:value pairs on NVS.
void nvsWipe();                                     // (unimplemented) Wipes all NVS data.

#endif