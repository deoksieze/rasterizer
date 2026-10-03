add_executable(test_obj_loader test_obj_loader.cpp)
add_executable(test_rasterizer test_rasterizer.cpp)
add_executable(test_clipper test_clipper.cpp)
add_executable(test_golden test_golden.cpp)
add_executable(test_png_loader test_png_loader.cpp)

set(tests
  test_obj_loader
  test_rasterizer
  test_clipper
  test_golden
  test_png_loader
)

foreach(test IN LISTS tests)
  target_link_libraries(${test} PRIVATE rasterizer_lib PNG::PNG)
  target_compile_options(${test} PRIVATE -Wall -Wextra -Wpedantic)
  target_include_directories(${test} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

  add_test(NAME ${test} COMMAND ${test})
  set_tests_properties(${test} PROPERTIES
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
  )
endforeach()
