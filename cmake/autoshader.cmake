#
# Copyright(c) 2018 Jon Spencer. See LICENSE file.

function(autoshader)
	cmake_parse_arguments(arg "" "OUTPUT;DATAFILE" "SHADERS;DEPENDS;EXTRA" "${ARGN}")

	# add in input arg for each source shader
	set(arglist "")
	foreach(src ${arg_SHADERS})
		list(APPEND arglist "--input" "${src}")
	endforeach(src)
	list(APPEND arglist ${arg_EXTRA})

	# get the output header name
	set(outputs "${arg_OUTPUT}")
	list(APPEND arglist "--output" "${arg_OUTPUT}")

	# check for separate data file
	if(arg_DATAFILE)
		list(APPEND arglist "--data" "${arg_DATAFILE}")
		list(APPEND outputs "${arg_DATAFILE}")
	endif()

	# create output directories
	foreach(out ${outputs})
		get_filename_component(out_dir ${out} DIRECTORY)
		if(out_dir)
		  get_filename_component(out_dir "${out_dir}" REALPATH BASE_DIR "${CMAKE_CURRENT_BINARY_DIR}")
		  file(MAKE_DIRECTORY ${out_dir})
		endif()
	endforeach()

	# invoke autoshader to generate the interface
	add_custom_command(
		OUTPUT ${outputs}
		COMMAND autoshader ${arglist}
		DEPENDS ${arg_SHADERS} ${arg_DEPENDS} autoshader
		VERBATIM
		)
endfunction()
