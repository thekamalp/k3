add_executable (onetri onetri.cpp ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/simple_vs.cso ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/simple_ps.cso)
add_executable (raytri raytri.cpp ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/simple_ray.cso ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/simple3d_vs.cso ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/simple3d_ps.cso)
add_executable (gradtest gradtest.cpp ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/color_vs.cso ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/color_ps.cso)
add_executable (animtest animtest.cpp ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/anim3d_vs.cso ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/anim3d_ps.cso)

function(add_hlsl_shader SHADER_TYPE INPUT_FILE OUTPUT_FILE ENTRY)
	add_custom_command(
		OUTPUT ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${OUTPUT_FILE}
		COMMAND dxc -E ${ENTRY} -T \"${SHADER_TYPE}\" -Fo ${OUTPUT_FILE} ../test/${INPUT_FILE}
		MAIN_DEPENDENCY ${INPUT_FILE}
		WORKING_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
	)
endfunction()

add_hlsl_shader(vs_5_1 "shaders/simple_vs.hlsl" "simple_vs.cso" "main")
add_hlsl_shader(ps_5_1 "shaders/simple_ps.hlsl" "simple_ps.cso" "main")
add_hlsl_shader(lib_6_3 "shaders/simple_ray.hlsl" "simple_ray.cso" "main")
add_hlsl_shader(vs_5_1 "shaders/simple3d.hlsl" "simple3d_vs.cso" "vs_main")
add_hlsl_shader(ps_5_1 "shaders/simple3d.hlsl" "simple3d_ps.cso" "ps_main")
add_hlsl_shader(vs_5_1 "shaders/color.hlsl" "color_vs.cso" "vs_main")
add_hlsl_shader(ps_5_1 "shaders/color.hlsl" "color_ps.cso" "ps_main")
add_hlsl_shader(vs_5_1 "shaders/anim3d.hlsl" "anim3d_vs.cso" "vs_main")
add_hlsl_shader(ps_5_1 "shaders/anim3d.hlsl" "anim3d_ps.cso" "ps_main")

target_link_libraries(onetri ${PROJ})
target_link_libraries(raytri ${PROJ})
target_link_libraries(gradtest ${PROJ})
target_link_libraries(animtest ${PROJ})

enable_testing()
add_test(onetri onetri)
add_test(raytri raytri)
