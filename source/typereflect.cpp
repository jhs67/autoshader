//
//  File: typereflect.cpp
//
//  Created by Jon Spencer on 2019-02-10 11:45:08
//  Copyright (c) Jon Spencer. See LICENSE file.
//

#include "typereflect.h"
#include <numeric>
#include <set>

namespace autoshader {

	namespace {

		//------------------------------------------------------------------------------------------
		//-- return the correct type for vector and matrix types (i.e. float vs. vec4 vs. mat2x3)

		string vector_string(spirv_cross::SPIRType const& type, const char *base,
				const char *vec, const char *mat = nullptr) {
			if (type.vecsize == 1) {
				if (type.columns != 1)
					throw std::runtime_error("unexpected row vector");
				return base;
			}

			if (type.columns == 1)
				return fmt::format("{}{}", vec, type.vecsize);

			if (mat == nullptr)
				throw std::runtime_error("unexpected type for matrix: " + string(base));

			if (type.columns == type.vecsize)
				return fmt::format("{}{}", mat, type.vecsize);

			return fmt::format("{}{}x{}", mat, type.columns, type.vecsize);
		}


		//------------------------------------------------------------------------------------------
		//-- return the suffix for the image dimension type

		string image_dimension_string(spv::Dim d) {
			switch (d) {
				case spv::Dim1D:
					return "1D";
				case spv::Dim2D:
					return "2D";
				case spv::Dim3D:
					return "3D";
				case spv::DimCube:
					return "Cube";
				case spv::DimRect:
					return "Rect";
				case spv::DimBuffer:
					throw std::runtime_error("image buffer's not supported");
				case spv::DimSubpassData:
					throw std::runtime_error("subpass data not supported");
				case spv::DimMax: break;
			}
			throw std::runtime_error("invalid image dimension");
		}

		//------------------------------------------------------------------------------------------
		//-- given a type return the mapped type name

		string type_string_mapped(ShaderRecord &sh, spirv_cross::SPIRType type) {
			using spirv_cross::SPIRType;
			if (type.basetype != SPIRType::Struct)
				return type_string(*sh.comp, type);
			auto i = sh.names.find(type.self);
			if (i == sh.names.end())
				throw std::runtime_error("internal error: unmapped structure name");
			return sh.names[type.self];
		}

		//------------------------------------------------------------------------------------------
		// the size of the type as declared in c

		size_t c_type_size(spirv_cross::Compiler &comp, spirv_cross::SPIRType type) {
			using spirv_cross::SPIRType;

			// array's will get the correct padding to match the stride
			if (!type.array.empty())
				throw std::runtime_error("shouldn't get c_type_size of array elements");

			switch (type.basetype) {
				case SPIRType::SByte:
				case SPIRType::UByte:
				case SPIRType::Short:
				case SPIRType::UShort:
				case SPIRType::ControlPointArray:
				case SPIRType::Void:
				case SPIRType::Unknown:
				case SPIRType::AtomicCounter:
				case SPIRType::Image:
				case SPIRType::SampledImage:
				case SPIRType::Sampler:
				case SPIRType::Boolean:
					throw std::runtime_error("invalid type for structure member");

				case SPIRType::Char:
				case SPIRType::Int:
				case SPIRType::UInt:
				case SPIRType::Int64:
				case SPIRType::UInt64:
				case SPIRType::Half:
				case SPIRType::Float:
				case SPIRType::Double:
					return type.width * type.vecsize * type.columns / 8;

				case SPIRType::Struct:
					// struct's will be padded to the declared size
					return comp.get_declared_struct_size(type);
			}

			throw std::runtime_error("invalid type for structure member");
		}

		//------------------------------------------------------------------------------------------
		//-- add padding to an inline structure definition

		void add_inline_pad(fmt::memory_buffer &r, int p) {
			format_to(r, "float p0");
			for (uint32_t i = 0; (p -= 4) > 0; )
				format_to(r, ", p{}", ++i);
		}

		//------------------------------------------------------------------------------------------
		//-- declare a the member of the structure type

		size_t declare_member(fmt::memory_buffer &r, ShaderRecord &sh,
				uint32_t t, uint32_t m) {
			auto &comp = *sh.comp;
			auto stct = comp.get_type(t);
			string name = comp.get_member_name(t, m);
			auto type = comp.get_type(stct.member_types[m]);

			// check if this type needs column padding
			int colpad = 0;
			auto arrtype = comp.get_type(type.self);
			auto coltype = arrtype;
			coltype.columns = 1;
			auto ccolsize = c_type_size(comp, coltype);
			if (arrtype.columns > 1) {
				auto scolsize = comp.type_struct_member_matrix_stride(stct, m);
				colpad = scolsize - ccolsize;
				ccolsize = scolsize;
			}

			// check it the type needs array column padding
			int arrpad = 0;
			auto carrsize = ccolsize * arrtype.columns;
			if (!type.array.empty()) {
				auto l = std::max(type.array.back(), uint32_t(1));
				auto s = comp.type_struct_member_array_stride(stct, m);
				auto t = std::accumulate(begin(type.array), end(type.array), uint32_t(1),
					[] (auto a, auto b) { return a * std::max(b, uint32_t(1)); });
				arrpad = s * l / t - carrsize;
				carrsize += arrpad;
			}

			if (arrpad != 0) {
				// array pad pre-amble
				format_to(r, "struct {{ ");
			}
			if (colpad != 0) {
				// column padding if needed
				format_to(r, "struct {{ {} v; ", type_string_mapped(sh, coltype));
				add_inline_pad(r, colpad);
				format_to(r, "; }} ");
			}
			else {
				format_to(r, "{} ", type_string_mapped(sh, arrtype));
			}
			format_to(r, "{}", arrpad != 0 ? "v" : name);
			if (colpad != 0) {
				format_to(r, "[{}]", arrtype.columns);
			}
			if (arrpad != 0) {
				format_to(r, "; ", name);
				add_inline_pad(r, arrpad);
				format_to(r, "; }} {}", name);
			}

			// add the array elements
			for (size_t i = type.array.size(); i-- != 0;) {
				format_to(r, "[{}]", type.array[i]);
				carrsize *= type.array[i];
			}

			return carrsize;
		}

