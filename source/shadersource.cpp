//
//  File: shadersource.cpp
//
//  Created by Jon Spencer on 2019-02-11 11:22:29
//  Copyright (c) Jon Spencer. See LICENSE file.
//

#include "shadersource.h"
#include "descriptorset.h"

namespace autoshader {

	//------------------------------------------------------------------------------------------
	//-- return the postifix based on the shader stage

	string get_execution_string(spirv_cross::Compiler &comp) {
		auto ep = comp.get_entry_points_and_stages();
		if (ep.empty())
			throw std::runtime_error("shader stages has no entry point");
		switch (ep[0].execution_model) {
			case spv::ExecutionModelVertex: return "vert";
			case spv::ExecutionModelTessellationControl: return "tesc";
			case spv::ExecutionModelTessellationEvaluation: return "tese";
			case spv::ExecutionModelGeometry: return "geom";
			case spv::ExecutionModelFragment: return "frag";
			case spv::ExecutionModelGLCompute: return "comp";
			case spv::ExecutionModelKernel: return "krnl";
			case spv::ExecutionModelMax: break;
		}
		throw std::runtime_error("invalid shader execution model");
	}


	//------------------------------------------------------------------------------------------
	//-- declare the storage for the shader source

	void shader_source_decl(fmt::memory_buffer &r, vector<ShaderRecord> &sh, const string& indent) {
		format_to(r, "\n");
		for (auto &s : sh) {
			auto p = get_execution_string(*s.comp);
			format_to(r, "{}extern const uint32_t {}_size;\n", indent, p);
			format_to(r, "{}extern const uint32_t {}_data[];\n", indent, p);
		}
	}


	//------------------------------------------------------------------------------------------
	//-- format the shader source into the buffer

	void shader_source(fmt::memory_buffer &r, vector<ShaderRecord> &sh, const string& indent) {
		format_to(r, "\n#ifdef AUTOSHADER_SOURCE_DATA\n\n");
		for (auto &s : sh) {
			auto p = get_execution_string(*s.comp);
			format_to(r, "{}extern const uint32_t {}_size = {}\n", indent, p,
				sizeof(uint32_t) * s.source.size());
			format_to(r, "{}extern const uint32_t {}_data[] = {{\n", indent, p);
			for (size_t i = 0; i < s.source.size();) {
				format_to(r, "{}  ", indent);
				for (size_t j = 0; i < s.source.size() && j < 8; ++j, ++i) {
					format_to(r, "0x{:08x}{}", s.source[i], i == s.source.size() - 1 ? "" : ",");
				}
				format_to(r, "\n");
			}
			format_to(r, "{}}};\n\n", indent);
		}
		format_to(r, "#endif // AUTOSHADER_SOURCE_DATA\n");
	}

} // namespace autoshader
