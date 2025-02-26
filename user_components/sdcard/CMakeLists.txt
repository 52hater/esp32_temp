idf_component_register(
    SRCS "sdcard.c"
    INCLUDE_DIRS "inc"
    REQUIRES fatfs sd_card main sqlite
    WHOLE_ARCHIVE)
