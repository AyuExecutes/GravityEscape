
#include <esp_timer.h>
#include <rom/ets_sys.h>

int render_menu(char * title, int nentries, char *entries[], int select);
void get_string(char *title, char *original, int len);