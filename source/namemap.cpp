//
//  File: namemap.cpp
//
//  Created by Jon Spencer on 2019-02-10 12:11:39
//  Copyright (c) Jon Spencer. See LICENSE file.
//

#include "namemap.h"
#include "shadersource.h"
#include <set>

namespace autoshader {

	namespace {

		void struct_signature(fmt::memory_buffer &r, spirv_cross::Compiler &comp, uint32_t t);


		//------------------------------------------------------------------------------------------
		//-- StructIndex: the shader and type of a structure

		struct StructIndex {
			size_t sh;
			uint32_t st;
		};

		//------------------------------------------------------------------------------------------
		//-- assign structure names to all the conflcting signatures

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


		//------------------------------------------------------------------------------------------
		//-- return the signature string for a type

		void type_signature(fmt::memory_buffer &r, spirv_cross::Compiler &comp, uint32_t t) {
			using spirv_cross::SPIRType;
			auto type = comp.get_type(t);
			if (type.basetype == SPIRType::Struct) {
				format_to(std::back_inserter(r), "{{");
				struct_signature(r, comp, type.self);
				format_to(std::back_inserter(r), "}}");
			}
			else {
				format_to(std::back_inserter(r), "{}", type_string(comp, type));
			}
		}

		//------------------------------------------------------------------------------------------
		//-- format the struct signature into a buffer

		void struct_signature(fmt::memory_buffer &r, spirv_cross::Compiler &comp, uint32_t t) {
			using spirv_cross::SPIRType;
			auto type = comp.get_type(t);
			if (type.basetype != SPIRType::Struct)
				throw std::runtime_error("struct_signature called on non-struct");

			format_to(std::back_inserter(r), "{},{:09d}:", comp.get_name(t), comp.get_declared_struct_size(type));

			for (uint32_t i = 0; size_t(i) < type.member_types.size(); ++i) {
				auto mtype = comp.get_type(type.member_types[i]);
				format_to(std::back_inserter(r), "{},", comp.get_member_name(t, i));
				type_signature(r, comp, type.member_types[i]);
				format_to(std::back_inserter(r), ",{}", comp.type_struct_member_offset(type, i));
				if (mtype.columns > 1)
					format_to(std::back_inserter(r), ";{}", comp.type_struct_member_matrix_stride(type, i));
				if (!mtype.array.empty())
					format_to(std::back_inserter(r), ",{}", comp.type_struct_member_array_stride(type, i));
				for (auto t : mtype.array)
					format_to(std::back_inserter(r), ",{}", t);
				format_to(std::back_inserter(r), ":");
			}
		}

		//------------------------------------------------------------------------------------------
		//-- return the signature string for a structure type

		string struct_signature(spirv_cross::Compiler &comp, uint32_t t) {
			fmt::memory_buffer r;
			struct_signature(r, comp, t);
			return to_string(r);
		}

	} // namespace

	//------------------------------------------------------------------------------------------
	//-- re-map all conflicting structure names

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
					"_" + get_execution_string(*sh[i].comp));
			}
		}
	}

} // namespace autoshader
