add_executable (onetri onetri.cpp ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/simple_vs.cso ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/simple_ps.cso)
add_executable (raytri raytri.cpp ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/simple_ray.cso)

function(add_hlsl_shader SHADER_TYPE INPUT_FILE OUTPUT_FILE)
	add_custom_command(
		OUTPUT ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${OUTPUT_FILE}
		COMMAND dxc -E \"main\" -T \"${SHADER_TYPE}\" -Fo ${OUTPUT_FILE} ../test/${INPUT_FILE}
		MAIN_DEPENDENCY ${INPUT_FILE}
		WORKING_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
	)
endfunction()

add_hlsl_shader(vs_5_0 "shaders/simple_vs.hlsl" "simple_vs.cso")
add_hlsl_shader(ps_5_0 "shaders/simple_ps.hlsl" "simple_ps.cso")
add_hlsl_shader(lib_6_3 "shaders/simple_ray.hlsl" "simple_ray.cso")

target_link_libraries(onetri ${PROJ})
target_link_libraries(raytri ${PROJ})

enable_testing()
add_test(onetri onetri)
add_test(raytri raytri)
