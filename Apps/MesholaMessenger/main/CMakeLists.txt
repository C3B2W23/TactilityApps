file(GLOB_RECURSE SOURCE_FILES
    Source/*.cpp
    Source/*.c
)
file(GLOB COMPAT_SOURCES
    Source/compat/*.c
)

# Pull in RadioLib from the T-Deck library bundle
file(GLOB_RECURSE RADIOLIB_SOURCES
    ${CMAKE_CURRENT_LIST_DIR}/../../../../T-Deck/lib/RadioLib/src/*.cpp
)
list(APPEND SOURCE_FILES ${RADIOLIB_SOURCES})

# Bundled minmea (NMEA parser) used by Tactility services
list(APPEND SOURCE_FILES ${CMAKE_CURRENT_LIST_DIR}/../../../../Tactility/Libraries/minmea/Source/minmea.c)

set(MESHOLA_INCLUDE_DIRS
    Source
    Source/protocol
    Source/profile
    Source/storage
    Source/views
    Source/compat
    ${CMAKE_CURRENT_LIST_DIR}/../../../../T-Deck/lib/RadioLib/src
    ${CMAKE_CURRENT_LIST_DIR}/../../../../Tactility/Libraries/minmea/Include
)

set(MESHOLA_REQUIRE_DEPS
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

if (DEFINED MESHOLA_MESSENGER_EMBED)
    # Building as a firmware-integrated component
    # Do not include compat stubs when linking inside firmware (real symbols exist)
    list(REMOVE_ITEM SOURCE_FILES ${COMPAT_SOURCES})
    list(APPEND MESHOLA_REQUIRE_DEPS
        Tactility
        TactilityCore
        TactilityC
        lvgl
    )
    idf_component_register(
        SRCS ${SOURCE_FILES}
        INCLUDE_DIRS ${MESHOLA_INCLUDE_DIRS}
        REQUIRES ${MESHOLA_REQUIRE_DEPS}
    )
    target_compile_definitions(${COMPONENT_LIB} PRIVATE MESHOLA_MESSENGER_EMBED=1)
else ()
    # Standalone app build using the packaged SDK
    list(APPEND SOURCE_FILES ${COMPAT_SOURCES})
    list(APPEND MESHOLA_INCLUDE_DIRS
        ${TACTILITY_SDK_PATH}/Libraries/TactilityCpp/Include
    )
    list(APPEND MESHOLA_REQUIRE_DEPS
        TactilitySDK
    )
    idf_component_register(
        SRCS ${SOURCE_FILES}
        INCLUDE_DIRS ${MESHOLA_INCLUDE_DIRS}
        REQUIRES ${MESHOLA_REQUIRE_DEPS}
    )
    # Link against C++ Tactility libraries packaged in the SDK
    target_link_libraries(${COMPONENT_LIB} INTERFACE
        ${TACTILITY_SDK_PATH}/Libraries/TactilityCpp/Binary/libTactility.a
        ${TACTILITY_SDK_PATH}/Libraries/TactilityCpp/Binary/libTactilityCore.a
        ${TACTILITY_SDK_PATH}/Libraries/TactilityC/Binary/libTactilityC.a
        ${TACTILITY_SDK_PATH}/Libraries/lvgl/Binary/liblvgl.a
    )
endif ()

# TODO: Add RadioLib and MeshCore library integration
