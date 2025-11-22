# main/CMakeLists.txt

# Tên component và file nguồn
idf_component_register(
    SRCS "hello_world_main.c"         # đổi tên file nếu bạn đặt khác
    INCLUDE_DIRS "."                  # thư mục chứa file .h nếu có
    REQUIRES driver esp_netif esp_wifi nvs_flash esp_http_server freertos log
)
