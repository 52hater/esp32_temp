#ifndef SDCARD_H
#define SDCARD_H

#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"
#include "sqlite3.h"
#include "sqllib.h"
#include <string.h>
#include <sys/stat.h>
#include <sys/unistd.h>

#define PIN_NUM_MISO 13
#define PIN_NUM_MOSI 11
#define PIN_NUM_CLK  12
#define PIN_NUM_CS   10

void sdcard_init(void);
const char* get_mount_point(void);

#endif