file(GLOB_RECURSE SOURCE_FILES
    Source/*.cpp
    Source/*.c
    Source/compat/*.c
)

# Pull in RadioLib from the T-Deck library bundle
file(GLOB_RECURSE RADIOLIB_SOURCES
    ${CMAKE_CURRENT_LIST_DIR}/../../../../T-Deck/lib/RadioLib/src/*.cpp
)
list(APPEND SOURCE_FILES ${RADIOLIB_SOURCES})

# Bundled minmea (NMEA parser) used by Tactility services
list(APPEND SOURCE_FILES ${CMAKE_CURRENT_LIST_DIR}/../../../../Tactility/Libraries/minmea/Source/minmea.c)

idf_component_register(
    SRCS ${SOURCE_FILES}
    INCLUDE_DIRS 
        Source
        Source/protocol
        Source/profile
        Source/storage
        Source/views
        Source/compat
        ${TACTILITY_SDK_PATH}/Libraries/TactilityCpp/Include
        ${CMAKE_CURRENT_LIST_DIR}/../../../../T-Deck/lib/RadioLib/src
        ${CMAKE_CURRENT_LIST_DIR}/../../../../Tactility/Libraries/minmea/Include
    REQUIRES 
        TactilitySDK
        esp_timer
        driver
        esp_wifi
        esp_netif
        esp_event
        json
        esp_http_server
        esp_http_client
        freertos
        fatfs
)

# Link against C++ Tactility libraries packaged in the SDK
target_link_libraries(${COMPONENT_LIB} INTERFACE
    ${TACTILITY_SDK_PATH}/Libraries/TactilityCpp/Binary/libTactility.a
    ${TACTILITY_SDK_PATH}/Libraries/TactilityCpp/Binary/libTactilityCore.a
    ${TACTILITY_SDK_PATH}/Libraries/TactilityC/Binary/libTactilityC.a
    ${TACTILITY_SDK_PATH}/Libraries/lvgl/Binary/liblvgl.a
)

# TODO: Add RadioLib and MeshCore library integration
