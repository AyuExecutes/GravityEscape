/* TTGO Demo example for 159236

*/
#include <driver/gpio.h>

#include <esp_system.h>
#include <esp_wifi.h>
#include <freertos/FreeRTOS.h>
#include <freertos/event_groups.h>
#include <freertos/task.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#include <math.h>
#include <esp_log.h>
#include <esp_sntp.h>
#include <nvs_flash.h>

#include "fonts.h"
#include "graphics.h"
#include "input_output.h"
#include "drawing.h"

#define PAD_START 3
#define PAD_END 5

#define SHOW_PADS

void app_main() {
    // initialise button handling
    input_output_init();
    // Initialize NVS
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        // NVS partition was truncated and needs to be erased
        // Retry nvs_flash_init
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK( err );
    // ===== Set time zone to NZ time using custom daylight savings rule======
    // if you are anywhere else in the world, this will need to be changed
    setenv("TZ", "NZST-12:00:00NZDT-13:00:00,M9.5.0,M4.1.0", 0);
    tzset();
    // initialise graphics and lcd display
    graphics_init();
    cls(0);
    // main menu
    int sel=0;
    while(1) {
        char *entries[]={"Start Game", "Highscore"};
        sel=render_menu("Welcome",sizeof(entries)/sizeof(char *),entries,sel);
        switch(sel) {
            case 0:
                //start_game();            
                break;
            case 1:
                //show_high_score();
                break;
            default:
                return;
        }
    }
}
