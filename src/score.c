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

// The idea is to show a background of rectangles creating a pulsating effect
struct rectangle_background {
    int x;
    int y;
    int width;
    int height;
    int colour;
    int colour_step;
};

// The constant values
const int total_rectangles = 6;     // total number of rectangles will be displayed
const int colour_step_speed = 3;    // speed of the colour change 
const int wait_time_until_key_press = 2000000; // 2 seconds wait time until key press is allowed

void render_score(int last_score){

    // for fps calculation
    int64_t start_time = esp_timer_get_time();
    int64_t current_time;
    int64_t last_time = esp_timer_get_time();

    // for the key press to be only appeared when after 2 seconds passed (to stop accidental key presses)
    bool allow_key_presses = false;

    struct rectangle_background rectangles[total_rectangles];
    int rectangle_size_step = display_width / total_rectangles / 2;     // to make it even on both sides
    int current_horizontal = 0;
    int current_vertical = 0;
    int current_colour = 255;                                           // only changing red colour (starting at 255)
    int colour_change = 255 / total_rectangles;                         // each rectangle will be different gradient of red

    // create each rectangle to be displayed
    for (int i = 0; i < total_rectangles; i++){

        // set the rectangles based on the current position and size
        rectangles[i].x = current_horizontal;
        rectangles[i].y = current_vertical;
        rectangles[i].width = display_width - current_horizontal * 2;
        rectangles[i].height = display_height - current_vertical * 2;

        // set the colour darker for each rectangle as they get smaller
        rectangles[i].colour = current_colour;
        rectangles[i].colour_step = colour_step_speed;

        // set the size for the next rectangle 
        current_horizontal += rectangle_size_step;
        current_vertical += rectangle_size_step;
        current_colour -= colour_change;
    }

    int frame = 0; 
    char score_str[256];

    while(1) {

        cls(rgbToColour(50, 50, 50));

        // draw the rectangles 
        for (int i = 0; i < total_rectangles; i++){

            draw_rectangle(rectangles[i].x, rectangles[i].y, rectangles[i].width, rectangles[i].height, rgbToColour(rectangles[i].colour, 0, 0));

            // animate the colour change
            rectangles[i].colour += rectangles[i].colour_step;


            // check for the boundaries and depending on that, the direction is also changed (bouncing up and down effect)
            if (rectangles[i].colour > 255){
                rectangles[i].colour = 255;
                rectangles[i].colour_step = -colour_step_speed;
            }

            if (rectangles[i].colour < 0){
                rectangles[i].colour = 0;
                rectangles[i].colour_step = colour_step_speed;
            }
        }

        // For the Game Over text
        setFont(FONT_DEJAVU18);
        setFontColour(255, 255, 0);
        print_xy("GAME OVER", CENTER, CENTER);

        // For the Score text
        setFontColour(0, 255, 0);
        snprintf(score_str,64,"Score: %d", last_score);
        print_xy(score_str,CENTER, LASTY + 18);

        if (allow_key_presses){

            setFontColour(255, 255, 255);
            setFont(FONT_SMALL);
            print_xy("Press any key", CENTER, display_height - 24);

        }

        flip_frame();

        current_time = esp_timer_get_time();
        if ((frame++ % 10) == 0) {
            printf("FPS:%f %d %d\n", 1.0e6 / (current_time - last_time),
                heap_caps_get_free_size(MALLOC_CAP_DMA),
                heap_caps_get_free_size(MALLOC_CAP_32BIT));
            
            vTaskDelay(1);
        }

        last_time = current_time;

        // check if time has passed 2 seconds, then allow key presses
        if (current_time - start_time > wait_time_until_key_press){
            allow_key_presses = true;
        }

        if (allow_key_presses){

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
}



