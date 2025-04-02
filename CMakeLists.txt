cmake_minimum_required(VERSION 3.28)
project(vulkan_experiments VERSION 1.0)


### Require out-of-source builds
file(TO_CMAKE_PATH "${PROJECT_BINARY_DIR}/CMakeLists.txt" LOC_PATH)
if(EXISTS "${LOC_PATH}")
    message(FATAL_ERROR "You cannot build this in a source directory (or any directory with a CMakeLists.txt file). "
                        "Please make a build subdirectory. Feel free to remove CMakeCache.txt and CMakeFiles.")
endif()

### Build type
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  message(STATUS "Setting build type to 'RelWithDebInfo' as none was specified.")
  set(CMAKE_BUILD_TYPE "RelWithDebInfo" CACHE
      STRING "Choose the type of build." FORCE)
  # Set the possible values of build type for cmake-gui
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS
  "Debug" "Release" "RelWithDebInfo")
endif()


set(CMAKE_CXX_STANDARD 23)
set(CXX_STANDARD_REQUIRED ON)


set(target_name vulkan_experiments)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

if ( MSVC )
  set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/lib)
  set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/lib)
  set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/bin)

  set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/lib)
  set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/lib)
  set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/bin)

  set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_MINSIZEREL ${CMAKE_BINARY_DIR}/lib)
  set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_MINSIZEREL ${CMAKE_BINARY_DIR}/lib)
  set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL ${CMAKE_BINARY_DIR}/bin)

  set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_RELWITHDEBINFO ${CMAKE_BINARY_DIR}/lib)
  set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_RELWITHDEBINFO ${CMAKE_BINARY_DIR}/lib)
  set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO ${CMAKE_BINARY_DIR}/bin)
endif ( MSVC )

# build external project glslang to compile shaders
include(ExternalProject)
set(glslang_build_dir ${CMAKE_SOURCE_DIR}/build/glslang_build)
ExternalProject_Add(
	glslang
	PREFIX ${glslang_build_dir}
	TMP_DIR ${glslang_build_dir}/tmp
	STAMP_DIR ${glslang_build_dir}/stamp
	SOURCE_DIR ${CMAKE_SOURCE_DIR}/3rdparty/glslang
	BINARY_DIR ${glslang_build_dir}
	STEP_TARGETS glslang_update_sources
	BUILD_COMMAND ${CMAKE_COMMAND} --build <BINARY_DIR>
	CMAKE_ARGS
	-DCMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG=
	-DCMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE=
	-DCMAKE_RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL=
	-DCMAKE_RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO=
	INSTALL_COMMAND ""
	COMMENT "Building glslang to compile shaders"
)

find_package (Python COMPONENTS Interpreter)
ExternalProject_Add_Step(
	glslang glslang_update_sources
	WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/3rdparty/glslang
	COMMAND ${Python_EXECUTABLE} ${CMAKE_SOURCE_DIR}/3rdparty/glslang/update_glslang_sources.py
	LOG TRUE
	COMMENT "Updating glslang sources"
)

