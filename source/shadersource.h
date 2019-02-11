//
//  File: shadersource.h
//
//  Created by Jon Spencer on 2019-02-11 11:22:34
//  Copyright (c) Jon Spencer. See LICENSE file.
//
#ifndef H_SOURCE_SHADERSOURCE_H__
#define H_SOURCE_SHADERSOURCE_H__

#include "typereflect.h"

namespace autoshader {

	//------------------------------------------------------------------------------------------
	//-- return the name based on the shader stage

	string get_execution_string(spirv_cross::Compiler &comp);

	//------------------------------------------------------------------------------------------
	//-- declare the storage for the shader source
	void shader_source_decl(fmt::memory_buffer &r, vector<ShaderRecord> &sh, const string& indent);

	//------------------------------------------------------------------------------------------
	//-- format the shader source into the buffer
	void shader_source(fmt::memory_buffer &r, vector<ShaderRecord> &sh, const string& indent);

} // namespace autoshader

#endif // H_SOURCE_SHADERSOURCE_H__
