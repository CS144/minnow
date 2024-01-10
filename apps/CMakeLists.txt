add_library (stream_copy STATIC bidirectional_stream_copy.cc)

macro(add_app exec_name)
  add_executable("${exec_name}" "${exec_name}.cc")
  target_link_libraries("${exec_name}" stream_copy)
  target_link_libraries("${exec_name}" minnow_debug)
  target_link_libraries("${exec_name}" util_debug)
endmacro(add_app)

add_app(webget)
add_app(tcp_native)
