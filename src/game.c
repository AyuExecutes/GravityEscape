#include <driver/gpio.h>
#include <esp_system.h>
#include "fonts.h"
#include "graphics.h"
#include "drawing.h"
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

struct enemy_block {
    float x;
    float y;
    float speed;
    bool active;
    int colour;
    int colour_step;
};

int const maximum_enemies = 12;
int const enemy_colour_step = 15;

void spawn_enemies(int number_of_enemies, int enemy_width, int enemy_height, int enemy_speed, struct enemy_block enemies[]){

    for (int i = 0; i < number_of_enemies; i++) {

        for (int j = 0; j < maximum_enemies; j++){

            if (enemies[j].active == false){

                enemies[j].x = rand() % (display_width - enemy_width);
                enemies[j].y = -((rand() % (enemy_height * 4)) + enemy_height);
                enemies[j].speed = enemy_speed;
                enemies[j].colour = 102;
                enemies[j].colour_step = enemy_colour_step;
                enemies[j].active = true;

                break;

            }
        }
    }
}

void render_game() {

    // for fps calculation
    int64_t current_time;
    int64_t last_time = esp_timer_get_time();

    int const player_speed = 2;
    int const player_width = display_width * 0.2;
    int const player_height = display_height * 0.05;

    int const enemy_height = display_height * 0.1;
    int const enemy_width = display_width * 0.05;

    int current_score = 0;
    int player_x = (display_width / 2) - (player_width / 2);
    int player_y = display_height - player_height;

    bool right_down = false, left_down = false;

    struct enemy_block enemies[maximum_enemies];

    // initialize enemies to inactive
    for (int i = 0; i < maximum_enemies; i++){
        enemies[i].active = false;
    }

    spawn_enemies(1, enemy_width, enemy_height, 1, enemies);

    int frame=0; 

    while(1) {
        cls(rgbToColour(50,50,50));
        //setFont(FONT_DEJAVU18);
        // draw_rectangle(0,3,display_width,24,rgbToColour(220,220,0));
        
        // setFontColour(0, 0, 0);
        // setFontColour(255, 255, 255);
        // setFont(FONT_UBUNTU16);

        draw_rectangle(player_x, player_y, player_width, player_height, rgbToColour(153, 255, 153));

        for (int i = 0; i < maximum_enemies; i++){

            if (enemies[i].active == true){
                
                draw_rectangle(enemies[i].x, enemies[i].y, enemy_width, enemy_height, rgbToColour(enemies[i].colour, 0, 0));
            }
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

        key_type key=get_input();

        switch(key) {
            case LEFT_DOWN: 
                left_down = true;
                break;

            case LEFT_UP: 
                left_down = false;
                break; 

            case RIGHT_DOWN: 
                right_down = true;
                break;

            case RIGHT_UP: 
                right_down = false;
                break;

            case NO_KEY: 
                break;
        }

        if(left_down){
            
            player_x -= player_speed;

            if (player_x < 0) {
                player_x = 0;
            }
        }


        if(right_down){
            
            player_x += player_speed;

            if (player_x >= display_width - player_width) {
                player_x = display_width - player_width;
            }
        }

        for (int i = 0; i < maximum_enemies; i++){

            if (enemies[i].active == true){

                enemies[i].y += enemies[i].speed;

                if (enemies[i].y > display_height){
                    enemies[i].active = false;

                    spawn_enemies(4, enemy_width, enemy_height, 1, enemies);

                } else {
                    
                    enemies[i].colour += enemies[i].colour_step;

                    if (enemies[i].colour > 255){
                        enemies[i].colour = 255;
                        enemies[i].colour_step = -enemy_colour_step;
                    } 

                    if (enemies[i].colour < 102){
                        enemies[i].colour = 102;
                        enemies[i].colour_step = enemy_colour_step;
                    }

                }
            }
        }
        
    }
}



