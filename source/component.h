//
//  File: component.h
//
//  Created by Jon Spencer on 2019-02-10 13:17:20
//  Copyright (c) Jon Spencer. See LICENSE file.
//
#ifndef H_SOURCE_COMPONENT_H__
#define H_SOURCE_COMPONENT_H__

#include "typereflect.h"
#include "descriptorset.h"

namespace autoshader {

	//------------------------------------------------------------------------------------------
	//-- generate the shader component structure

	void generate_components(fmt::memory_buffer &r,
		std::map<uint32_t, DescriptorSet> &sets, vector<ShaderRecord> &sh, bool withVertex,
		bool withPush, const string &indent);

} // namespace autoshader

#endif // H_SOURCE_COMPONENT_H__
