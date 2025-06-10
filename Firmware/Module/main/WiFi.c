#include <string.h>

#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"

#include "NVSUtil.h"

static const char* TAG = "WiFi";

// FreeRTOS event group to handle WiFi events
static EventGroupHandle_t s_wifi_event_group;
#define CONNECTED_BIT BIT0
#define DISCONNECTED_BIT BIT1

// WiFi event handler
static void wifi_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data) {
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {                 // WiFi started: initial connect
        ESP_LOGI(TAG, "Connecting to WiFi");
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {   // Lost connection: retry
        ESP_LOGI(TAG, "Lost connection to Wifi, attempting reconnection");
        xEventGroupClearBits(s_wifi_event_group, CONNECTED_BIT);    // Clear the connected bit
        xEventGroupSetBits(s_wifi_event_group, DISCONNECTED_BIT);   // Set the disconnected bit
        esp_wifi_connect();
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {             // Successfully connected
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "Connected to WiFi with ip " IPSTR, IP2STR(&event->ip_info.ip));
        xEventGroupClearBits(s_wifi_event_group, DISCONNECTED_BIT); // Clear the disconnected bit
        xEventGroupSetBits(s_wifi_event_group, CONNECTED_BIT);      // Set the connected bit
    } else {
        ESP_LOGI(TAG, "unknown event base = %s, id = %ld", event_base, event_id);
    }
}

// Get WiFi info from NVS, start WiFi event handler, and start WiFi
bool startWifi() {
    wifi_config_t wifiCfg = {0};
    char* ssid;
    char* pswd;

    if (!nvsGetStr("NTWK_SSID", &ssid)) ESP_LOGE(TAG, "Cannot connect to WiFi: NTWK_SSID not in NVS");
    if (!nvsGetStr("NTWK_PSWD", &pswd)) ESP_LOGE(TAG, "Cannot connect to WiFi: NTWK_PSWD not in NVS");

    // Register the WiFi event handler for WiFi-related events
    s_wifi_event_group = xEventGroupCreate();
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL));

    // Create init config and NETIF
    ESP_ERROR_CHECK(esp_netif_init());
    esp_netif_create_default_wifi_sta();
    wifi_init_config_t initCfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&initCfg));

    strncpy((char*)wifiCfg.sta.ssid, ssid, strlen(ssid));
    strncpy((char*)wifiCfg.sta.password, pswd, strlen(pswd));

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifiCfg));
    ESP_ERROR_CHECK(esp_wifi_start());

    // Don't call esp_wifi_connect() here -- let our WiFi event handler deal with the WIFI_EVENT_STA_START
    // However, callers expect us to block until WiFi connects
    // So, wait on our "successful connection" bit that we trigger in the WiFi event handler
    // Do not set xClearOnExit: our WifiHandler will always keep the connected/disconnected bits accurate
    xEventGroupWaitBits(s_wifi_event_group, CONNECTED_BIT, pdFALSE, pdFALSE, portMAX_DELAY);

    ESP_LOGI(TAG, "WiFi finished initialization");

    return true;
}