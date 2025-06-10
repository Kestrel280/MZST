#include "esp_system.h"
#include <esp_log.h>
#include "nvs_flash.h"
#include "nvs.h"

#include "NVSUtil.h"

const char* TAG = "nvsutil";    // Logging tag
nvs_handle_t nvsHandle = 0;     // Initialized in initNvs()
esp_err_t err;                  // Return value from standard nvs functions

#define __nvs_check_init() if (!nvsHandle) {ESP_LOGE(TAG, "%s called before initNvs()", __PRETTY_FUNCTION__); exit(1); }

void nvsInit() {
    err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);

    ESP_LOGI(TAG, "Initialized NVS");

    err = nvs_open("storage", NVS_READWRITE, &nvsHandle);
    ESP_ERROR_CHECK(err);
}

bool nvsGetInt(const char* key, uint32_t* out) {
    __nvs_check_init();
    err = nvs_get_u32(nvsHandle, key, out);
    switch (err) {
        case ESP_OK: return true;
        case ESP_ERR_NVS_NOT_FOUND: return false;
        default: ESP_LOGE(TAG, "Unusual error '%d' getting NVS value for key '%s'", err, key); return false;
    }
    // unreachable
}

bool nvsGetStr(const char* key, char** pOut) {
    size_t required_size;
    __nvs_check_init();
    err = nvs_get_str(nvsHandle, key, NULL, &required_size); // Get required size to store string
    switch (err) {
        case ESP_OK: break;
        case ESP_ERR_NVS_NOT_FOUND: ESP_LOGI(TAG, "Failure in %s: key '%s' not found", __PRETTY_FUNCTION__, key); return false;
        default: ESP_LOGE(TAG, "Unusual error '%d' getting NVS value for key '%s'", err, key); return false;
    }

    *pOut = (char*)malloc(required_size);
    err = nvs_get_str(nvsHandle, key, *pOut, &required_size);
    
    switch (err) { // No clause for missing key, because we've already passed that
        case ESP_OK: return true;
        default: ESP_LOGE(TAG, "Unusual error '%d' getting NVS value for key '%s'", err, key); return false;
    }
    // unreachable
}

bool nvsSetInt(const char* key, uint32_t val) {
    __nvs_check_init();
    err = nvs_set_u32(nvsHandle, key, val);
    switch (err) {
        case ESP_OK: return true;
        default: ESP_LOGE(TAG, "Unusual error '%d' setting NVS value '%lu' for key '%s'", err, val, key); return false;
    }
    // unreachable
}

bool nvsSetStr(const char* key, const char* val) {
    __nvs_check_init();
    err = nvs_set_str(nvsHandle, key, val);
    switch (err) {
        case ESP_OK: return true;
        default: ESP_LOGE(TAG, "Unusual error '%d' setting NVS string '%s' for key '%s'", err, val, key); return false;
    }
    // unreachable
}

bool nvsCommit() {
    __nvs_check_init();
    err = nvs_commit(nvsHandle);
    switch (err) {
        case ESP_OK: return true;
        default: ESP_LOGE(TAG, "Unusual error '%d' committing NVS", err); return false;
    }
    // unreachable
}

void dumpNvs() {
    __nvs_check_init();
    ESP_LOGE(TAG, "dumpNvs() unimplemented");
}

void nvsWipe() {
    __nvs_check_init();
    ESP_LOGE(TAG, "nvsWipe() unimplemented");
}