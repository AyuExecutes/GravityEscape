
#include <esp_timer.h>
#include <rom/ets_sys.h>

#include "graphics3d.h"

#define maxval(x,y) (((x) >= (y)) ? (x) : (y))

typedef enum {
    NO_KEY,
    LEFT_DOWN,
    LEFT_UP,
    RIGHT_DOWN,
    RIGHT_UP
} key_type;

int read_touch(int t);
void input_output_init(); 
key_type get_input();
vec2 get_touchpads();

int storage_read_int(char *name, int def);
void storage_write_int(char *name, int val);
void storage_read_string(char *name, char *def, char *dest, int len);
void storage_write_string(char *name, char *val);
void edit_stored_string(char *name, char *prompt);
void showfps();