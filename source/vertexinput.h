//
//  File: vertexinput.h
//
//  Created by Jon Spencer on 2019-02-10 12:54:12
//  Copyright (c) Jon Spencer. See LICENSE file.
//
#ifndef H_SOURCE_VERTEXINPUT_H__
#define H_SOURCE_VERTEXINPUT_H__

#include "autoshader.h"
#include "spirv_cross.hpp"
#include <fmt/format.h>

namespace autoshader {

	void get_vertex_definition(fmt::memory_buffer &r, spirv_cross::Compiler &comp,
			const string &name, const string &indent);

} // namespace autoshader

#endif // H_SOURCE_VERTEXINPUT_H__
