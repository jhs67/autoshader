//
//  File: specializer.h
//
//  Created by Jon Spencer on 2019-05-14 11:35:59
//  Copyright (c) Jon Spencer. See LICENSE file.
//
#ifndef H_SOURCE_SPECIALIZER_H__
#define H_SOURCE_SPECIALIZER_H__

#include "typereflect.h"

namespace autoshader {

	//-------------------------------------------------------------------------------------------
	//-- write out utility methods for creating vk::SpecializationInfo records.

	void specializers(fmt::memory_buffer &r, vector<ShaderRecord> &sh, const string &indent);

} // namespace autoshader

#endif // H_SOURCE_SPECIALIZER_H__
