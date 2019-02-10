//
//  File: namemap.h
//
//  Created by Jon Spencer on 2019-02-10 12:11:19
//  Copyright (c) Jon Spencer. See LICENSE file.
//
#ifndef H_SOURCE_NAMEMAP_H__
#define H_SOURCE_NAMEMAP_H__

#include "typereflect.h"

namespace autoshader {

	//------------------------------------------------------------------------------------------
	//-- re-map all conflicting structure names
	void map_struct_names(vector<ShaderRecord> &sh);

} // namespace autoshader

#endif // H_SOURCE_NAMEMAP_H__
