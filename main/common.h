#pragma once

#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <sys/param.h>

#include "esp_crt_bundle.h"
#include "esp_event.h"
#include "esp_http_client.h"
#include "esp_https_server.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_system.h"
#include "esp_tls.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/task.h"
#include "lwip/err.h"
#include "lwip/sys.h"
#include "nvs_flash.h"

#define TAG "Monsys"

#define MONSYS_MONITOR_MAX_HANDLER    (4)
#define MONSYS_CONTROLLER_MAX_HANDLER (0xff)
#define MONSYS_BUF_SIZE               (2048)

void monsys_monitor_register(size_t (*h)(char *, size_t));
esp_err_t monsys_controller_register(uint8_t address, esp_err_t (*h)(char *, size_t));