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
#include "menu.h"
#include "game.h"
#include "score.h"
#include "instructions.h"

#define PAD_START 3
#define PAD_END 5

#define SHOW_PADS

enum CurrentScreen{
    MENU, GAME, SCORE, INSTRUCTIONS
};

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

    // Initialize graphics, clear screen and set orientation to always portrait
    graphics_init();
    cls(0);
    set_orientation(PORTRAIT);

    // Set current screen to Menu which is the first screen to be displayed
    enum CurrentScreen currentScreen = INSTRUCTIONS;
    
    int last_score = 0;
    int selected_menu = 0;
    
    // Menu has two entries: to start the game and to view the highscore
    char *entries[]={"Start Game", "Instructions"};

    while(1) {

        switch (currentScreen){
            
            // Show the Main Menu screen / Welcome screen 
            case MENU:

                // Show the menu and wait for the respond based on the selection
                selected_menu = render_menu("Welcome",sizeof(entries)/sizeof(char *), entries, selected_menu);

                switch (selected_menu) {

                    case 0:
                        currentScreen = GAME;          
                        break;
                    case 1:
                        currentScreen = INSTRUCTIONS;
                        break;
                    default:
                        return;
                }

                break;

            // Show the Game screen to play the game
            case GAME:
                last_score = render_game();
                currentScreen = SCORE;
                break;

            // Show the Score screen to display the score after the game is over
            case SCORE:

                // Do a check of the current score, if it is larger than the last high score, then high score is updated
                if (last_score > storage_read_int("high_score", 0)){

                    storage_write_int("high_score", last_score);
                }

                render_score(last_score);
                currentScreen = MENU;
                break;

            // Show the Instructions before the game starts / also when user click
            case INSTRUCTIONS:
                render_instructions();
                currentScreen = MENU;
                break;

            default:
                return;

        }
    }
}
