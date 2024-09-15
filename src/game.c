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
                enemies[j].y = -(enemy_height * (i + 1));
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
    int64_t last_spawn_time = esp_timer_get_time();
    int64_t last_difficulty_time = esp_timer_get_time();

    char string_buffer[256];

    int const player_speed = 2;
    int const player_width = display_width * 0.2;
    int const player_height = display_height * 0.05;
    int const difficulty_interval = 15000;
    int const max_difficulty = 5;

    int const enemy_height = display_height * 0.1;
    int const enemy_width = display_width * 0.05;

    int current_spawn_rate = 3500;
    int current_spawn_count = 1;
    int current_spawn_speed = 1;

    int current_difficulty_level = 1;
    
    int current_score = 0;
    int player_x = (display_width / 2) - (player_width / 2);
    int player_y = display_height - player_height;

    bool right_down = false, left_down = false;

    struct enemy_block enemies[maximum_enemies];

    // initialize enemies to inactive
    for (int i = 0; i < maximum_enemies; i++){
        enemies[i].active = false;
    }

    spawn_enemies(current_spawn_count, enemy_width, enemy_height, current_spawn_speed, enemies);

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

        if (potential_hit){
            // draw borders of warning left and right
            draw_rectangle(0, 0, 2, display_height, rgbToColour(255, 128, 0));
            draw_rectangle(display_width - 2, 0, display_width, display_height, rgbToColour(255, 128, 0));
        }
        
        snprintf(string_buffer, 64, "Score: %d", current_score);
        print_xy(string_buffer, 0, 0);

        sniprintf(string_buffer, 64, "Lvl:");
        print_xy(string_buffer, display_width / 2, 0);

        for (int i = 0; i < current_difficulty_level ; i++){
            draw_rectangle((display_width / 2) + (i * 9) + 20, 0, 6, 10, rgbToColour(0, 255, 255));
        }

        flip_frame();

        current_time = esp_timer_get_time();
        if ((frame++ % 10) == 0) {
            printf("FPS:%f %d %d\n", 1.0e6 / (current_time - last_time),
                heap_caps_get_free_size(MALLOC_CAP_DMA),
                heap_caps_get_free_size(MALLOC_CAP_32BIT));
            
            vTaskDelay(1);
        }

        if ((current_time - last_spawn_time) / 1000 > current_spawn_rate){
            spawn_enemies(current_spawn_count, enemy_width, enemy_height, current_spawn_speed, enemies);
            last_spawn_time = current_time;
        }

        if ((current_time - last_difficulty_time) / 1000 > difficulty_interval){
            last_difficulty_time = current_time;
            current_difficulty_level += 1;

            if (current_difficulty_level > max_difficulty){
                current_difficulty_level = max_difficulty;
            }

            switch (current_difficulty_level){
                
                case 1: 
                    current_spawn_rate = 3500;
                    current_spawn_count = 1;
                    current_spawn_speed = 1;
                    break;

                case 2:
                    current_spawn_rate = 3000;
                    current_spawn_count = 2;
                    current_spawn_speed = 1;
                    break;

                case 3:
                    current_spawn_rate = 3000;
                    current_spawn_count = 3;
                    current_spawn_speed = 1;
                    break;

                case 4:
                    current_spawn_rate = 2500;
                    current_spawn_count = 3;
                    current_spawn_speed = 2;
                    break;

                case 5:
                    current_spawn_rate = 2000;
                    current_spawn_count = 4;
                    current_spawn_speed = 2;
                    break;

                default:
                    break;
            }
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




