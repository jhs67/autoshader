//
//  File: autoshader.cpp
//
//  Created by Jon Spencer on 2019-01-24 00:02:48
//  Copyright (c) Jon Spencer. See LICENSE file.
//

#include "autoshader.h"
#include "shadersource.h"
#include "descriptorset.h"
#include "typereflect.h"
#include "vertexinput.h"
#include "pushranges.h"
#include "component.h"
#include "namemap.h"
#include <cxxopts.hpp>
#include <iostream>
#include <fstream>
#include <set>

namespace autoshader {

	namespace {

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

		void get_dependant_structs(vector<uint32_t> &r, spirv_cross::Compiler &comp, uint32_t b) {
			if (std::any_of(r.begin(), r.end(), [b] (auto r) { return r == b; }))
				return;
			auto type = comp.get_type(b);
			if (type.basetype != spirv_cross::SPIRType::Struct)
				throw std::runtime_error(fmt::format(
					"get_dependant_structs called with non struct type {}", type.basetype));
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

		auto path_filename(const char *p) {
			int s = '/';
			#ifdef WIN32
			s = '\\';
			#endif
			auto t = strrchr(p, s);
			return t == nullptr ? p : t + 1;
		}

		bool is_file_match(const string &name, const string &data) {
			std::ifstream str(name);
			str.seekg(0, str.end);
			size_t l = str.tellg();
			if (l != data.size())
				return false;
			str.seekg(0, str.beg);
			vector<char> t(l);
			str.read(t.data(), t.size());
			if (!str.good())
				return false;
			return std::equal(t.begin(), t.end(), data.begin());
		}

		void write_file(const string &name, const string &data) {
			if (is_file_match(name, data))
				return;
			std::ofstream str(name);
			str.write(data.data(), data.size());
			if (!str.good())
				throw std::runtime_error("error writing output to: " + name);
		}

	} // namespace

	int main(int ac, char *av[]) {
		auto prog = path_filename(av[0]);
		cxxopts::Options opts(prog, "automatic shader reflection to c++");

		opts
		.show_positional_help()
		.positional_help("input*")
		.add_options()
			("h,help", "print help")
			("i,input", "input spirv file (stdin if missing)", cxxopts::value<vector<string>>())
			("v,vertex", "name for generated vertex struct",
				cxxopts::value<string>()->default_value("Vertex"))
			("no-vertex", "suppress the generation of vertex structures")
			("no-source", "suppress the generation of static shader data variables")
			("namespace", "enclose the output in a namespace", cxxopts::value<vector<string>>())
			("d,data", "output shader data to a separate file", cxxopts::value<string>())
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
		std::map<uint32_t, DescriptorSet> descriptorSets;
		if (input.empty()) {
			shaders.emplace_back();
			auto &sh = shaders.back();
			sh.source = load_spirv("stdin", std::cin);
			sh.comp.reset(new spirv_cross::Compiler(sh.source));
			find_buffer_structs(sh.structs, *sh.comp);
			get_descriptor_sets(descriptorSets, *sh.comp);
		}
		for (auto i : input) {
			shaders.emplace_back();
			auto &sh = shaders.back();
			sh.source = load_spirv(i);
			sh.comp.reset(new spirv_cross::Compiler(sh.source));
			find_buffer_structs(sh.structs, *sh.comp);
			get_descriptor_sets(descriptorSets, *sh.comp);
		}

		// re-map potential name collisions
		map_struct_names(shaders);

		// Generate the output
		fmt::memory_buffer r;
		fmt::memory_buffer dr;
		format_to(r, "// generated with {}\n\n", prog);

		// figure out the namespace
		vector<string> namespaces;
		if (options.count("namespace") != 0)
			namespaces = options["namespace"].as<vector<string>>();
		auto indent = namespaces.empty() ? string() : "  ";

		if (!namespaces.empty()) {
			for (size_t i = 0; i < namespaces.size(); ++i)
				format_to(r, "{}namespace {} {{", i == 0 ? "" : " ", namespaces[i]);
			format_to(r, "\n\n");

			if (options.count("data")) {
				for (size_t i = 0; i < namespaces.size(); ++i)
					format_to(dr, "{}namespace {} {{", i == 0 ? "" : " ", namespaces[i]);
				format_to(dr, "\n\n");
			}
		}

		for (auto &sh : shaders) {
			for (auto t : sh.structs) {
				struct_definition(r, sh, t, indent);
				format_to(r, ";\n\n");
			}
		}

		bool withVertex = false;
		if (!options["no-vertex"].as<bool>()) {
			string vertname = options["vertex"].as<string>();
			for (auto &sh : shaders) {
				if (get_execution_model(*sh.comp) != spv::ExecutionModelVertex)
					continue;
				withVertex = get_vertex_definition(r, *sh.comp, vertname, indent);
			}
		}

		for (auto &s : descriptorSets) {
			descriptor_layout(r, s.second, descriptorSets.size() == 1 ? "DescriptorSet" :
				fmt::format("DescriptorSet{}", s.first), indent);
		}

		bool withPush = push_ranges(r, shaders, indent);

		if (!options["no-source"].as<bool>()) {
			shader_source_decl(r, shaders, indent);

			generate_components(r, descriptorSets, shaders, withVertex, withPush, indent);

			if (options.count("data") != 0)
				shader_source(dr, shaders, indent, false);
			else
				shader_source(r, shaders, indent, true);
		}

		if (!namespaces.empty()) {
			for (size_t i = 0; i < namespaces.size(); ++i)
				format_to(r, "{}}}", i == 0 ? "" : " ");
			format_to(r, "\n");

			if (options.count("data")) {
				for (size_t i = 0; i < namespaces.size(); ++i)
					format_to(dr, "{}}}", i == 0 ? "" : " ");
				format_to(dr, "\n");
			}
		}

		// wrte the output to the intended destination
		string out = to_string(r);
		string dout = to_string(dr);
		if (options.count("output") == 0) {
			std::cout.write(out.data(), out.size());
		}
		else {
			write_file(options["output"].as<string>(), out);
		}

		if (options.count("data")) {
			write_file(options["data"].as<string>(), dout);
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
