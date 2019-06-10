#
# Copyright(c) 2018 Jon Spencer.
# See LICENSE file.
#

cmake_minimum_required(VERSION 3.12)

# use pkgxx to build vcpkg dependencies
include(pkgxx/pkgxx.cmake OPTIONAL)

# Options
option(AUTOSHADER_BuildTests "Build the unit tests when BUILD_TESTING is enabled." ON)
option(AUTOSHADER_WarnAsError "Build the tests with warnings as errors." ON)
option(AUTOSHADER_VulkanTests "Build the unit tests that need a vulkan device to run." ON)

include(cmake/autoshader.cmake)

project(autoshader)

find_package(spirv_cross_core CONFIG REQUIRED)
find_package(spirv_cross_reflect CONFIG REQUIRED)
find_package(cxxopts CONFIG REQUIRED)
find_package(fmt CONFIG REQUIRED)

set(sources
	source/autoshader.cpp
	source/autoshader.h
	source/component.cpp
	source/component.h
	source/descriptorset.cpp
	source/descriptorset.h
	source/descriptorwrite.cpp
	source/descriptorwrite.h
	source/namemap.cpp
	source/namemap.h
	source/pushranges.cpp
	source/pushranges.h
	source/shadersource.cpp
	source/shadersource.h
	source/typereflect.cpp
	source/typereflect.h
	source/vertexinput.cpp
	source/vertexinput.h
	)

add_executable(${PROJECT_NAME} ${sources})
set_target_properties(${PROJECT_NAME} PROPERTIES CXX_STANDARD 14 CXX_STANDARD_REQUIRED ON)
target_link_libraries(${PROJECT_NAME} PRIVATE spirv-cross-reflect)
target_link_libraries(${PROJECT_NAME} PRIVATE spirv-cross-core)
target_link_libraries(${PROJECT_NAME} PRIVATE fmt::fmt)
target_link_libraries(${PROJECT_NAME} PRIVATE cxxopts::cxxopts)

if( AUTOSHADER_WarnAsError AND NOT MSVC )
	set_target_properties(${PROJECT_NAME} PROPERTIES COMPILE_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Werror")
endif()

add_library(${PROJECT_NAME}-lib INTERFACE)
target_include_directories(${PROJECT_NAME}-lib INTERFACE include)

include(CTest)

if(BUILD_TESTING AND AUTOSHADER_BuildTests)
	enable_testing()
	add_subdirectory(test)
endif()
