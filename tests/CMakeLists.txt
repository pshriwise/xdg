

set(
TEST_NAMES

test_initialize
test_bbox
test_bvh
)


ADD_CUSTOM_TARGET(link_test_file ALL
                  COMMAND ${CMAKE_COMMAND} -E create_symlink
                  "${CMAKE_CURRENT_SOURCE_DIR}/test_files/pwr_pincell.h5m"
                  "${CMAKE_CURRENT_BINARY_DIR}/pwr_pincell.h5m")

foreach(test ${TEST_NAMES})
    add_executable(${test} ${test}.cpp)
    target_link_libraries(${test} libxdg Catch2::Catch2WithMain)
    add_test(NAME ${test} COMMAND ${test} WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
endforeach()
