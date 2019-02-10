//
//  File: typereflect.h
//
//  Created by Jon Spencer on 2019-02-10 11:44:45
//  Copyright (c) Jon Spencer. See LICENSE file.
//
#ifndef H_SOURCE_TYPEREFLECT_H__
#define H_SOURCE_TYPEREFLECT_H__

#include "autoshader.h"
#include "spirv_cross.hpp"
#include <fmt/format.h>
#include <map>

namespace autoshader {

	struct ShaderRecord {
		std::unique_ptr<spirv_cross::Compiler> comp;
		vector<uint32_t> structs;
		std::map<uint32_t, string> names;
	};

	//------------------------------------------------------------------------------------------
	//-- return the the c type matching the glsl type
	string type_string(spirv_cross::Compiler &comp, spirv_cross::SPIRType type);

	//------------------------------------------------------------------------------------------
	//-- format the structure definition into the buffer
	void struct_definition(fmt::memory_buffer &r, ShaderRecord &sh, uint32_t t,
		const string& indent);

}

#endif // H_SOURCE_TYPEREFLECT_H__
