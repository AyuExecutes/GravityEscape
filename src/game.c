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

// Struct to hold each enemy block
struct enemy_block {
    float x;
    float y;
    float speed;
    bool active;    
    int colour;     // current colour for the channel (in this case the red channel)
    int colour_step; // the amount that colour changes on each frame
};

// Game constants
int const maximum_enemies = 12;
int const enemy_colour_step = 15;
int const points_per_miss = 100;

// Spawn enemies from the top of the screen
void spawn_enemies(int number_of_enemies, int enemy_width, int enemy_height, float enemy_speed, struct enemy_block enemies[]){

    for (int i = 0; i < number_of_enemies; i++) {

        // look for the free slot in the enemies array
        for (int j = 0; j < maximum_enemies; j++){
            
            // check if the enemy is inactive which means the slot is available
            if (enemies[j].active == false){
                
                // make sure enemy is spawned within the screen
                enemies[j].x = rand() % (display_width - enemy_width);

                // make sure enemies are coming staggered from the top so they do not come together in a line
                enemies[j].y = -(enemy_height * (i + 1));

                // set the speed, colour, and colour step
                enemies[j].speed = enemy_speed;
                enemies[j].colour = 102;
                enemies[j].colour_step = enemy_colour_step;

                // mark the enemy as active / slot is unavailable
                enemies[j].active = true;

                break;

            }
        }
    }
}

