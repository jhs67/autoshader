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
option(AUTOSHADER_BuildTools "Build the autoshader tool" ON)

include(cmake/autoshader.cmake)

project(autoshader VERSION 0.2.0)

if(AUTOSHADER_BuildTools)
	find_package(spirv_cross_core CONFIG REQUIRED)
	find_package(spirv_cross_reflect CONFIG REQUIRED)
	find_package(cxxopts CONFIG REQUIRED)
	find_package(fmt CONFIG REQUIRED)
endif()

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
	source/specializer.cpp
	source/specializer.h
	source/typereflect.cpp
	source/typereflect.h
	source/vertexinput.cpp
	source/vertexinput.h
)

set(includes
	include/autoshader/anyarg.h
	include/autoshader/createpipe.h
	include/autoshader/pipeline.h
)

if(AUTOSHADER_BuildTools)
	add_executable(${PROJECT_NAME} ${sources})
	target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_14)
	target_link_libraries(${PROJECT_NAME} PRIVATE spirv-cross-reflect)
	target_link_libraries(${PROJECT_NAME} PRIVATE spirv-cross-core)
	target_link_libraries(${PROJECT_NAME} PRIVATE fmt::fmt)
	target_link_libraries(${PROJECT_NAME} PRIVATE cxxopts::cxxopts)

	if(AUTOSHADER_WarnAsError AND NOT MSVC)
		target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Werror)
	endif()
endif()

add_library(${PROJECT_NAME}-lib INTERFACE)
target_compile_features(${PROJECT_NAME}-lib INTERFACE cxx_std_14)
target_include_directories(${PROJECT_NAME}-lib INTERFACE
	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include> $<INSTALL_INTERFACE:include>)

include(CTest)

if(BUILD_TESTING AND AUTOSHADER_BuildTools AND AUTOSHADER_BuildTests)
	enable_testing()
	add_subdirectory(test)
endif()

# install
include(CMakePackageConfigHelpers)
include(GNUInstallDirs)

# install the targets
if(AUTOSHADER_BuildTools)
	install(TARGETS ${PROJECT_NAME} EXPORT ${PROJECT_NAME}-export)
endif()

install(TARGETS ${PROJECT_NAME}-lib EXPORT ${PROJECT_NAME}-export)

# install the headers
install(FILES ${includes} DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/autoshader")

# install the cmake config files
set(config_install_dir "${CMAKE_INSTALL_DATADIR}/${PROJECT_NAME}")

# create a version file
write_basic_package_version_file(
	autoshader-config-version.cmake
	VERSION ${PACKAGE_VERSION}
	COMPATIBILITY AnyNewerVersion
)

# create config template file
file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/autoshader-config.cmake.in" [=[
@PACKAGE_INIT@
include("@PACKAGE_PATH_EXPORT_TARGETS@")
]=])

# create config file
set(PATH_EXPORT_TARGETS "${config_install_dir}/autoshader-targets.cmake")
configure_package_config_file(
	"${CMAKE_CURRENT_BINARY_DIR}/autoshader-config.cmake.in"
	"${CMAKE_CURRENT_BINARY_DIR}/autoshader-config.cmake"
	PATH_VARS PATH_EXPORT_TARGETS
	INSTALL_DESTINATION "${config_install_dir}"
)

# install the cmake files
install(FILES
	"${CMAKE_CURRENT_BINARY_DIR}/autoshader-config-version.cmake"
	"${CMAKE_CURRENT_BINARY_DIR}/autoshader-config.cmake"
	DESTINATION "${config_install_dir}")

# export the targets
install(EXPORT ${PROJECT_NAME}-export
	FILE autoshader-targets.cmake
	NAMESPACE autoshader::
	DESTINATION "${config_install_dir}")
