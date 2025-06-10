#include "esp_system.h"

void nvsInit();
bool nvsGetInt(const char* key, uint32_t* out);
bool nvsGetStr(const char* key, char** pOut);
bool nvsSetInt(const char* key, uint32_t val);
bool nvsSetStr(const char* key, const char* val);
bool nvsCommit();
void nvsDump();
void nvsWipe();