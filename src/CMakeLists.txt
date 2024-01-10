file(GLOB LIB_SOURCES "*.cc")

add_library(minnow_debug STATIC ${LIB_SOURCES})

add_library(minnow_sanitized EXCLUDE_FROM_ALL STATIC ${LIB_SOURCES})
target_compile_options(minnow_sanitized PUBLIC ${SANITIZING_FLAGS})

add_library(minnow_optimized EXCLUDE_FROM_ALL STATIC ${LIB_SOURCES})
target_compile_options(minnow_optimized PUBLIC "-O2")
