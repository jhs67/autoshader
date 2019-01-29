//
//  File: autoshader.cpp
//
//  Created by Jon Spencer on 2019-01-24 00:02:48
//  Copyright (c) Jon Spencer. See LICENSE file.
//

#include "spirv_cross.hpp"
#include <fmt/format.h>
#include <cxxopts.hpp>
#include <filesystem>
#include <iostream>
#include <fstream>

namespace autoshader {

	using std::string;
	using std::vector;

	vector<uint32_t> load_spirv(const std::string &f, std::istream &str) {
		vector<uint32_t> r(64 * 1024);
		for (size_t s = 0;;) {
			str.read(reinterpret_cast<char*>(r.data()) + s, 4 * r.size() - s);
			if (str.bad())
				throw std::runtime_error("error loading spriv from " + f);
			s += str.gcount();
			if (str.eof()) {
				if (s & 3)
					throw std::runtime_error("invalid spirv file length for " + f);
				r.resize(s / 4);
				return r;
			}

		}
	}

	vector<uint32_t> load_spirv(const string &f) {
		std::ifstream str(f);
		if (!str.good())
			throw std::runtime_error("failed to open file: " + f);
		return load_spirv("file " + f, str);
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
			format_to(r, "  {}; // {} {} {} {} {}\n", declare_var(comp, type.member_types[i],
				comp.get_member_name(t, i)), comp.type_struct_member_offset(type, i),
				comp.get_declared_struct_member_size(type, i),
				comp.has_decoration(type.member_types[i], spv::DecorationArrayStride) ? comp.type_struct_member_array_stride(type, i) : 0,
				comp.has_decoration(type.member_types[i], spv::DecorationMatrixStride) ? comp.type_struct_member_matrix_stride(type, i) : 0,
				comp.get_member_qualified_name(t, i));
		}

		format_to(r, "}}", comp.get_name(t));
		return to_string(r);
	}


	void get_dependant_structs(vector<uint32_t> &r, spirv_cross::Compiler &comp, uint32_t b) {
		if (std::any_of(r.begin(), r.end(), [b] (auto r) { return r == b; }))
			return;
		auto type = comp.get_type(b);
		if (type.basetype != spirv_cross::SPIRType::Struct)
			throw std::runtime_error(fmt::format("get_dependant_structs called with non struct type {}", type.basetype));
		for (uint32_t i = 0; size_t(i) < type.member_types.size(); ++i) {
			uint32_t t = type.member_types[i];
			auto mtype = comp.get_type(t);
			if (mtype.basetype == spirv_cross::SPIRType::Struct) {
				get_dependant_structs(r, comp, t);
			}
		}
		r.push_back(b);
	}

	void find_buffer_structs(vector<uint32_t> &r, spirv_cross::Compiler &comp) {
		spirv_cross::ShaderResources res = comp.get_shader_resources();
		for (auto &v : res.uniform_buffers)
			get_dependant_structs(r, comp, v.base_type_id);
		for (auto &v : res.storage_buffers)
			get_dependant_structs(r, comp, v.base_type_id);
		for (auto &v : res.push_constant_buffers)
			get_dependant_structs(r, comp, v.base_type_id);
	}

	int main(int ac, char *av[]) {
		auto prog = std::filesystem::path(av[0]).filename();
		cxxopts::Options opts(prog, "automatic shader reflection to c++");

		opts
		.show_positional_help()
		.positional_help("input*")
		.add_options()
			("h,help", "print help")
			("i,input", "input spirv file (stdin if missing)", cxxopts::value<vector<string>>())
			("o,output", "output source file (stdout if missing)", cxxopts::value<string>());
		opts.parse_positional({ "input" });

		auto options = opts.parse(ac, av);

		if (options["help"].as<bool>()) {
			std::cerr << opts.help() << std::endl;
			return 0;
		}

		string out = "\n";
		auto input = options["input"].as<vector<string>>();
		if (input.empty()) {
			spirv_cross::Compiler comp(load_spirv("stdin", std::cin));
			vector<uint32_t> r;
			find_buffer_structs(r, comp);
			for (auto t : r)
				out += struct_definition(comp, t) + ";\n\n";
		}
		for (auto i : input) {
			spirv_cross::Compiler comp(load_spirv(i));
			vector<uint32_t> r;
			find_buffer_structs(r, comp);
			for (auto t : r)
				out += struct_definition(comp, t) + ";\n\n";
		}

		if (options.count("output") == 0) {
			std::cout.write(out.data(), out.size());
		}
		else {
			auto outname = options["output"].as<string>();
			std::ofstream str(outname);
			str.write(out.data(), out.size());
			if (!str.good())
				throw std::runtime_error("error writing output to: " + outname);
		}

		return 0;
	}

} // namespace autoshader

int main(int ac, char *av[]) {
	try {
		return autoshader::main(ac, av);
	}
	catch (std::exception &err) {
		std::cerr << av[0] << " failed: " << err.what() << std::endl;
	}
	return 10;
}
