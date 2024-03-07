


set(TOOL_NAMES
particle_sim
ray_fire
find_volume
)

foreach(tool ${TOOL_NAMES})
    string(REPLACE "_" "-" tool_exec ${tool})
    add_executable(${tool_exec} ${tool}.cpp)
    target_link_libraries(${tool_exec} xdg argparse)
endforeach()