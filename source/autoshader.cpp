//
//  File: autoshader.cpp
//
//  Created by Jon Spencer on 2019-01-24 00:02:48
//  Copyright (c) Jon Spencer. See LICENSE file.
//

#include "spirv_cross.hpp"
#include <fmt/format.h>
#include <iostream>
#include <fstream>

namespace autoshader {

	using std::string;
	using std::vector;

	vector<uint32_t> loadSpirv(const string &f) {
		std::ifstream str(f);
		str.seekg(0, str.end);
		size_t l = str.tellg();
		str.seekg(0, str.beg);
		if ((l & 3) != 0)
			throw std::runtime_error("invalid spirv file length for file " + f);
		vector<uint32_t> r(l / 4);
		str.read(reinterpret_cast<char*>(r.data()), 4 * r.size());
		if (!str.good())
			throw std::runtime_error("error loading spriv file " + f);
		return r;
	}

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

	string type_string(spirv_cross::Compiler &comp, uint32_t t) {
		using spirv_cross::SPIRType;
		auto type = comp.get_type(t);
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
				return vector_string(type, "int", "ivec");
			}
			case SPIRType::UInt: {
				return vector_string(type, "uint", "uvec");
			}
			case SPIRType::Int64: {
				if (type.vecsize != 1 || type.columns != 1)
					throw std::runtime_error("invalid vector of int64's");
				return "int64";
			}
			case SPIRType::UInt64: {
				if (type.vecsize != 1 || type.columns != 1)
					throw std::runtime_error("invalid vector of uint64's");
				return "uint64";
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
				return comp.get_name(t);
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
		}
		throw std::runtime_error("invalid type can't be converted to string");
	}

	string declare_var(spirv_cross::Compiler &comp, uint32_t t, string name) {
		return fmt::format("{} {}", type_string(comp, t), name);
	}

	string struct_definition(spirv_cross::Compiler &comp, uint32_t t) {
		using spirv_cross::SPIRType;
		SPIRType type = comp.get_type(t);
		if (type.basetype != SPIRType::Struct)
			throw std::runtime_error("struct_definition called on non-struct");

		fmt::memory_buffer r;
		format_to(r, "struct {} {{ // {}\n", comp.get_name(t), comp.get_declared_struct_size(type));

		for (uint32_t i = 0; size_t(i) < type.member_types.size(); ++i) {
			format_to(r, "  {}; // {}\n", declare_var(comp, type.member_types[i],
				comp.get_member_name(t, i)), comp.type_struct_member_offset(type, i));
		}

		format_to(r, "}}", comp.get_name(t));
		return to_string(r);
	}

	void inspectType(spirv_cross::Compiler &comp, uint32_t t) {
		using spirv_cross::SPIRType;
		auto type = comp.get_type(t);
		switch (type.basetype) {
			case SPIRType::Unknown: {
				std::cout << "Unknown" << std::endl;
				break;
			}
			case SPIRType::Void: {
				std::cout << fmt::format("Void: {} {}", type_string(comp, t), type.width) << std::endl;
				break;
			}
			case SPIRType::Boolean: {
				std::cout << fmt::format("Boolean: {} {}", type_string(comp, t), type.width) << std::endl;
				break;
			}
			case SPIRType::Char: {
				std::cout << fmt::format("Char: {} {}", type_string(comp, t), type.width) << std::endl;
				break;
			}
			case SPIRType::Int: {
				std::cout << fmt::format("Int: {} {}", type_string(comp, t), type.width) << std::endl;
				break;
			}
			case SPIRType::UInt: {
				std::cout << fmt::format("UInt: {} {}", type_string(comp, t), type.width) << std::endl;
				break;
			}
			case SPIRType::Int64: {
				std::cout << fmt::format("Int64: {} {}", type_string(comp, t), type.width) << std::endl;
				break;
			}
			case SPIRType::UInt64: {
				std::cout << fmt::format("UInt64: {} {}", type_string(comp, t), type.width) << std::endl;
				break;
			}
			case SPIRType::AtomicCounter: {
				std::cout << "AtomicCounter" << std::endl;
				break;
			}
			case SPIRType::Half: {
				std::cout << "Half" << std::endl;
				break;
			}
			case SPIRType::Float: {
				std::cout << fmt::format("Float: {} {}", type_string(comp, t), type.width) << std::endl;
				break;
			}
			case SPIRType::Double: {
				std::cout << fmt::format("Double: {} {}", type_string(comp, t), type.width) << std::endl;
				break;
			}
			case SPIRType::Struct: {
				std::cout << fmt::format("Struct {} {}\n{}", comp.get_name(t), type.width, struct_definition(comp, t)) << std::endl;
				for (uint32_t i = 0; size_t(i) < type.member_types.size(); ++i) {
					std::cout << fmt::format("  member {} name: {}", i, comp.get_member_name(t, i)) << std::endl;
					inspectType(comp, type.member_types[i]);
				}
				break;
			}
			case SPIRType::Image: {
				std::cout << fmt::format("Image: {} {}", type_string(comp, t), type.width) << std::endl;
				break;
			}
			case SPIRType::SampledImage: {
				std::cout << "SampledImage" << std::endl;
				break;
			}
			case SPIRType::Sampler: {
				std::cout << "Sampler" << std::endl;
				break;
			}
		}
	}

	void dumpResources(spirv_cross::Compiler &comp, spirv_cross::Resource r) {
		unsigned set = comp.get_decoration(r.id, spv::DecorationDescriptorSet);
		unsigned binding = comp.get_decoration(r.id, spv::DecorationBinding);
		std::cout << fmt::format("  {} {} {}", r.name, set, binding);
		inspectType(comp, r.type_id);
	}

	void main(string f) {
		auto spirv = loadSpirv(f);
		spirv_cross::Compiler comp(move(spirv));
		spirv_cross::ShaderResources res = comp.get_shader_resources();

		std::cout << "uniform_buffers" << std::endl;
		for (auto &r : res.uniform_buffers) {
			dumpResources(comp, r);
		}

		std::cout << "storage_buffers" << std::endl;
		for (auto &r : res.storage_buffers) {
			dumpResources(comp, r);
		}

		std::cout << "stage_inputs" << std::endl;
		for (auto &r : res.stage_inputs) {
			dumpResources(comp, r);
		}

		std::cout << "stage_outputs" << std::endl;
		for (auto &r : res.stage_outputs) {
			dumpResources(comp, r);
		}

		std::cout << "subpass_inputs" << std::endl;
		for (auto &r : res.subpass_inputs) {
			dumpResources(comp, r);
		}

		std::cout << "storage_images" << std::endl;
		for (auto &r : res.storage_images) {
			dumpResources(comp, r);
		}

		std::cout << "sampled_images" << std::endl;
		for (auto &r : res.sampled_images) {
			dumpResources(comp, r);
		}

		std::cout << "push_constant_buffers" << std::endl;
		for (auto &r : res.push_constant_buffers) {
			dumpResources(comp, r);
		}

		std::cout << "separate_images" << std::endl;
		for (auto &r : res.separate_images) {
			dumpResources(comp, r);
		}

		std::cout << "separate_samplers" << std::endl;
		for (auto &r : res.separate_samplers) {
			dumpResources(comp, r);
		}

	}

} // namespace autoshader

int main(int ac, const char *av[]) {
	autoshader::main(av[1]);
}
