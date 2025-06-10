

#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"

#include "esp_wifi.h"
#include "nvs_flash.h"

#include "esp_mac.h"
#include "esp_bridge.h"
#include "esp_mesh_lite.h"

uint8_t root_node_mac[6] = {0xdc, 0x06, 0x75, 0xf8, 0xb4, 0xa8};

bool is_mesh_root = false;

void mesh_lite_init(void)
{
    static char softap_ssid[32] = {0};

    snprintf(softap_ssid, sizeof(softap_ssid), "%.31s", CONFIG_BRIDGE_SOFTAP_SSID);

    esp_mesh_lite_set_softap_info(softap_ssid, CONFIG_BRIDGE_SOFTAP_PASSWORD);

    wifi_config_t wifi_config = {0};
    esp_bridge_wifi_set_config(WIFI_IF_STA, &wifi_config);
    snprintf((char *)wifi_config.ap.ssid, sizeof(wifi_config.ap.ssid), "%s", softap_ssid);
    strlcpy((char *)wifi_config.ap.password, CONFIG_BRIDGE_SOFTAP_PASSWORD,
            sizeof(wifi_config.ap.password));

    wifi_config.ap.channel = 0;

    ESP_ERROR_CHECK(esp_bridge_wifi_set_config(WIFI_IF_AP, &wifi_config));

    esp_mesh_lite_config_t mesh_lite_config = ESP_MESH_LITE_DEFAULT_INIT();

    mesh_lite_config.join_mesh_ignore_router_status = true;
    mesh_lite_config.join_mesh_without_configured_wifi = !is_mesh_root;

    esp_mesh_lite_init(&mesh_lite_config);
}

static void print_system_info_timercb(TimerHandle_t timer)
{
    printf("\033[0;33m==========\033[0m mesh_node_number: %" PRIu32 " Free heap: %" PRIu32 " \033[0;33m==========\033[0m\n",
           esp_mesh_lite_get_mesh_node_number(), esp_get_free_heap_size());
}
void app_main(void)
{

    uint8_t sta_mac[6];
    esp_read_mac(sta_mac, ESP_IF_WIFI_STA);
    if (memcmp(root_node_mac, sta_mac, 6) == 0)
    {
        is_mesh_root = true;
    }
    printf("is_mesh_root %d", is_mesh_root);

    nvs_flash_init();

    esp_event_loop_create_default();

    ESP_ERROR_CHECK(esp_netif_init());

    esp_bridge_create_all_netif();

    mesh_lite_init();

    if (is_mesh_root)
    {
        esp_mesh_lite_set_allowed_level(1);
    }
    else
    {
        esp_mesh_lite_set_disallowed_level(1);
    }
    esp_mesh_lite_start();

    xTimerStart(xTimerCreate("performance", pdMS_TO_TICKS(10000), pdTRUE, NULL, print_system_info_timercb), 0);
}
