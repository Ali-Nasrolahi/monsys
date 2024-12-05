#pragma once

#include "common.h"

/* Remote REST Server Settings */
#define HTTP_CLI_REST_API_URL "https://192.168.8.253:8080/api/v1/T2_TEST_TOKEN/telemetry"

esp_err_t http_cli_post(const void *data, size_t len);
esp_err_t http_cli_init(void);
esp_err_t httpd_init(esp_err_t (*get_h)(httpd_req_t *), esp_err_t (*post_h)(httpd_req_t *));
