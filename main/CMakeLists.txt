set(SOURCES main.cpp
            ../espThermostat.cpp)
idf_component_register(SRCS ${SOURCES}
                       INCLUDE_DIRS ../include
                       REQUIRES driver esp_event espWifi mqtt lwip json)
