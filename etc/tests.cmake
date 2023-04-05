include(CTest)

list(APPEND CMAKE_CTEST_ARGUMENTS --output-on-failure --stop-on-failure --timeout 10 -E 'speed_test|optimization')

set(compile_name "compile with bug-checkers")
add_test(NAME ${compile_name}
  COMMAND "${CMAKE_COMMAND}" --build "${CMAKE_BINARY_DIR}" -t functionality_testing webget)

macro (ttest name)
  add_test(NAME ${name} COMMAND "${name}_sanitized")
  set_property(TEST ${name} PROPERTY FIXTURES_REQUIRED compile)
endmacro (ttest)

set_property(TEST ${compile_name} PROPERTY TIMEOUT -1)
set_tests_properties(${compile_name} PROPERTIES FIXTURES_SETUP compile)

add_test(NAME t_webget COMMAND "${PROJECT_SOURCE_DIR}/tests/webget_t.sh" "${PROJECT_BINARY_DIR}")
set_property(TEST t_webget PROPERTY FIXTURES_REQUIRED compile)

ttest(byte_stream_basics)
ttest(byte_stream_capacity)
ttest(byte_stream_one_write)
ttest(byte_stream_two_writes)
ttest(byte_stream_many_writes)
ttest(byte_stream_stress_test)

add_custom_target (check0 COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure --stop-on-failure --timeout 12 -R 'webget|^byte_stream_')

add_custom_target (check_webget COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure --timeout 12 -R 'webget')

###

add_custom_target (speed COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure --timeout 12 -R '_speed_test')

set(compile_name_opt "compile with optimization")
add_test(NAME ${compile_name_opt}
  COMMAND "${CMAKE_COMMAND}" --build "${CMAKE_BINARY_DIR}" -t speed_testing)

macro (stest name)
  add_test(NAME ${name} COMMAND ${name})
  set_property(TEST ${name} PROPERTY FIXTURES_REQUIRED compile_opt)
endmacro (stest)

set_property(TEST ${compile_name_opt} PROPERTY TIMEOUT -1)
set_tests_properties(${compile_name_opt} PROPERTIES FIXTURES_SETUP compile_opt)

stest(byte_stream_speed_test)

