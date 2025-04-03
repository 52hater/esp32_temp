set(PROJECT_PATH "D:/Desktop/esp32_temp")

# The following lines of boilerplate have to be in your project's CMakeLists
# in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.16)

# user_components 디렉토리 내부를 컴포넌트 디렉토리로 사용
set(EXTRA_COMPONENT_DIRS "user_components")

set(COMPONENTS main sqlite sdcard ds18b20 websocket ble config)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(esp32_temp)
