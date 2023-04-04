file(GLOB LIB_SOURCES "*.cc")

add_library(util_debug STATIC ${LIB_SOURCES})

add_library(util_sanitized EXCLUDE_FROM_ALL STATIC ${LIB_SOURCES})
target_compile_options(util_sanitized PUBLIC ${SANITIZING_FLAGS})

add_library(util_optimized EXCLUDE_FROM_ALL STATIC ${LIB_SOURCES})
target_compile_options(util_optimized PUBLIC "-O2")
