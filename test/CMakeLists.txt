add_executable (onetri onetri.cpp ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/simple_vs.cso ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/simple_ps.cso)

function(add_hlsl_shader SHADER_TYPE INPUT_FILE OUTPUT_FILE)
	add_custom_command(
		OUTPUT ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${OUTPUT_FILE}
		COMMAND fxc -E \"main\" -T \"${SHADER_TYPE}\" -Fo ${OUTPUT_FILE} ../test/${INPUT_FILE}
		MAIN_DEPENDENCY ${INPUT_FILE}
		WORKING_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
	)
endfunction()

add_hlsl_shader(vs_5_0 "shaders/simple_vs.hlsl" "simple_vs.cso")
add_hlsl_shader(ps_5_0 "shaders/simple_ps.hlsl" "simple_ps.cso")

target_link_libraries(onetri k3)

enable_testing()
add_test(onetri onetri)