# these instructions search the directory tree when CMake is
# invoked and put all files that match the pattern in the variables
# `sources` and `data`
file(GLOB_RECURSE sources src/*.cpp)
file(GLOB_RECURSE data resources/*)

add_subdirectory(3rdparty/glm)
add_subdirectory(3rdparty/glfw)
add_subdirectory(3rdparty/tinyobjloader)

include_directories(SYSTEM 3rdparty/tinyobjloader)
include_directories(SYSTEM 3rdparty/glm)
include_directories(SYSTEM 3rdparty/imgui)


###############################################################################
## compile shaders ############################################################
###############################################################################
set(frag_shader_output ${CMAKE_BINARY_DIR}/bin/shaders/frag.spv)
set(vert_shader_output ${CMAKE_BINARY_DIR}/bin/shaders/vert.spv)
set(raytracing_shader_output ${CMAKE_BINARY_DIR}/bin/shaders/raytracing.spv)

set(raytracing_shader_rgen_output ${CMAKE_BINARY_DIR}/bin/shaders/shader.rgen.spv)
set(raytracing_shader_rchit_output ${CMAKE_BINARY_DIR}/bin/shaders/shader.rchit.spv)
set(raytracing_shader_rmiss_output ${CMAKE_BINARY_DIR}/bin/shaders/shader.rmiss.spv)
set(raytracing_shader_shadow_rmiss_output ${CMAKE_BINARY_DIR}/bin/shaders/shader_shadow.rmiss.spv)
set(raytracing_aabb_intersection_output ${CMAKE_BINARY_DIR}/bin/shaders/shader_aabb.rint.spv)

if ( MSVC )
	set(glslang_executable_path ${glslang_build_dir}/StandAlone/glslang.exe)
else()
	set(glslang_executable_path ${glslang_build_dir}/StandAlone/glslang)
endif ( MSVC )


# ensure the shaders output directory exists
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/bin/shaders)

add_custom_command(
  OUTPUT ${frag_shader_output} ${vert_shader_output}  ${raytracing_shader_rgen_output} ${raytracing_shader_rchit_output} ${raytracing_shader_rmiss_output} ${raytracing_shader_shadow_rmiss_output}
  COMMAND ${glslang_executable_path} --target-env vulkan1.3 -V "shaders/shader.frag" -o "${frag_shader_output}"
  COMMAND ${glslang_executable_path} --target-env vulkan1.3 -V "shaders/shader.vert" -o "${vert_shader_output}"
  COMMAND ${glslang_executable_path} --target-env vulkan1.3 -V "shaders/raytracing.rgen" -o "${raytracing_shader_output}"
  COMMAND ${glslang_executable_path} --target-env vulkan1.3 -V "shaders/shader.rgen" -o "${raytracing_shader_rgen_output}"
  COMMAND ${glslang_executable_path} --target-env vulkan1.3 -V "shaders/shader.rchit" -o "${raytracing_shader_rchit_output}"
  COMMAND ${glslang_executable_path} --target-env vulkan1.3 -V "shaders/shader.rmiss" -o "${raytracing_shader_rmiss_output}"
  COMMAND ${glslang_executable_path} --target-env vulkan1.3 -V "shaders/shader_shadow.rmiss" -o "${raytracing_shader_shadow_rmiss_output}"
  COMMAND ${glslang_executable_path} --target-env vulkan1.3 -V "shaders/shader_aabb.rint" -o "${raytracing_aabb_intersection_output}"
  DEPENDS glslang shaders/shader.frag shaders/shader.vert shaders/raytracing.rgen shaders/shader.rgen shaders/shader.rchit shaders/shader.rmiss shaders/shader_shadow.rmiss shaders/shader_aabb.rint
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  COMMENT "Compiling shader files"
)

# # Print all variables
# get_cmake_property(_variableNames VARIABLES)
# list (SORT _variableNames)
# foreach (_variableName ${_variableNames})
#     message(STATUS "${_variableName}=${${_variableName}}")
# endforeach()

set(3d_models_output  ${CMAKE_BINARY_DIR}/bin/3d-models)
cmake_path(GET 3d_models_output PARENT_PATH 3d_models_output_parent)
file(COPY "3d-models" DESTINATION ${3d_models_output_parent})

# add_custom_command(
#   OUTPUT ${3d_models_output}
#   COMMAND ${CMAKE_COMMAND} -E copy "3d-models/" ${3d_models_output}
#   DEPENDS 3d-models
#   WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
#   COMMENT "Copying 3D models to output directory"
# )

include_directories("include")

###############################################################################
## IMGUI  #####################################################################
###############################################################################
# Set where the ImGui files are stored
set(IMGUI_PATH "3rdparty/imgui")
file(GLOB IMGUI_SOURCES
    ${IMGUI_PATH}/imgui.h
    ${IMGUI_PATH}/imgui.cpp
    ${IMGUI_PATH}/imconfig.h
    ${IMGUI_PATH}/imgui_demo.cpp
    ${IMGUI_PATH}/imgui_draw.cpp
    ${IMGUI_PATH}/imgui_internal.h
    ${IMGUI_PATH}/imstb_rectpack.h
    ${IMGUI_PATH}/imstb_textedit.h
    ${IMGUI_PATH}/imstb_truetype.h
    ${IMGUI_PATH}/imgui_tables.cpp
    ${IMGUI_PATH}/imgui_widgets.cpp

    # bindings
    ${IMGUI_PATH}/backends/imgui_impl_glfw.h
    ${IMGUI_PATH}/backends/imgui_impl_glfw.cpp
    ${IMGUI_PATH}/backends/imgui_impl_vulkan.h
    ${IMGUI_PATH}/backends/imgui_impl_vulkan.cpp
)
add_library("imgui" STATIC ${IMGUI_SOURCES})

target_include_directories("imgui" SYSTEM PRIVATE ${IMGUI_PATH})
target_link_libraries("imgui" PRIVATE glfw)


###############################################################################
## target definitions #########################################################
###############################################################################

# add the data to the target, so it becomes visible in some IDE
add_executable(${target_name} ${sources}
    ${frag_shader_output}
    ${vert_shader_output}
    ${raytracing_shader_rgen_output}
    ${raytracing_shader_rchit_output}
    ${raytracing_shader_rmiss_output}
    ${raytracing_shader_shadow_rmiss_output}
)


if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
	# using Clang
	message("Using Clang")
	target_compile_options(${target_name} PRIVATE 
		-pedantic
		-pedantic-errors
		-Wall
		-Wconversion
		-Wshadow
		-Wfloat-conversion
		-Wextra
		-Weffc++
		-Werror
		-Wno-error=shadow
	)
elseif( CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
	# using GCC/G++
	target_compile_options(${target_name} PRIVATE
		-pedantic
		-pedantic-errors
		-Wall
		-Wextra
		-Wshadow
		-Wfloat-equal
		-Wundef
		-Wno-shadow
		-Wno-comment
		-Werror
	)
elseif( CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
	# using MSVC
	target_compile_options(${target_name} PRIVATE
		-W4
		-WX
		-wd4324
	)
else()
	message("Warning: Undefined compiler, setting no compiler flags")
endif()

# TODO: this is not working? we manually set -Werror
set_property(TARGET ${target_name} PROPERTY CMAKE_COMPILE_WARNING_AS_ERROR ON)

###############################################################################
## dependencies ###############################################################
###############################################################################
# source code
# target_include_directories(vulkan_experiments PUBLIC src/)

# Vulkan
find_package(Vulkan REQUIRED)
include_directories(SYSTEM ${Vulkan_INCLUDE_DIR})
target_include_directories(${target_name} SYSTEM PRIVATE Vulkan_INCLUDE_DIRS)

target_link_libraries(${target_name} PRIVATE
    # here you can add any library dependencies
  "glfw"
  "tinyobjloader"
  ${Vulkan_LIBRARY}
  "imgui"
)
