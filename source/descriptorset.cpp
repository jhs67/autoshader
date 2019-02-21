//
//  File: descriptorset.cpp
//
//  Created by Jon Spencer on 2019-02-10 12:19:14
//  Copyright (c) Jon Spencer. See LICENSE file.
//

#include "descriptorset.h"

namespace autoshader {

	namespace {


		//-------------------------------------------------------------------------------------------
		// return the stage flag for the given shader

		const char *vulkan_stage_string(spv::ExecutionModel e) {
			switch (e) {
				case spv::ExecutionModelVertex:
					return "vk::ShaderStageFlagBits::eVertex";
				case spv::ExecutionModelTessellationControl:
					return "vk::ShaderStageFlagBits::eTessellationControl";
				case spv::ExecutionModelTessellationEvaluation:
					return "vk::ShaderStageFlagBits::eTessellationEvaluation";
				case spv::ExecutionModelGeometry:
					return "vk::ShaderStageFlagBits::eGeometry";
				case spv::ExecutionModelFragment:
					return "vk::ShaderStageFlagBits::eFragment";
				case spv::ExecutionModelGLCompute:
					return "vk::ShaderStageFlagBits::eCompute";
				default: break;
			}
			throw std::runtime_error("unsupported execution model for shader");
		}


		//-------------------------------------------------------------------------------------------
		// get descriptor sets for the given resource type

		void get_descriptor_sets(std::map<uint32_t, DescriptorSet> &ds, spirv_cross::Compiler &comp,
				spv::ExecutionModel em, std::vector<spirv_cross::Resource> &res, DescriptorType d) {

			for (auto &v : res) {
				auto set = comp.get_decoration(v.id, spv::DecorationDescriptorSet);
				auto bin = comp.get_decoration(v.id, spv::DecorationBinding);
				auto type = comp.get_type(v.base_type_id);
				auto var = comp.get_type(v.type_id);
				int as = var.array.empty() ? 1 : var.array.front();
				auto t = ds[set].descriptors.emplace(bin,
					DescriptorRecord{ {}, d, type.image.dim, as });
				if (t.first->second.type != d)
					throw std::runtime_error(fmt::format(
						"type mismatch for descriptor(set={} binding={})", set, bin));
				if (t.first->second.imagedim != type.image.dim)
					throw std::runtime_error(fmt::format(
						"image dimension mismatch for descriptor(set={} binding={})", set, bin));
				if (t.first->second.arraysize != as)
					throw std::runtime_error(fmt::format(
						"array size mismatch for descriptor(set={} binding={})", set, bin));
				t.first->second.stages.insert(em);
				if (t.first->second.name.empty())
					t.first->second.name = v.name;
			}
		}

	} // namespace


	//-------------------------------------------------------------------------------------------
	// return the vulkan type for a descriptor resource

	const char *vulkan_descriptor_type(DescriptorType type) {
		switch (type) {
			case DescriptorType::Sampler: return "vk::DescriptorType::eSampler";
			case DescriptorType::ImageSampler: return "vk::DescriptorType::eCombinedImageSampler";
			case DescriptorType::SampledImage: return "vk::DescriptorType::eSampledImage";
			case DescriptorType::StorageImage: return "vk::DescriptorType::eStorageImage";
			case DescriptorType::Uniform: return "vk::DescriptorType::eUniformBuffer";
			case DescriptorType::StorageBuffer: return "vk::DescriptorType::eStorageBuffer";
		}
		throw std::runtime_error("internal error: invalid descriptor type");
	}


	//-------------------------------------------------------------------------------------------
	// return the combination of stage flags

	void vulkan_stage_flags(fmt::memory_buffer &r, std::set<spv::ExecutionModel> const& stages) {
		bool s = false;
		for (auto e : stages) {
			if (s)
				format_to(r, " | ");
			format_to(r, "{}", vulkan_stage_string(e));
			s = true;
		}
	}


	//-------------------------------------------------------------------------------------------
	// return the execution model for the shaders first entry point

	spv::ExecutionModel get_execution_model(spirv_cross::Compiler &comp) {
		auto ep = comp.get_entry_points_and_stages();
		if (ep.empty())
			throw std::runtime_error("shader stages has no entry point");
		return ep[0].execution_model;
	}


	//-------------------------------------------------------------------------------------------
	// gather the descriptors from the shader

	void get_descriptor_sets(std::map<uint32_t, DescriptorSet> &r, spirv_cross::Compiler &comp) {
		auto em = get_execution_model(comp);
		spirv_cross::ShaderResources res = comp.get_shader_resources();
		get_descriptor_sets(r, comp, em, res.uniform_buffers, DescriptorType::Uniform);
		get_descriptor_sets(r, comp, em, res.storage_buffers, DescriptorType::StorageBuffer);
		get_descriptor_sets(r, comp, em, res.storage_images, DescriptorType::StorageImage);
		get_descriptor_sets(r, comp, em, res.sampled_images, DescriptorType::ImageSampler);
		get_descriptor_sets(r, comp, em, res.separate_images, DescriptorType::SampledImage);
		get_descriptor_sets(r, comp, em, res.separate_samplers, DescriptorType::Sampler);
	}


	//-------------------------------------------------------------------------------------------
	// write out the descriptor set definition

	void descriptor_layout(fmt::memory_buffer &r, const DescriptorSet &set,
			const string &name, const string &indent) {
		bool c = false;
		format_to(r, "{}auto get{}LayoutBindings() {{\n", indent, name);
		format_to(r, "{}  return std::array<vk::DescriptorSetLayoutBinding, {}>({{{{", indent,
			set.descriptors.size());
		for (auto &d : set.descriptors) {
			format_to(r, c ? ",\n" : "\n");
			format_to(r, "{}    {{ {}, {}, {}, ", indent, d.first,
				vulkan_descriptor_type(d.second.type), d.second.arraysize);
			vulkan_stage_flags(r, d.second.stages);
			format_to(r, " }}");
			c = true;
		}
		format_to(r, "\n{}  }}}});\n{}}}\n\n", indent, indent);
	}


	//-------------------------------------------------------------------------------------------
	// return the stage flags for the first entry point

	string get_shader_stage_flags(spirv_cross::Compiler &comp) {
		return vulkan_stage_string(get_execution_model(comp));
	}


	//-------------------------------------------------------------------------------------------
	// return the execution model for the shaders first entry point

	string get_first_entry_point_name(spirv_cross::Compiler &comp) {
		auto ep = comp.get_entry_points_and_stages();
		if (ep.empty())
			throw std::runtime_error("shader stages has no entry point");
		return ep[0].name;
	}

} // namespace autoshader
