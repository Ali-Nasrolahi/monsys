#include "common.h"
#include "http.h"
#include "network.h"

size_t (*monsys_monitor_handler[MONSYS_MONITOR_MAX_HANDLER])(char *, size_t);
esp_err_t (*monsys_controller_handler[MONSYS_CONTROLLER_MAX_HANDLER])(char *, size_t);

/***************** Declarations ******************/
static esp_err_t controller_post_req(httpd_req_t *req);
static esp_err_t controller_get_req(httpd_req_t *req);
static void monitor_propagate(void *, size_t);
/***************** Declarations ******************/

void monsys_monitor_register(size_t (*h)(char *, size_t))
{
    uint16_t i = 0;
    while (i < MONSYS_MONITOR_MAX_HANDLER && monsys_monitor_handler[i++]);
    if (i <= MONSYS_MONITOR_MAX_HANDLER) monsys_monitor_handler[i - 1] = h;
}

esp_err_t monsys_controller_register(uint8_t addr, esp_err_t (*h)(char *, size_t))
{
    if (addr >= MONSYS_CONTROLLER_MAX_HANDLER || monsys_controller_handler[addr]) return ESP_FAIL;
    monsys_controller_handler[addr] = h;
    return ESP_OK;
}

void monsys_monitor(void *)
{
    uint16_t i = 0;
    ESP_ERROR_CHECK(http_cli_init());
    char buf[MONSYS_BUF_SIZE] = {0};

    while (1) {
        while (monsys_monitor_handler[i]) {
            monitor_propagate(buf, monsys_monitor_handler[i++](buf, sizeof(buf)));
        }
        vTaskDelay(10000);
    }
}

void monsys_controller_init(void)
{
    ESP_ERROR_CHECK(httpd_init(controller_get_req, controller_post_req));
}

static esp_err_t controller_get_req(httpd_req_t *req)
{
    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, "<h1>Get Request Works, Use Post instead!</h1>", HTTPD_RESP_USE_STRLEN);

    return ESP_OK;
}

static esp_err_t controller_post_req(httpd_req_t *req)
{
    esp_err_t status = ESP_FAIL;
    char content[MONSYS_BUF_SIZE] = {0}, addr_s[5] = {0};
    size_t len;
    httpd_resp_set_type(req, "application/json");

    if ((len = httpd_req_recv(req, addr_s, 4)) == 4 &&
        (len = httpd_req_recv(req, content, sizeof(content))) > 0) {
        uint8_t addr = (uint8_t)strtol(addr_s, NULL, 0);  // address format: 0x00 - 0xff (4 char)
        if (monsys_controller_handler[addr]) status = monsys_controller_handler[addr](content, len);
    }

    httpd_resp_send(req, status == ESP_OK ? "{status:ok}" : "{status:fail}", HTTPD_RESP_USE_STRLEN);

    return ESP_OK;
}

static void provision(void)
{
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }

    ESP_ERROR_CHECK(ret);
    ESP_ERROR_CHECK(network_init());
}

static void monitor_propagate(void *buf, size_t len)
{
    http_cli_post(buf, len);

    /* Or any other propagation protocol.....*/
}

/*************** TESTS ********************/
static size_t fetch_temperature(char *buf, size_t len)
{
    /* TODO use a real peripheral */
    return (size_t)(stpncpy(buf, "{temperature: 88, unit: fahrenheit}", len) - buf);
}

static esp_err_t test_controller(char *buf, size_t len)
{
    /* TODO use a real peripheral */
    ESP_LOGI(TAG, "Test Controller: %s", buf);
    return ESP_OK;
}
/*************** TESTS ********************/

void app_main(void)
{
    /* Monitoring Handlers*/
    monsys_monitor_register(fetch_temperature);

    /* Controllers */
    ESP_ERROR_CHECK(monsys_controller_register(0x55, test_controller));

    provision();
    monsys_controller_init();
    xTaskCreate(&monsys_monitor, "Monitor", 8192, NULL, 5, NULL);
}