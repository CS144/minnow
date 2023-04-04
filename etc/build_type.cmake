set(default_build_type "Debug")

if (NOT (CMAKE_BUILD_TYPE_SHADOW STREQUAL CMAKE_BUILD_TYPE))
    if (NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
        message (STATUS "Setting build type to '${default_build_type}'")
        set (CMAKE_BUILD_TYPE "${default_build_type}" CACHE STRING "Choose the type of build." FORCE)
    endif ()
    set (CMAKE_BUILD_TYPE_SHADOW ${CMAKE_BUILD_TYPE} CACHE STRING "used to detect changes in build type" FORCE)
endif ()

message (STATUS "Building in '${CMAKE_BUILD_TYPE}' mode.")
