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

// Show the menu screen (welcome screen with options to start the game or view instructions)
int render_menu(char * title, int nentries, char *entries[], int select) {

    // for fps calculation
    int64_t current_time;
    int64_t last_time = esp_timer_get_time();
   
    int frame = 0; 

    // Prepare storage for high score and set default value to 0 to begin with
    int high_score = storage_read_int("high_score", 0);

    // Print the high score to the screen
    char high_score_str[64];
    snprintf(high_score_str, 64, "%d", high_score);

    while(1) {

        cls(rgbToColour(255, 204, 229));
        setFont(FONT_DEJAVU18);

        // For the selected item background
        draw_rectangle(0, (select * 25) + 57, display_width, 22, rgbToColour(204, 255, 153));
        
        // For the Welcome title
        setFontColour(0, 102, 51);
        print_xy("WELCOME", CENTER, 25);

        // For the choices in the menu
        setFontColour(204, 0, 102);
        setFont(FONT_UBUNTU16);

        for (int i = 0; i < nentries; i++) {
            print_xy(entries[i], CENTER, LASTY + ((i==0) ? 35 : 25)); 
        }  

        // For the High Score title
        setFontColour(0, 102, 51);
        setFont(FONT_UBUNTU16);
        print_xy("HIGH SCORE", CENTER, display_height - 100);

        // For the High Score value
        setFontColour(204, 0, 102);
        setFont(FONT_DEJAVU18);
        print_xy(high_score_str, CENTER, display_height - 75);

        // For the icons controls
        setFont(FONT_UBUNTU16);
        setFontColour(0, 102, 51);
        print_xy("\x86",4,display_height-16); // down arrow 
        print_xy("\x90",display_width-16,display_height-16); // OK

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
        if(key==LEFT_DOWN) select=(select+1)%nentries;
        if(key==RIGHT_DOWN) return select;
    }
}

const int ROWS=4;
const int COLS=12;
const int DEL_KEY = 0x7f;
const int SHIFT_KEY = 0x80;
const int ENTER_KEY = 0x81;
const char QWERTY_KEYS[2][48] = {"1234567890-=qwertyuiop[]asdfghjkl;'/\x80zxcvbnm,.\x7f\x81",
                        "!@#$%^&*()_+QWERTYUIOP{}ASDFGHJKL:\"?\x80ZXCVBNM<>\x7f\x81"};

void draw_keyboard(int topy, int highlight, int alt) {
    char str[2] = {0};
    draw_rectangle(0, topy, display_width, display_height - topy,
                   rgbToColour(32, 32, 42));
    for (int r = 0; r < ROWS; r++)
        for (int c = 0; c < COLS; c++) {
            int chi = r * COLS + c;
            int y = topy + (r * (display_height - topy)) / ROWS;
            int x = (c * display_width) / COLS;
            str[0] = QWERTY_KEYS[alt][chi];
            if (chi == highlight) {
                draw_rectangle(x, y, display_width / COLS,
                               (display_height - topy) / ROWS,
                               rgbToColour(120, 20, 20));
            }
            print_xy(str, x + 4, y + 4);
        }
}

void draw_controls(char *string, int sel) {
    setFontColour(0,0,0);
    char ch[2]={0};
    for(int i=0;i<strlen(string); i++) {
        int x=display_width-16*strlen(string)+i*16;
        ch[0]=string[i];
        if(i==sel)
            draw_rectangle(x,2,16,16,rgbToColour(255,255,255));
        print_xy(ch,x,2);
    }
}

void get_string(char *title, char *string, int len) {
    set_orientation(LANDSCAPE);
    int highlight=ROWS*COLS-1;
    int alt=0;
    int control=4;
    char controls[]="\x88\x89\x86\x87\x90"; // right,left,down,up,enter
    int key;
    do {
        cls(0);
        draw_rectangle(3,0,display_width,20,rgbToColour(220,220,0));
        setFontColour(0,0,0);
        setFont(FONT_DEJAVU18);
        print_xy(title,5,3);
        setFontColour(255,255,255);
        setFont(FONT_UBUNTU16);
        print_xy(string,5,24);
        draw_keyboard(48,highlight,alt);
        draw_controls(controls,control);
        flip_frame();
        vec2 tp=get_touchpads();
        key=get_input();
        if(key==LEFT_DOWN)
            control=(control+1)%strlen(controls);
        if(key==RIGHT_DOWN) {
            int key_val=QWERTY_KEYS[alt][highlight];
            switch(control) {
                case 0: tp.x=1; break;
                case 1: tp.x=-1; break;
                case 2: tp.y=1; break;
                case 3: tp.y=-1; break;
                case 4:
                if(key_val==DEL_KEY)
                    string[maxval((int)strlen(string)-1,0)]=0;
                else if(key_val==SHIFT_KEY)
                    alt=1-alt;
                else if(key_val==ENTER_KEY)
                    return;
                else if (strlen(string)<len-1)
                    string[strlen(string)]=key_val;
            }
        }
        highlight=(highlight+tp.x+tp.y*COLS+ROWS*COLS)%(ROWS*COLS);
    } while(true);
}
