//
//  File: descriptorwrite.h
//
//  Created by Jon Spencer on 2019-02-20 23:12:08
//  Copyright (c) Jon Spencer. See LICENSE file.
//
#ifndef H_SOURCE_DESCRIPTORWRITE_H__
#define H_SOURCE_DESCRIPTORWRITE_H__

#include "descriptorset.h"

namespace autoshader {

	//-------------------------------------------------------------------------------------------
	//-- write out utility methods for writing descriptor set updates.

	void descriptor_writer(fmt::memory_buffer &r, const std::map<uint32_t, DescriptorSet> &set,
		const string &indent);

} // namespace autoshader

#endif // H_SOURCE_DESCRIPTORWRITE_H__
