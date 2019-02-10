//
//  File: descriptorset.h
//
//  Created by Jon Spencer on 2019-02-10 12:19:02
//  Copyright (c) Jon Spencer. See LICENSE file.
//
#ifndef H_SOURCE_DESCRIPTORSET_H__
#define H_SOURCE_DESCRIPTORSET_H__

#include "autoshader.h"
#include "spirv_cross.hpp"
#include <fmt/format.h>
#include <map>
#include <set>

namespace autoshader {

	enum struct DescriptorType {
		Sampler,
		ImageSampler,
		SampledImage,
		StorageImage,
		Uniform,
		StorageBuffer,
	};

	struct DescriptorRecord {
		std::set<spv::ExecutionModel> stages;
		DescriptorType type;
		spv::Dim imagedim;
		int arraysize;
	};

	struct DescriptorSet {
		std::map<uint32_t, DescriptorRecord> descriptors;
	};


	//-------------------------------------------------------------------------------------------
	// return the execution model for the shaders first entry point

	spv::ExecutionModel get_execution_model(spirv_cross::Compiler &comp);


	//-------------------------------------------------------------------------------------------
	// gather the descriptors from the shader

	void get_descriptor_sets(std::map<uint32_t, DescriptorSet> &r, spirv_cross::Compiler &comp);


	//-------------------------------------------------------------------------------------------
	// write out the descriptor set definition

	void descriptor_layout(fmt::memory_buffer &r, const DescriptorSet &set,
			const string &name, const string &indent);


} // namespace autoshader

#endif // H_SOURCE_DESCRIPTORSET_H__
