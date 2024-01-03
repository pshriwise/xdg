

set(
TEST_NAMES

test_initialize
test_bbox
)

foreach(test ${TEST_NAMES})
    add_executable(${test} ${test}.cpp)
    target_link_libraries(${test} libxdg Catch2::Catch2WithMain)
    add_test(NAME ${test} COMMAND ${test} WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
endforeach()
