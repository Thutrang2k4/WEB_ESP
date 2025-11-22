/*
 * SPDX-FileCopyrightText: 2010-2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 *
 * ESP32-S3 NeoPixel RGB Web Server (RMT)
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
#include "driver/rmt.h"

#define LED_NEO_PIN    45
#define RMT_TX_CHANNEL RMT_CHANNEL_0

static const char *TAG = "NEO_RGB_SERVER";

// ================== WS2812 Helper ==================
void ws2812_send_pixel(uint8_t r, uint8_t g, uint8_t b)
{
    uint8_t colors[3] = {g, r, b}; // WS2812 dùng GRB
    rmt_item32_t items[24];

    for(int i=0; i<3; i++){
        for(int j=0; j<8; j++){
            if(colors[i] & (1 << (7-j))){
                items[i*8 + j].duration0 = 8;   // T_HIGH
                items[i*8 + j].level0 = 1;
                items[i*8 + j].duration1 = 4;   // T_LOW
                items[i*8 + j].level1 = 0;
            } else {
                items[i*8 + j].duration0 = 4;
                items[i*8 + j].level0 = 1;
                items[i*8 + j].duration1 = 8;
                items[i*8 + j].level1 = 0;
            }
        }
    }
    rmt_write_items(RMT_TX_CHANNEL, items, 24, true);
    rmt_wait_tx_done(RMT_TX_CHANNEL, portMAX_DELAY);
}

void set_color(const char* color)
{
    if (!color) return;
    if (!strcmp(color, "red")) {
        ws2812_send_pixel(255,0,0);
    } else if (!strcmp(color, "green")) {
        ws2812_send_pixel(0,255,0);
    } else if (!strcmp(color, "blue")) {
        ws2812_send_pixel(0,0,255);
    } else if (!strcmp(color, "off")) {
        ws2812_send_pixel(0,0,0);
    }
}

// ================== WiFi Event Handler ==================
static void wifi_event_handler(void* arg, esp_event_base_t event_base,
                               int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        esp_wifi_connect();  // reconnect
        ESP_LOGI(TAG, "Reconnecting to WiFi...");
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "Got IP: " IPSTR, IP2STR(&event->ip_info.ip));
    }
}

// ================== WiFi STA ==================
void wifi_init_sta(void)
{
    esp_netif_init();
    esp_event_loop_create_default();
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&cfg);

    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    esp_event_handler_instance_register(WIFI_EVENT,
                                        ESP_EVENT_ANY_ID,
                                        &wifi_event_handler,
                                        NULL,
                                        &instance_any_id);
    esp_event_handler_instance_register(IP_EVENT,
                                        IP_EVENT_STA_GOT_IP,
                                        &wifi_event_handler,
                                        NULL,
                                        &instance_got_ip);

    wifi_config_t wifi_conf = {
        .sta = {
            .ssid = "THU TRANG",       // <-- đổi đúng WiFi
            .password = "thutrang260404"    // <-- đổi đúng password
        }
    };

    esp_wifi_set_mode(WIFI_MODE_STA);
    esp_wifi_set_config(WIFI_IF_STA, &wifi_conf);
    esp_wifi_start();
}

// ================== HTTP Handler ==================
static esp_err_t rgb_handler(httpd_req_t *req)
{
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*"); // CORS

    char buf[100];
    int len = httpd_req_get_url_query_len(req) + 1;
    if (len > 1) {
        httpd_req_get_url_query_str(req, buf, len);
        char color[20];
        if (httpd_query_key_value(buf, "color", color, sizeof(color)) == ESP_OK) {
            ESP_LOGI(TAG, "Color = %s", color);
            set_color(color); // đổi màu NeoPixel
        }
    }

    httpd_resp_set_type(req, "text/plain");
    return httpd_resp_sendstr(req, "OK");
}

// ================== Web Server ==================
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

// ================== Main ==================
void app_main(void)
{
    nvs_flash_init();
    wifi_init_sta();

    // --- Khởi tạo RMT cho NeoPixel ---
    rmt_config_t config = RMT_DEFAULT_CONFIG_TX(LED_NEO_PIN, RMT_TX_CHANNEL);
    config.clk_div = 2;
    rmt_config(&config);
    rmt_driver_install(config.channel, 0, 0);

    ESP_LOGI(TAG, "NeoPixel ready!");

    // --- Chờ WiFi nhận IP ---
    vTaskDelay(5000 / portTICK_PERIOD_MS);

    ESP_LOGI(TAG, "Starting Web Server...");
    start_webserver();
}
