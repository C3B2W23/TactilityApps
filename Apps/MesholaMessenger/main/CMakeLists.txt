file(GLOB_RECURSE SOURCE_FILES
    Source/*.cpp
    Source/*.c
)

idf_component_register(
    SRCS ${SOURCE_FILES}
    INCLUDE_DIRS 
        Source
        Source/protocol
        Source/profile
        Source/storage
        Source/views
    REQUIRES 
        TactilitySDK
)

# TODO: Add RadioLib and MeshCore library integration
