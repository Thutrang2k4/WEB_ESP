/*
 * SPDX-FileCopyrightText: 2010-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */



#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include "esp_http_server.h"
#include "driver/gpio.h"

// --- GPIO LED RGB ---
#define LED_R  15
#define LED_G  2
#define LED_B  4

static const char *TAG = "RGB_SERVER";

// ================== RGB Handler ==================
static esp_err_t rgb_handler(httpd_req_t *req)
{
    char buf[100];
    int len = httpd_req_get_url_query_len(req) + 1;

    if (len > 1) {
        httpd_req_get_url_query_str(req, buf, len);
        char color[20];

        if (httpd_query_key_value(buf, "color", color, sizeof(color)) == ESP_OK) {
            ESP_LOGI(TAG, "Color = %s", color);

            if (!strcmp(color, "red")) {
                gpio_set_level(LED_R, 1);
                gpio_set_level(LED_G, 0);
                gpio_set_level(LED_B, 0);
            } else if (!strcmp(color, "green")) {
                gpio_set_level(LED_R, 0);
                gpio_set_level(LED_G, 1);
                gpio_set_level(LED_B, 0);
            } else if (!strcmp(color, "blue")) {
                gpio_set_level(LED_R, 0);
                gpio_set_level(LED_G, 0);
                gpio_set_level(LED_B, 1);
            } else if (!strcmp(color, "off")) {
                gpio_set_level(LED_R, 0);
                gpio_set_level(LED_G, 0);
                gpio_set_level(LED_B, 0);
            }
        }
    }

    httpd_resp_set_type(req, "text/plain");
    return httpd_resp_sendstr(req, "OK");
}

// ================== Start Server ==================
static httpd_handle_t start_webserver(void)
{
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    httpd_handle_t server = NULL;

    if (httpd_start(&server, &config) == ESP_OK) {
        httpd_uri_t rgb_uri = {
            .uri       = "/rgb",
            .method    = HTTP_GET,
            .handler   = rgb_handler
        };
        httpd_register_uri_handler(server, &rgb_uri);
    }
    return server;
}

// ================== WiFi STA Mode ==================
void wifi_init_sta(void)
{
    esp_netif_init();
    esp_event_loop_create_default();
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&cfg);

    wifi_config_t wifi_conf = {
        .sta = {
            .ssid = "YOUR_WIFI_SSID",
            .password = "YOUR_WIFI_PASS"
        }
    };

    esp_wifi_set_mode(WIFI_MODE_STA);
    esp_wifi_set_config(WIFI_IF_STA, &wifi_conf);
    esp_wifi_start();
    esp_wifi_connect();
}

// ================== Main ==================
void app_main(void)
{
    nvs_flash_init();

    gpio_set_direction(LED_R, GPIO_MODE_OUTPUT);
    gpio_set_direction(LED_G, GPIO_MODE_OUTPUT);
    gpio_set_direction(LED_B, GPIO_MODE_OUTPUT);

    wifi_init_sta();

    vTaskDelay(5000 / portTICK_PERIOD_MS);  // chờ WiFi kết nối
    ESP_LOGI(TAG, "Starting Web Server...");
    start_webserver();
}
