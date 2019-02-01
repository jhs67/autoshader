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
#include <numeric>
#include <set>

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
		}
		throw std::runtime_error("invalid type can't be converted to string");
	}

	struct ShaderRecord {
		std::unique_ptr<spirv_cross::Compiler> comp;
		vector<uint32_t> structs;
		std::map<uint32_t, string> names;
	};

	string type_string_mapped(ShaderRecord &sh, spirv_cross::SPIRType type) {
		using spirv_cross::SPIRType;
		if (type.basetype != SPIRType::Struct)
			return type_string(*sh.comp, type);
		auto i = sh.names.find(type.self);
		if (i == sh.names.end())
			throw std::runtime_error("internal error: unmapped structure name");
		return sh.names[type.self];
	}

	// the size of the type as declared in c
	size_t c_type_size(spirv_cross::Compiler &comp, spirv_cross::SPIRType type) {
		using spirv_cross::SPIRType;

		// array's will get the correct padding to match the stride
		if (!type.array.empty())
			throw std::runtime_error("shouldn't get c_type_size of array elements");

		switch (type.basetype) {
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

	void struct_signature(fmt::memory_buffer &r, spirv_cross::Compiler &comp, uint32_t t);

	void type_signature(fmt::memory_buffer &r, spirv_cross::Compiler &comp, uint32_t t) {
		using spirv_cross::SPIRType;
		auto type = comp.get_type(t);
		if (type.basetype == SPIRType::Struct) {
			format_to(r, "{{");
			struct_signature(r, comp, type.self);
			format_to(r, "}}");
		}
		else {
			format_to(r, "{}", type_string(comp, type));
		}
	}

	void struct_signature(fmt::memory_buffer &r, spirv_cross::Compiler &comp, uint32_t t) {
		using spirv_cross::SPIRType;
		auto type = comp.get_type(t);
		if (type.basetype != SPIRType::Struct)
			throw std::runtime_error("struct_signature called on non-struct");

		format_to(r, "{},{:09d}:", comp.get_name(t), comp.get_declared_struct_size(type));

		for (uint32_t i = 0; size_t(i) < type.member_types.size(); ++i) {
			auto mtype = comp.get_type(type.member_types[i]);
			format_to(r, "{},", comp.get_member_name(t, i));
			type_signature(r, comp, type.member_types[i]);
			format_to(r, ",{}", comp.type_struct_member_offset(type, i));
			if (mtype.columns > 1)
				format_to(r, ";{}", comp.type_struct_member_matrix_stride(type, i));
			if (!mtype.array.empty())
				format_to(r, ",{}", comp.type_struct_member_array_stride(type, i));
			for (auto t : mtype.array)
				format_to(r, ",{}", t);
			format_to(r, ":");
		}
	}

	string struct_signature(spirv_cross::Compiler &comp, uint32_t t) {
		fmt::memory_buffer r;
		struct_signature(r, comp, t);
		return to_string(r);
	}

	void add_inline_pad(fmt::memory_buffer &r, int p) {
		format_to(r, "float p0");
		for (uint32_t i = 0; (p -= 4) > 0; )
			format_to(r, ", p{}", ++i);
	}

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
			auto l = type.array.back();
			auto s = comp.type_struct_member_array_stride(stct, m);
			auto t = std::accumulate(begin(type.array), end(type.array), uint32_t(1),
				[] (auto a, auto b) { return a * b; });
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

	string get_pad_name(int &index, std::set<string> &members) {
		for (;;) {
			string t = fmt::format("pad{}_", index);
			index += 1;
			if (members.insert(t).second)
				return t;
		}
	}

	void add_padding(fmt::memory_buffer &r, size_t &offset, size_t target,
			int &index, std::set<string> &members) {
		if (offset >= target)
			return;
		format_to(r, "  float {}", get_pad_name(index, members));
		while ((offset += 4) < target)
			format_to(r, ", {}", get_pad_name(index, members));
		format_to(r, ";\n");
	}

	string struct_definition(ShaderRecord &sh, uint32_t t) {
		using spirv_cross::SPIRType;
		auto &comp = *sh.comp;
		SPIRType type = comp.get_type(t);
		if (type.basetype != SPIRType::Struct)
			throw std::runtime_error("struct_definition called on non-struct");

		fmt::memory_buffer r;
		format_to(r, "struct {} {{\n", sh.names[t]);

		// keep track of pad variables
		int padindex = 0;
		std::set<string> members;
		for (uint32_t i = 0; size_t(i) < type.member_types.size(); ++i)
			members.insert(comp.get_member_name(t, i));

		size_t offset = 0;
		for (uint32_t i = 0; size_t(i) < type.member_types.size(); ++i) {
			// Add padding to bring to the declared offset
			add_padding(r, offset, comp.type_struct_member_offset(type, i), padindex, members);

			format_to(r, "  ");

			// add the member declaration
			auto csize = declare_member(r, sh, t, i);
			format_to(r, ";\n");

			// update the size of the c structure
			offset += csize;
		}

		// pad up to the declared structure size (can this happen?)
		add_padding(r, offset, comp.get_declared_struct_size(type), padindex, members);

		format_to(r, "}}");

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
			auto s = comp.get_type(mtype.self);
			if (s.basetype == spirv_cross::SPIRType::Struct) {
				get_dependant_structs(r, comp, mtype.self);
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

	struct StructIndex {
		size_t sh;
		uint32_t st;
	};

	void assign_struct_names(vector<ShaderRecord> &sh, std::set<string> &shaders,
			std::map<string, vector<StructIndex>> &sigs, string post) {
		// assign each signature a unique name
		size_t i = 0;
		for (auto &sig : shaders) {
			auto &s = sigs[sig];
			auto &c = s.front();
			string name = sh[c.sh].comp->get_name(c.st) + post;
			if (shaders.size() > 1)
				name += fmt::format("_{}", i++);

			bool tagged = false;
			for (auto &i : s) {
				// recored the name for this structure definition
				sh[i.sh].names[i.st] = name;
				// remove duplicate definitions
				auto &st = sh[i.sh].structs;
				if (tagged)
					st.erase(std::find(st.begin(), st.end(), i.st));
				tagged = true;
			}
		}
	}

	string get_shader_postfix(spirv_cross::Compiler &comp) {
		auto ep = comp.get_entry_points_and_stages();
		if (ep.empty())
			throw std::runtime_error("shader stages has no entry point");
		switch (ep[0].execution_model) {
			case spv::ExecutionModelVertex: return "_vert";
			case spv::ExecutionModelTessellationControl: return "_tesc";
			case spv::ExecutionModelTessellationEvaluation: return "_tese";
			case spv::ExecutionModelGeometry: return "_geom";
			case spv::ExecutionModelFragment: return "_frag";
			case spv::ExecutionModelGLCompute: return "_comp";
			case spv::ExecutionModelKernel: return "_krnl";
			case spv::ExecutionModelMax: break;
		}
		throw std::runtime_error("invalid shader execution model");
	}

	void map_struct_names(vector<ShaderRecord> &sh) {
		std::map<string, vector<StructIndex>> names;

		// build a map of all like named structures
		for (size_t i = 0; i < sh.size(); ++i) {
			for (auto t : sh[i].structs) {
				names[sh[i].comp->get_name(t)].push_back({ i, t });
			}
		}

		// fix any name collisions
		for (auto &p : names) {
			// get all the different structure layouts
			std::map<string, vector<StructIndex>> sigs;
			for (auto i : p.second) {
				auto sig = struct_signature(*sh[i.sh].comp, i.st);
				sigs[sig].push_back(i);
			}

			// figure out which structs are shared between shaders
			std::set<string> globals;
			vector<std::set<string>> locals(sh.size());
			for (auto &s : sigs) {
				std::set<size_t> sources;
				for (auto i : s.second)
					sources.insert(i.sh);
				if (sources.size() == 1)
					locals[*sources.begin()].insert(s.first);
				else
					globals.insert(s.first);
			}

			// count the number of non-empty shader namespaces
			auto notempty = !globals.empty() +
				std::count_if(locals.begin(), locals.end(), [] (auto &v) { return !v.empty(); });

			// assign the struct names for each shader namespace
			assign_struct_names(sh, globals, sigs, string());
			for (size_t i = 0; i < locals.size(); ++i) {
				assign_struct_names(sh, locals[i], sigs, notempty == 1 ? string() :
					get_shader_postfix(*sh[i].comp));
			}
		}
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

		// load all the shader stages and find all public structure definitions
		auto input = options["input"].as<vector<string>>();
		vector<ShaderRecord> shaders;
		if (input.empty()) {
			shaders.emplace_back();
			auto &sh = shaders.back();
			sh.comp.reset(new spirv_cross::Compiler(load_spirv("stdin", std::cin)));
			find_buffer_structs(sh.structs, *sh.comp);
		}
		for (auto i : input) {
			shaders.emplace_back();
			auto &sh = shaders.back();
			sh.comp.reset(new spirv_cross::Compiler(load_spirv(i)));
			find_buffer_structs(sh.structs, *sh.comp);
		}

		// re-map potential name collisions
		map_struct_names(shaders);

		// Generate the output
		string out = "\n";
		for (auto &sh : shaders) {
			for (auto t : sh.structs) {
				out += struct_definition(sh, t) + ";\n\n";
			}
		}

		// wrte the output to the intended destination
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
