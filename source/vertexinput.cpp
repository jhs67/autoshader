//
//  File: vertexinput.cpp
//
//  Created by Jon Spencer on 2019-02-10 12:54:57
//  Copyright (c) Jon Spencer. See LICENSE file.
//

#include "vertexinput.h"
#include "typereflect.h"

namespace autoshader {

	namespace {

		//------------------------------------------------------------------------------------------
		// return a vk::Format for the associated type

		string vertex_format_string(spirv_cross::Compiler &comp, uint32_t id) {
			using spirv_cross::SPIRType;
			auto type = comp.get_type(id);
			string ext;
			int bits;
			switch (type.basetype) {
				case SPIRType::Unknown: {
					throw std::runtime_error("cant' get vertex format of unkown type");
				}
				case SPIRType::Char:
				case SPIRType::Boolean:
				case SPIRType::Void:
				case SPIRType::AtomicCounter:
				case SPIRType::Half:
				case SPIRType::Struct:
				case SPIRType::Image:
				case SPIRType::SampledImage:
				case SPIRType::Sampler:
					throw std::runtime_error("invalid type for vertex format");
				case SPIRType::Int: {
					ext = "Sint";
					bits = 32;
					break;
				}
				case SPIRType::UInt: {
					ext = "Uint";
					bits = 32;
					break;
				}
				case SPIRType::Int64: {
					ext = "Sint";
					bits = 64;
					break;
				}
				case SPIRType::UInt64: {
					ext = "Uint";
					bits = 64;
					break;
				}
				case SPIRType::Float: {
					ext = "Sfloat";
					bits = 32;
					break;
				}
				case SPIRType::Double: {
					ext = "Sfloat ";
					bits = 64;
					break;
				}
			}

			if (ext.empty())
				throw std::runtime_error("unexpected type for vertex format");

			fmt::memory_buffer r;
			format_to(r, "vk::Format::e");
			static constexpr char components[4] = { 'R', 'G', 'B', 'A' };
			for (uint32_t i = 0; i < type.vecsize && i < 4; ++i) {
				format_to(r, "{}{}", components[i], bits);
			}
			format_to(r, "{}", ext);
			return to_string(r);
		}

	} // namespace


	//------------------------------------------------------------------------------------------
	// format a default vertex definition into the buffer

	bool get_vertex_definition(fmt::memory_buffer &r, spirv_cross::Compiler &comp,
			const string &name, const string &indent) {
		spirv_cross::ShaderResources res = comp.get_shader_resources();
		if (res.stage_inputs.empty())
			return false;

		format_to(r, "{}struct {} {{\n", indent, name);
		for (auto &v : res.stage_inputs) {
			auto var = comp.get_type(v.type_id);
			auto type = comp.get_type(v.base_type_id);
			format_to(r, "{}  {} {}", indent, type_string(comp, type), v.name);
			for (size_t i = var.array.size(); i-- != 0;)
				format_to(r, "[{}]", var.array[i]);
			format_to(r, ";\n");
		}
		format_to(r, "{}}};\n\n", indent);

		format_to(r, "{}auto getVertexBindingDescription() {{\n", indent);
		format_to(r, "{}  return std::array<vk::VertexInputBindingDescription, 1>({{{{\n", indent);
		format_to(r, "{}    {{ 0, sizeof({}), vk::VertexInputRate::eVertex }}\n", indent, name);
		format_to(r, "{}  }}}});\n{}}}\n\n", indent, indent);

		format_to(r, "{}auto getVertexAttributeDescriptions() {{\n", indent);
		format_to(r, "{}  return std::array<vk::VertexInputAttributeDescription, {}>({{{{\n",
			indent, res.stage_inputs.size());
		for (auto &v : res.stage_inputs) {
			format_to(r, "{}    {{ {}, 0, {}, offsetof({}, {}) }},\n", indent,
				comp.get_decoration(v.id, spv::DecorationLocation),
				vertex_format_string(comp, v.base_type_id), name, v.name);
		}
		format_to(r, "{}  }}}});\n{}}}\n\n", indent, indent);
		return true;
	}

} // namespace autoshader
