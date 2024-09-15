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
int const points_per_miss = 100;

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

int render_game() {

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
    bool potential_hit = false;

    setFont(FONT_SMALL);
    setFontColour(255, 255, 255);

    while(1) {


        cls(rgbToColour(50,50,50));

        draw_rectangle(player_x, player_y, player_width, player_height, rgbToColour(153, 255, 153));

        for (int i = 0; i < maximum_enemies; i++){

            if (enemies[i].active == true){
                
                draw_rectangle(enemies[i].x, enemies[i].y, enemy_width, enemy_height, rgbToColour(enemies[i].colour, 0, 0));
            }
        }

        gprintf("Score: %d\n", current_score);

        if (potential_hit){
            // draw borders of warning left and right
            draw_rectangle(0, 0, 2, display_height, rgbToColour(255, 128, 0));
            draw_rectangle(display_width - 2, 0, display_width, display_height, rgbToColour(255, 128, 0));
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
        
        potential_hit = false;

        for (int i = 0; i < maximum_enemies; i++){

            if (enemies[i].active == true){

                int enemies_left = enemies[i].x;
                int enemies_right = enemies[i].x + enemy_width;
                int player_left = player_x;
                int player_right = player_x + player_width;

                if (((enemies_left > player_left) && (enemies_left < player_right)) || ((enemies_right > player_left) && (enemies_right < player_right))){
                    
                    if (enemies[i].y > 0){

                        potential_hit = true;

                    }
                    
                    int enemies_top = enemies[i].y;
                    int enemies_bottom = enemies[i].y + enemy_height;
                    int player_top = player_y;
                    int player_bottom = player_y + player_height;

                    if (((enemies_bottom > player_top) && (enemies_top < player_bottom))){
                        return current_score;
                    }
                }
                

                enemies[i].y += enemies[i].speed;

                if (enemies[i].y > display_height){
                    enemies[i].active = false;
                    current_score += points_per_miss;

                    spawn_enemies(1, enemy_width, enemy_height, 1, enemies);

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



