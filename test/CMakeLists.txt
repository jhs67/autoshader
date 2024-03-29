#
# Copyright(c) 2018 Jon Spencer.
# See LICENSE file.
#

cmake_minimum_required(VERSION 3.12)

# compile a shader into a spirv file
function(autoshader_test_compile_spirv_shader source)
  cmake_parse_arguments(arg "" "OUTPUT" "DEPENDS" "${ARGN}")

  # figure out the output name
  set(out_name ${source}.spv)
  if(arg_OUTPUT)
    set(out_name ${arg_OUTPUT})
  endif()

  # create the output directory
  get_filename_component(out_dir ${out_name} DIRECTORY)
  if(out_dir)
    get_filename_component(out_dir "${out_dir}" REALPATH BASE_DIR "${CMAKE_CURRENT_BINARY_DIR}")
    file(MAKE_DIRECTORY ${out_dir})
  endif()

  find_program(GLSLANGVALIDATOR_PROGRAM glslangValidator HINTS $ENV{VULKAN_SDK}/bin)

  # invoke glslang to compile the shader
  add_custom_command(
    OUTPUT ${out_name}
    COMMAND ${GLSLANGVALIDATOR_PROGRAM} -o "${CMAKE_CURRENT_BINARY_DIR}/${out_name}"
      -V "${CMAKE_CURRENT_SOURCE_DIR}/${source}"
    DEPENDS ${source} ${arg_DEPENDS}
    VERBATIM
    )

endfunction()

# compile spirv shaders into a autoshader pipeline
function(compile_autoshader out_name)
  cmake_parse_arguments(arg "" "" "DEPENDS" "${ARGN}")  # figure out the output name

  # get the sources
  set(sources ${arg_UNPARSED_ARGUMENTS})

  # create the output directory
  get_filename_component(out_dir ${out_name} DIRECTORY)
  if(out_dir)
    get_filename_component(out_dir "${out_dir}" REALPATH BASE_DIR "${CMAKE_CURRENT_BINARY_DIR}")
    file(MAKE_DIRECTORY ${out_dir})
  endif()

  # invoke autoshader to compile the shader
  add_custom_command(
    OUTPUT ${out_name}
    COMMAND autoshader -o "${CMAKE_CURRENT_BINARY_DIR}/${out_name}" ${sources}
    DEPENDS ${sources} ${arg_DEPENDS} autoshader
    VERBATIM
    )

endfunction()

project(autoshader-test)

find_package(Vulkan REQUIRED)
find_package(Catch2 CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)

add_custom_target(autoshader-test)

set(files
  struct-align.cpp
  name-map.cpp
  descriptor-set-layout.cpp
  vertex-input.cpp
  any-arg.cpp
  push-ranges.cpp
  array-descriptor.cpp
  )

if(AUTOSHADER_VulkanTests)
  list(APPEND files
  create-pipe.cpp
    )
endif()

set(shaders
  struct-align.comp
  name-map.vert
  name-map.frag
  descriptor-set-layout.vert
  descriptor-set-layout.frag
  vertex-input.vert
  create-pipe.vert
  create-pipe.frag
  push-ranges.vert
  push-ranges.frag
  array-descriptor.comp
  )

# compile the shaders to spirv
foreach(shader ${shaders})
  autoshader_test_compile_spirv_shader(${shader})
endforeach(shader)

foreach(file ${files})

  # get a name for the test executable
  get_filename_component(basename ${file} NAME_WE)
  set(testcase ${basename}-test)
  set(test_files ${file})

  # find the shaders for this target
  set(test_shaders ${shaders})
  list(FILTER test_shaders INCLUDE REGEX "${basename}\\..*")
  if(test_shaders)
    set(test_spirv ${test_shaders})
    list(TRANSFORM test_spirv APPEND ".spv")
    autoshader(OUTPUT "${basename}-autoshader.h" SHADERS ${test_spirv})
    list(APPEND test_files "${basename}-autoshader.h")
  endif()

  # create the test executable
  add_executable(${testcase} ${test_files})
  target_link_libraries(${testcase} PRIVATE autoshader-lib)
  target_link_libraries(${testcase} PRIVATE Catch2::Catch2WithMain)
  target_link_libraries(${testcase} PRIVATE Vulkan::Vulkan)
  target_include_directories(${testcase} PRIVATE ${CMAKE_CURRENT_BINARY_DIR})

  if( AUTOSHADER_WarnAsError AND NOT MSVC )
  	target_compile_options(${testcase} PRIVATE -Wall -Werror)
  endif()

  # setup the test
  add_dependencies(autoshader-test ${testcase})
  add_test(NAME test-${basename} COMMAND ${testcase})
  set_tests_properties(test-${basename} PROPERTIES DEPENDS ${testcase})

endforeach()