// Game starts here
int render_game() {

    // for fps calculation and timing
    int64_t current_time;
    int64_t last_time = esp_timer_get_time();
    int64_t last_spawn_time = 0;
    int64_t last_difficulty_time = esp_timer_get_time();

    // allocate memory for the string buffer
    char string_buffer[256];

    int const player_speed = 2;                         // player speed in pixels
    int const player_width = display_width * 0.2;       // player width is 20% of the screen width
    int const player_height = display_height * 0.05;    // player height is 5% of the screen height
    int const difficulty_interval_ms = 15000;           // difficulty changes every 15 seconds
    int const max_difficulty = 5;                       // 5 levels of difficulty

    // set the enemy height and width to be 10% and 5% of the screen height and width respectively
    int const enemy_height = display_height * 0.1;
    int const enemy_width = display_width * 0.05;

    // initialisation of the difficulty parameters
    int current_spawn_rate = 0;
    int current_spawn_count = 0;
    int current_spawn_speed = 0;

    // start the game at difficulty level 1
    int current_difficulty_level = 1;

    // optimisation to avoid setting difficulty variable every frame
    bool difficulty_changed = true;
    
    // initialisation of current score, player position (in the middle of the screen) and height is on the bottom of the screen
    int current_score = 0;
    int player_x = (display_width / 2) - (player_width / 2);
    int player_y = display_height - player_height;

    // initialise buttons 
    bool right_down = false;
    bool left_down = false;

    // initialise the enemies array based on the maximum number of enemies
    struct enemy_block enemies[maximum_enemies];

    // initialize enemies to inactive
    for (int i = 0; i < maximum_enemies; i++){
        enemies[i].active = false;
    }

    int frame = 0; 

    // potential hit is used to show the warning on the left and right side of the screen as helper for the player (based on x positions)
    bool potential_hit = false;

    setFont(FONT_SMALL);
    setFontColour(255, 255, 255);

    while(1) {

        cls(rgbToColour(50, 50, 50));

        // draw the player
        draw_rectangle(player_x, player_y, player_width, player_height, rgbToColour(153, 255, 153));

        // draw the enemies
        for (int i = 0; i < maximum_enemies; i++){
            
            // only draw the enemies that are active 
            if (enemies[i].active == true){
                
                draw_rectangle(enemies[i].x, enemies[i].y, enemy_width, enemy_height, rgbToColour(enemies[i].colour, 0, 0));
            }
        }

        // if the x position is within the player's horizontal boundaries meaning potential hit is true then draw the warning lines on the left and right side of the screen
        if (potential_hit){

            // draw borders of warning left and right
            draw_rectangle(0, 0, 2, display_height, rgbToColour(255, 128, 0));
            draw_rectangle(display_width - 2, 0, display_width, display_height, rgbToColour(255, 128, 0));
        }
        
        // for the score and difficulty level
        snprintf(string_buffer, 64, "Score: %d", current_score);
        print_xy(string_buffer, 0, 1);

        sniprintf(string_buffer, 64, "Lvl:");
        print_xy(string_buffer, display_width / 2, 1);

        // draw the difficulty level as a bar on the screen right side
        for (int i = 0; i < current_difficulty_level ; i++){
            
            // each rectangle has width of 6 pixels and height of 10 pixels, and the area width is 9 pixels
            int spacing_between_rectangles = 9;
            int rectangle_width = 6;
            int rectangle_height = 10;
            int spacing_from_center = 20;

            draw_rectangle((display_width / 2) + (i * spacing_between_rectangles) + spacing_from_center, 0, rectangle_width, rectangle_height, rgbToColour(0, 255, 255));
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

        // keep track of the time then when it is pass the difficulty interval then increase the difficulty level
        if ((current_time - last_difficulty_time) / 1000 > difficulty_interval_ms){
            last_difficulty_time = current_time;
            current_difficulty_level += 1;
            difficulty_changed = true;

            // make sure the difficulty level does not go over the maximum difficulty level (5 will always be the maximum)
            if (current_difficulty_level > max_difficulty){
                current_difficulty_level = max_difficulty;
            }

        }

        // if the difficulty is changed either from the timer above or from the start of the game, set the new difficulty parameters
        // the higher the level, the faster the enemies spawn, the more enemies spawn, and the faster they move
        if (difficulty_changed){

            switch (current_difficulty_level){
                
                case 1: 
                    current_spawn_rate = 3500;
                    current_spawn_count = 1;
                    current_spawn_speed = 1;
                    break;

                case 2:
                    current_spawn_rate = 3000;
                    current_spawn_count = 2;
                    current_spawn_speed = 1.2;
                    break;

                case 3:
                    current_spawn_rate = 3000;
                    current_spawn_count = 3;
                    current_spawn_speed = 1.5;
                    break;

                case 4:
                    current_spawn_rate = 2500;
                    current_spawn_count = 3;
                    current_spawn_speed = 1.7;
                    break;

                case 5:
                    current_spawn_rate = 2000;
                    current_spawn_count = 4;
                    current_spawn_speed = 1.8;
                    break;

                default:
                    break;
            }

            // make sure difficulty changed is set to false so that it does not have to run this block of code every frame
            difficulty_changed = false;
        }

        // calculate the time that has passed since the last spawn, if its more than the spawn rate, then spawn the enemies
        if ((current_time - last_spawn_time) / 1000 > current_spawn_rate){

            spawn_enemies(current_spawn_count, enemy_width, enemy_height, current_spawn_speed, enemies);

            // remember the last time the enemies were spawned
            last_spawn_time = current_time;
        }

       

        key_type key = get_input();

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

        // if the left button is pressed, then the player x position is subtracted to move the player to the left
        if (left_down){
            
            player_x -= player_speed;

            // make sure the player does not go off the screen
            if (player_x < 0) {
                player_x = 0;
            }
        }

        // if the right button is pressed, then the player x position is added to move the player to the right
        if(right_down){
            
            player_x += player_speed;

            // make sure the player does not go off the screen
            if (player_x > display_width - player_width) {
                player_x = display_width - player_width;
            }
        }
        
        // initially we assume the player is not in danger
        potential_hit = false;

        // go through each enemy and make sure they are the ones that are active
        for (int i = 0; i < maximum_enemies; i++){

            if (enemies[i].active == true){
                
                // get the enemies and player left and right positions
                int enemies_left = enemies[i].x;
                int enemies_right = enemies[i].x + enemy_width;
                int player_left = player_x;
                int player_right = player_x + player_width;

                // detecting the horizontal collision
                if (((enemies_left > player_left) && (enemies_left < player_right)) || ((enemies_right > player_left) && (enemies_right < player_right))){
                    
                    // and also enemy is within the screen
                    if (enemies[i].y > 0){
                        
                        // meaning potential hit is true
                        potential_hit = true;

                    }
                    
                    // get the enemies and player top and bottom positions
                    int enemies_top = enemies[i].y;
                    int enemies_bottom = enemies[i].y + enemy_height;
                    int player_top = player_y;
                    int player_bottom = player_y + player_height;

                    // detecting the vertical collision (but also make sure that the enemies hasnt gone passed the player)
                    if (((enemies_bottom > player_top) && (enemies_top < player_bottom))){

                        // when conditions are true, then player is hit, then game over, return the current score to the main loop
                        return current_score;
                    }
                }
                
                // increase the y position of the enemy by the speed of the enemy
                enemies[i].y += enemies[i].speed;

                // so when the enemy is gone passed the screen, they now set to inactive (slot is available), then add score by 100 (points_per_miss)
                if (enemies[i].y > display_height){
                    enemies[i].active = false;
                    current_score += points_per_miss;

                } else {
                    
                    // otherwise, it is still on the screen, we cycle the colour by the colour step (bouncing affect)
                    enemies[i].colour += enemies[i].colour_step;
                    
                    // if the colour is reached 255, then set the colour to 255, reverse the step (negative)
                    if (enemies[i].colour > 255){
                        enemies[i].colour = 255;
                        enemies[i].colour_step = -enemy_colour_step;
                    } 

                    // when we get to the bottom of the colour range, then set the colour to 102, then reverse the step (positive)
                    if (enemies[i].colour < 102){
                        enemies[i].colour = 102;
                        enemies[i].colour_step = enemy_colour_step;
                    }

                }
            }
        }
        
    }
}




