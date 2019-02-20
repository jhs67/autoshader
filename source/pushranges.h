//
//  File: pushranges.h
//
//  Created by Jon Spencer on 2019-02-19 10:52:31
//  Copyright (c) Jon Spencer. See LICENSE file.
//
#ifndef H_SOURCE_PUSHRANGES_H__
#define H_SOURCE_PUSHRANGES_H__

#include "typereflect.h"

namespace autoshader {

	//------------------------------------------------------------------------------------------
	//-- format the push ranges into the buffer

	bool push_ranges(fmt::memory_buffer &r, vector<ShaderRecord> &sh, const string& indent);

} // namespace autoshader

#endif // H_SOURCE_PUSHRANGES_H__
