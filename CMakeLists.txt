# CMakeList.txt : Top-level CMake project file, do global configuration
# and include sub-projects here.
#
cmake_minimum_required (VERSION 3.8)

project (k3)

if(CMAKE_SIZEOF_VOID_P EQUAL 8)
	set(PROJ k3x64)
	set(FT_LIB freetype64)
else()
	set(PROJ k3)
	set(FT_LIB freetype32)
endif()

if(UNIX AND NOT APPLE)
	set(LINUX TRUE)
endif()

include_directories (include src/include)

set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS TRUE)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/bin)

file (GLOB SOURCE_IMAGE  "src/image/*.cpp")
file (GLOB SOURCE_MATH   "src/math/*.cpp")
file (GLOB SOURCE_COMMON "src/cmn/*.cpp")
file (GLOB SOURCE_GFX    "src/gfx/*.cpp")

if(WIN32)
	file (GLOB SOURCE_WIN32 "src/gfx/win32/*.cpp")
	file (GLOB SOURCE_DX12  "src/gfx/dx12/*.cpp")
	set (SOURCE_PLATFORM ${SOURCE_WIN32} ${SOURCE_DX12})
	set (GFX_LINK_LIB d3d12 dxgi d3dcompiler)
	set (LINK_LIB dsound hid dxguid winmm ${FT_LIB} ${GFX_LINK_LIB})
endif()

add_library(${PROJ} SHARED ${SOURCE_COMMON} ${SOURCE_IMAGE} ${SOURCE_MATH} ${SOURCE_GFX} ${SOURCE_PLATFORM})

target_link_libraries(${PROJ} ${LINK_LIB})

install(FILES ${PROJ}.lib DESTINATION lib)

add_subdirectory (test)