		//------------------------------------------------------------------------------------------
		//-- find an unused name for a pad variable

		string get_pad_name(int &index, std::set<string> &members) {
			for (;;) {
				string t = fmt::format("pad{}_", index);
				index += 1;
				if (members.insert(t).second)
					return t;
			}
		}

		//------------------------------------------------------------------------------------------
		//-- add padding members for the structure definition

		void add_padding(fmt::memory_buffer &r, size_t &offset, size_t target,
				int &index, std::set<string> &members, const string& indent) {
			if (offset >= target)
				return;
			format_to(r, "{}  float {}", indent, get_pad_name(index, members));
			while ((offset += 4) < target)
				format_to(r, ", {}", get_pad_name(index, members));
			format_to(r, ";\n");
		}


	} // namespace


	//------------------------------------------------------------------------------------------
	//-- return the the c type matching the glsl type

	string type_string(spirv_cross::Compiler &comp, spirv_cross::SPIRType type) {
		using spirv_cross::SPIRType;
		switch (type.basetype) {
			case SPIRType::Unknown: {
				throw std::runtime_error("unkown type can't be converted to string");
			}
			case SPIRType::Void: {
				if (type.vecsize != 1 || type.columns != 1)
					throw std::runtime_error("invalid vector of void's");
				return "void";
			}
			case SPIRType::Boolean: {
				return vector_string(type, "bool", "bvec");
			}
			case SPIRType::Char: {
				if (type.vecsize != 1 || type.columns != 1)
					throw std::runtime_error("invalid vector of char's");
				return "char";
			}
			case SPIRType::Int: {
				return vector_string(type, "int32_t", "ivec");
			}
			case SPIRType::UInt: {
				return vector_string(type, "uint32_t", "uvec");
			}
			case SPIRType::Int64: {
				if (type.vecsize != 1 || type.columns != 1)
					throw std::runtime_error("invalid vector of int64's");
				return "int64_t";
			}
			case SPIRType::UInt64: {
				if (type.vecsize != 1 || type.columns != 1)
					throw std::runtime_error("invalid vector of uint64's");
				return "uint64_t";
			}
			case SPIRType::AtomicCounter: {
				throw std::runtime_error("automic counter's not supported");
			}
			case SPIRType::Half: {
				throw std::runtime_error("half float's not supported");
			}
			case SPIRType::Float: {
				return vector_string(type, "float", "vec", "mat");
			}
			case SPIRType::Double: {
				return vector_string(type, "double", "dvec", "dmat");
			}
			case SPIRType::Struct: {
				return comp.get_name(type.self);
			}
			case SPIRType::Image: {
				return fmt::format("{}{}", type.image.sampled == 1 ? "texture" : "image",
					image_dimension_string(type.image.dim));
			}
			case SPIRType::SampledImage: {
				throw std::runtime_error("sampled image's not supported");
			}
			case SPIRType::Sampler: {
				throw std::runtime_error("sampler's not supported");
			}
			case SPIRType::SByte: {
				throw std::runtime_error("SByte's not supported");
			}
			case SPIRType::UByte: {
				throw std::runtime_error("UByte's not supported");
			}
			case SPIRType::Short: {
				throw std::runtime_error("Short's not supported");
			}
			case SPIRType::UShort: {
				throw std::runtime_error("UShort's not supported");
			}
			case SPIRType::ControlPointArray: {
				throw std::runtime_error("ControlPointArray's not supported");
			}
		}
		throw std::runtime_error("invalid type can't be converted to string");
	}

	//------------------------------------------------------------------------------------------
	//-- format the structure definition into the buffer

	void struct_definition(fmt::memory_buffer &r, ShaderRecord &sh, uint32_t t,
			const string& indent) {
		using spirv_cross::SPIRType;
		auto &comp = *sh.comp;
		SPIRType type = comp.get_type(t);
		if (type.basetype != SPIRType::Struct)
			throw std::runtime_error("struct_definition called on non-struct");

		format_to(r, "{}struct {} {{\n", indent, sh.names[t]);

		// keep track of pad variables
		int padindex = 0;
		std::set<string> members;
		for (uint32_t i = 0; size_t(i) < type.member_types.size(); ++i)
			members.insert(comp.get_member_name(t, i));

		size_t offset = 0;
		for (uint32_t i = 0; size_t(i) < type.member_types.size(); ++i) {
			// Add padding to bring to the declared offset
			add_padding(r, offset, comp.type_struct_member_offset(type, i), padindex,
				members, indent);

			format_to(r, "{}  ", indent);

			// add the member declaration
			auto csize = declare_member(r, sh, t, i);
			format_to(r, ";\n");

			// update the size of the c structure
			offset += csize;
		}

		// pad up to the declared structure size (can this happen?)
		add_padding(r, offset, comp.get_declared_struct_size(type), padindex, members, indent);

		format_to(r, "{}}}", indent);
	}

} // namespace autoshader
