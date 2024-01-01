set(SOURCES espThermostat.cpp)
if(DEFINED PROJECT_NAME)
idf_component_register(SRCS ${SOURCES}
INCLUDE_DIRS include
REQUIRES esp_wifi nvs_flash esp_event esp_netif lwip espWifi mqtt espThermostat)
else()
cmake_minimum_required(VERSION 3.16)
set(CMAKE_CXX_STANDARD 17)
set(EXTRA_COMPONENT_DIRS include)
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(espThermostat)
endif()
