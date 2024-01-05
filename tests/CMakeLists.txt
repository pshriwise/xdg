

set(
TEST_NAMES
test_bbox
test_bvh
test_closest
test_ray_fire
test_mesh_internal
test_xdg_interface
test_moab
)

foreach(test ${TEST_NAMES})
    add_executable(${test} ${test}.cpp)
    target_link_libraries(${test} xdg Catch2::Catch2WithMain)
    add_test(NAME ${test} COMMAND ${test} WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
endforeach()

set(
TEST_FILES
pwr_pincell.h5m
cube.h5m
)

foreach(file ${TEST_FILES})
    ADD_CUSTOM_TARGET(${file} ALL
                      COMMAND ${CMAKE_COMMAND} -E create_symlink
                      "${CMAKE_CURRENT_SOURCE_DIR}/test_files/${file}"
                      "${CMAKE_CURRENT_BINARY_DIR}/${file}")
endforeach()
