#include "hw.h"

void hw_init(void){
    ds18b20_init();
    sdcard_init();
}