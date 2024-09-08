#include <driver/gpio.h>

#include <esp_system.h>

#include "input_output.h"
#include "fonts.h"
#include "graphics.h"
#include "drawing.h"

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

#ifdef TTGO_S3
#define SHOW_TOUCH_PADS
const int RIGHT_BUTTON=14;
const int TOUCH_PADS[4]={1,2,12,13};
#else
const int RIGHT_BUTTON=35;
const int TOUCH_PADS[4]={2,3,9,8};
#define SHOW_TOUCH_PADS
#endif

// for button inputs
QueueHandle_t inputQueue;
TimerHandle_t repeatTimer;
uint64_t lastkeytime=0;
int keyrepeat=1;

static int button_val[2]={1,1};

int read_touch(int t) {
    #ifdef TTGO_S3
    uint32_t touch_value;
    touch_pad_read_raw_data(t, &touch_value);
    //printf("touch %lu\n",touch_value);
    if(touch_value>30000) return 1;
    #else
    uint16_t touch_value;
    touch_pad_read(t, &touch_value);
    if(touch_value<1000) return 1;
    #endif
    
    return 0;
}

static void repeatTimerCallback(TimerHandle_t pxTimer) {
    int v;
    if(button_val[0]==0) {
        v=0;
        xQueueSendFromISR(inputQueue,&v,0);
    }
    if(button_val[1]==0) {
        v=RIGHT_BUTTON;
        xQueueSendFromISR(inputQueue,&v,0);
    }
    xTimerChangePeriod( repeatTimer, pdMS_TO_TICKS(200), 0);
    xTimerStart( repeatTimer, 0 );

}
// interrupt handler for button presses on GPIO0 and GPIO35
static void IRAM_ATTR gpio_isr_handler(void* arg)
{
    uint32_t gpio_num = (uint32_t) arg;
    int gpio_index=(gpio_num==RIGHT_BUTTON);
    int val=(1-button_val[gpio_index]);

    uint64_t time=esp_timer_get_time();
    uint64_t timesince=time-lastkeytime;
    // ets_printf("gpio_isr_handler %d %d %lld\n",gpio_num,val, timesince);
    // the buttons can be very bouncy so debounce by checking that it's been .5ms since the last
    // change
    if(timesince>500) {
        int v=gpio_num+val*100;
        xQueueSendFromISR(inputQueue,&v,0);
        if(val==0 && keyrepeat) {
            xTimerChangePeriod( repeatTimer, pdMS_TO_TICKS(400), 0);
            xTimerStart( repeatTimer, 0 );
        }
        if(val==1 && keyrepeat) {
            xTimerStop( repeatTimer, 0 );
        }
        lastkeytime=time;
    }
    button_val[gpio_index]=val;
    
    gpio_set_intr_type(gpio_num,val==0?GPIO_INTR_HIGH_LEVEL:GPIO_INTR_LOW_LEVEL);

}

// get a button press, returns a key_type value which can be
// NO_KEY if no buttons have been pressed since the last call.
key_type get_input() {
    int key;
    if(xQueueReceive(inputQueue,&key,0)==pdFALSE)
        return NO_KEY;
    switch(key) {
        case 0: return LEFT_DOWN;
        case RIGHT_BUTTON: return RIGHT_DOWN;
        case 100: return LEFT_UP;
        case 100+RIGHT_BUTTON: return RIGHT_UP;
    }
    return NO_KEY;
}

void input_output_init() {
     // queue for button presses
    inputQueue = xQueueCreate(4,4);
    repeatTimer = xTimerCreate("repeat", pdMS_TO_TICKS(300),pdFALSE,(void*)0, repeatTimerCallback);
    // interrupts for button presses
    gpio_set_direction(0, GPIO_MODE_INPUT);
    gpio_set_direction(RIGHT_BUTTON, GPIO_MODE_INPUT);
    gpio_set_intr_type(0, GPIO_INTR_LOW_LEVEL);
    gpio_set_intr_type(RIGHT_BUTTON, GPIO_INTR_LOW_LEVEL);
    gpio_install_isr_service(0);
    gpio_isr_handler_add(0, gpio_isr_handler, (void*) 0);
    gpio_isr_handler_add(RIGHT_BUTTON, gpio_isr_handler, (void*) RIGHT_BUTTON);
#ifdef SHOW_TOUCH_PADS
    touch_pad_init();
    
    
    for (int i = 0;i< 4;i++) {
        #ifdef TTGO_S3
        touch_pad_config(TOUCH_PADS[i]);
        #else
        touch_pad_set_fsm_mode(TOUCH_FSM_MODE_TIMER);
        touch_pad_set_voltage(TOUCH_HVOLT_2V7, TOUCH_LVOLT_0V5, TOUCH_HVOLT_ATTEN_1V);
        touch_pad_config(TOUCH_PADS[i],0);
        #endif
    }
    
    #ifdef TTGO_S3
    touch_pad_set_fsm_mode(TOUCH_FSM_MODE_TIMER);
    touch_pad_fsm_start();
    #endif
    
#endif
}

static uint16_t touch_values[4];
static uint64_t touch_time=0;
static uint64_t delay=400000;

vec2 get_touchpads() {
vec2 xy = {0, 0};
#ifdef SHOW_TOUCH_PADS
    //const int TOUCH_PADS[4] = {2, 3, 9, 8};
    
    uint64_t currenttime = esp_timer_get_time();
    uint64_t timesince = currenttime - touch_time;
    for (int i = 0; i < 4; i++) {
        uint32_t touch_value=read_touch(TOUCH_PADS[i]);
        if (touch_value &&
            (!touch_values[i] || timesince > delay)) {
            if ((i / 2) == 0) {
                if (i % 2)
                    xy.y++;
                else
                    xy.y--;
            } else {
                if (i % 2)
                    xy.x++;
                else
                    xy.x--;
            }
            if (touch_values[i] >= 1000)
                delay = 400000;
            else
                delay = 100000;
            touch_time = currenttime;
        }
        touch_values[i] = touch_value;
    }
#endif
    return xy;
}

nvs_handle_t storage_open(nvs_open_mode_t mode) {
    esp_err_t err;
    nvs_handle_t my_handle;
    err = nvs_open("storage", mode, &my_handle);
    if(err!=0) {
        nvs_flash_init();
        err = nvs_open("storage", mode, &my_handle);
        printf("err1: %d\n",err);
    }
    return my_handle;
}

int storage_read_int(char *name, int def) {
    nvs_handle_t handle=storage_open(NVS_READONLY);
    int32_t val=def;
    nvs_get_i32(handle, name, &val);
    nvs_close(handle);
    return val;
}

void storage_write_int(char *name, int val) {
    nvs_handle_t handle=storage_open(NVS_READWRITE);
    nvs_set_i32(handle, name, val);
    nvs_commit(handle);
    nvs_close(handle);
}


void storage_read_string(char *name, char *def, char *dest, int len) {
    nvs_handle_t handle=storage_open(NVS_READONLY);
    strncpy(dest,def,len);
    size_t length=len;
    nvs_get_str(handle, name, dest, &length);
    nvs_close(handle);
    printf("Read %s = %s\n",name,dest);
}

void storage_write_string(char *name, char *val) {
    nvs_handle_t handle=storage_open(NVS_READWRITE);
    nvs_set_str(handle, name, val);
    nvs_commit(handle);
    nvs_close(handle);
     printf("Wrote %s = %s\n",name,val);
}

void edit_stored_string(char *name, char *prompt) {
    char val[64];
    storage_read_string(name,"",val,sizeof(val));
    get_string(prompt,val,sizeof(val));
    storage_write_string(name,val);
}