/* ESP HTTP Client Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include "http.h"

#define MAX_HTTP_OUTPUT_BUFFER 2048

httpd_handle_t _httpd_server;
httpd_uri_t root_get_uri =
                {
                    .uri = "/*",
                    .method = HTTP_GET,
                    .handler = NULL,
},
            root_post_uri = {
                .uri = "/api/v1/*",
                .method = HTTP_POST,
                .handler = NULL,
};

esp_err_t _http_event_handler(esp_http_client_event_t *evt)
{
    static char *output_buffer;  // Buffer to store response of http request from event handler
    static int output_len;       // Stores number of bytes read
    switch (evt->event_id) {
        case HTTP_EVENT_ERROR:
            ESP_LOGD(TAG, "HTTP_EVENT_ERROR");
            break;
        case HTTP_EVENT_ON_CONNECTED:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_CONNECTED");
            break;
        case HTTP_EVENT_HEADER_SENT:
            ESP_LOGD(TAG, "HTTP_EVENT_HEADER_SENT");
            break;
        case HTTP_EVENT_ON_HEADER:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_HEADER, key=%s, value=%s", evt->header_key,
                     evt->header_value);
            break;
        case HTTP_EVENT_ON_DATA:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_DATA, len=%d", evt->data_len);
            // Clean the buffer in case of a new request
            if (output_len == 0 && evt->user_data) {
                // we are just starting to copy the output data into the use
                memset(evt->user_data, 0, MAX_HTTP_OUTPUT_BUFFER);
            }
            if (!esp_http_client_is_chunked_response(evt->client)) {
                // If user_data buffer is configured, copy the response into the buffer
                int copy_len = 0;
                if (evt->user_data) {
                    // The last byte in evt->user_data is kept for the NULL character in case of
                    // out-of-bound access.
                    copy_len = MIN(evt->data_len, (MAX_HTTP_OUTPUT_BUFFER - output_len));
                    if (copy_len) { memcpy(evt->user_data + output_len, evt->data, copy_len); }
                } else {
                    int content_len = esp_http_client_get_content_length(evt->client);
                    if (output_buffer == NULL) {
                        // We initialize output_buffer with 0 because it is used by strlen() and
                        // similar functions therefore should be null terminated.
                        output_buffer = (char *)calloc(content_len + 1, sizeof(char));
                        output_len = 0;
                        if (output_buffer == NULL) {
                            ESP_LOGE(TAG, "Failed to allocate memory for output buffer");
                            return ESP_FAIL;
                        }
                    }
                    copy_len = MIN(evt->data_len, (content_len - output_len));
                    if (copy_len) { memcpy(output_buffer + output_len, evt->data, copy_len); }
                }
                output_len += copy_len;
            }

            break;
        case HTTP_EVENT_ON_FINISH:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_FINISH");
            if (output_buffer != NULL) {
                free(output_buffer);
                output_buffer = NULL;
            }
            output_len = 0;
            break;
        case HTTP_EVENT_DISCONNECTED:
            ESP_LOGI(TAG, "HTTP_EVENT_DISCONNECTED");
            int mbedtls_err = 0;
            esp_err_t err = esp_tls_get_and_clear_last_error((esp_tls_error_handle_t)evt->data,
                                                             &mbedtls_err, NULL);
            if (err != 0) {
                ESP_LOGI(TAG, "Last esp error code: 0x%x", err);
                ESP_LOGI(TAG, "Last mbedtls failure: 0x%x", mbedtls_err);
            }
            if (output_buffer != NULL) {
                free(output_buffer);
                output_buffer = NULL;
            }
            output_len = 0;
            break;
        case HTTP_EVENT_REDIRECT:
            ESP_LOGD(TAG, "HTTP_EVENT_REDIRECT");
            esp_http_client_set_header(evt->client, "From", "user@example.com");
            esp_http_client_set_header(evt->client, "Accept", "text/html");
            esp_http_client_set_redirection(evt->client);
            break;
    }
    return ESP_OK;
}

esp_err_t http_cli_post(const void *data, size_t len)
{
    char buf[MAX_HTTP_OUTPUT_BUFFER + 1] = {0};

    esp_http_client_config_t http_conf = {
        .url = HTTP_CLI_REST_API_URL,
        .user_data = buf,
        .event_handler = _http_event_handler,
    };

    esp_http_client_handle_t cli_handle = esp_http_client_init(&http_conf);

    esp_http_client_set_method(cli_handle, HTTP_METHOD_POST);
    esp_http_client_set_header(cli_handle, "Content-Type", "application/json");
    esp_http_client_set_post_field(cli_handle, data, len);
    esp_err_t err = esp_http_client_perform(cli_handle);
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "HTTP POST Status = %d, content_length = %" PRId64,
                 esp_http_client_get_status_code(cli_handle),
                 esp_http_client_get_content_length(cli_handle));
    } else {
        ESP_LOGE(TAG, "HTTP POST request failed: %s", esp_err_to_name(err));
    }

    esp_http_client_cleanup(cli_handle);
    return err;
}

esp_err_t http_cli_init(void)
{
    ESP_LOGI(TAG, "Starting HTTP Client Init");
    ESP_LOGI(TAG, "Starting HTTP Client Finished");

    return ESP_OK;
}

esp_err_t httpd_init(esp_err_t (*get_h)(httpd_req_t *), esp_err_t (*post_h)(httpd_req_t *))
{
    httpd_handle_t server = NULL;

    ESP_LOGI(TAG, "Starting HTTPS Server");

    httpd_ssl_config_t conf = HTTPD_SSL_CONFIG_DEFAULT();

    extern const unsigned char servercert_start[] asm("_binary_server_pem_start");
    extern const unsigned char servercert_end[] asm("_binary_server_pem_end");
    conf.servercert = servercert_start;
    conf.servercert_len = servercert_end - servercert_start;

    extern const unsigned char prvtkey_pem_start[] asm("_binary_server_key_pem_start");
    extern const unsigned char prvtkey_pem_end[] asm("_binary_server_key_pem_end");
    conf.prvtkey_pem = prvtkey_pem_start;
    conf.prvtkey_len = prvtkey_pem_end - prvtkey_pem_start;

    conf.httpd.uri_match_fn = httpd_uri_match_wildcard;
    root_get_uri.handler = get_h;
    root_post_uri.handler = post_h;

    ESP_ERROR_CHECK(httpd_ssl_start(&server, &conf));

    httpd_register_uri_handler(server, &root_get_uri);
    httpd_register_uri_handler(server, &root_post_uri);
    ESP_LOGI(TAG, "HTTPS Sever Init Finished!");
    return ESP_OK;
}