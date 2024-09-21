#include <driver/gpio.h>
#include <esp_system.h>
#include "fonts.h"
#include "graphics.h"
#include "menu.h"
#include "input_output.h"
#include <driver/touch_pad.h>
#include <freertos/FreeRTOS.h>
#include <freertos/event_groups.h>
#include <freertos/timers.h>
#include <freertos/task.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <esp_log.h>
#include <esp_sntp.h>
#include <nvs_flash.h>

void render_instructions(){

    // for fps calculation
    int64_t current_time;
    int64_t last_time = esp_timer_get_time();

    int frame = 0; 

    while(1) {

        cls(rgbToColour(153, 0, 76)); 

        // The Insructions title
        setFont(FONT_UBUNTU16);
        setFontColour(255, 255, 255);
        gprintf("INSTRUCTIONS\n\n");

        // The instructions text
        setFont(FONT_SMALL);
        gprintf("Start: Press 'Start Game'.\n\n");
        gprintf("Controls: Right to select,\nLeft to keep choosing.\n\n");
        gprintf("Gameplay: You start in the\nmiddle and avoid falling\nenemies.\n\n");
        gprintf("Levels: Level up every 15s,\nmax level is 5. Speed and\nenemies increase.\n\n");
        gprintf("Warning: Orange borders\nappear if danger is near.\n\n");
        gprintf("Game Over: Hit an enemy,\ngame ends, high score\nshown.");

        // To get out of the instructions screen
        setFontColour(255, 255, 255);
        setFont(FONT_SMALL);
        print_xy("Press any key", CENTER, display_height - 24);

        flip_frame();

        current_time = esp_timer_get_time();
        if ((frame++ % 10) == 0) {
            printf("FPS:%f %d %d\n", 1.0e6 / (current_time - last_time),
                heap_caps_get_free_size(MALLOC_CAP_DMA),
                heap_caps_get_free_size(MALLOC_CAP_32BIT));
            
            vTaskDelay(1);
        }

        last_time = current_time;

        key_type key = get_input();

        switch(key) {
            case LEFT_DOWN: 
            case RIGHT_DOWN: 
                return;
                break;

            default: 
                break;
        }
        
    }
}



