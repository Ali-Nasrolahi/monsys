#pragma once

#include "common.h"

/* Wi-Fi Settings */
#define NETWORK_WIFI_SSID      CONFIG_ESP_WIFI_SSID
#define NETWORK_WIFI_PASS      CONFIG_ESP_WIFI_PASSWORD
#define NETWORK_WIFI_MAX_RETRY CONFIG_ESP_MAXIMUM_RETRY

esp_err_t network_init(void);
void network_deinit(void);