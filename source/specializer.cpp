//
//  File: specializer.cpp
//
//  Created by Jon Spencer on 2019-05-14 11:36:50
//  Copyright (c) Jon Spencer. See LICENSE file.
//

#include "specializer.h"
#include "shadersource.h"
#include "descriptorset.h"

namespace autoshader {

	namespace {

		auto writerSrc =
R"({0}struct {1}Specializer {{
{0}  int32_t values[{2}];
{0}  vk::SpecializationMapEntry map[{2}];
{0}  uint32_t mapIndex;

{0}  {1}Specializer() : mapIndex(0) {{}}

{0}  std::pair<vk::ShaderStageFlags,vk::SpecializationInfo> info() {{
{0}    return std::make_pair({3},
{0}      vk::SpecializationInfo{{ mapIndex, map, mapIndex * sizeof(int32_t), values }});
{0}  }}
)";

		auto intSrc =
R"(
{0}  {5}Specializer& set{1}({3} v) {{
{0}      if (mapIndex >= {2})
{0}        throw std::runtime_error("autoshader specializer overflow");
{0}    values[mapIndex] = int32_t(v);
{0}    map[mapIndex] = vk::SpecializationMapEntry{{ {4}, uint32_t(mapIndex * sizeof(int32_t)), sizeof(int32_t) }};
{0}    ++mapIndex;
{0}    return *this;
{0}  }}
)";

		auto floatSrc =
R"(
{0}  {5}Specializer& set{1}({3} v) {{
{0}      if (mapIndex >= {2})
{0}        throw std::runtime_error("autoshader specializer overflow");
{0}    memcpy(&values[mapIndex], &v, sizeof(int32_t));
{0}    map[mapIndex] = vk::SpecializationMapEntry{{ {4}, uint32_t(mapIndex * sizeof(int32_t)), sizeof(int32_t) }};
{0}    ++mapIndex;
{0}    return *this;
{0}  }}
)";

		string capitalize(string s) {
			if (s.size() > 0)
				s[0] = toupper(s[0]);
			return s;
		}

		void specializer_value(fmt::memory_buffer &r, spirv_cross::Compiler &comp,
				const string &indent, size_t count, uint32_t id, const string &name,
				uint32_t constantID, const string &specname) {

			auto type = comp.get_type(comp.get_constant(id).constant_type);
			if (type.basetype == spirv_cross::SPIRType::Float) {
				format_to(std::back_inserter(r), floatSrc, indent, name, count, type_string(comp, type),
					constantID, specname);
			}
			else {
				format_to(std::back_inserter(r), intSrc, indent, name, count, type_string(comp, type),
					constantID, specname);
			}
		}

		void shader_specializer(fmt::memory_buffer &r, spirv_cross::Compiler &comp, string pre,
				const string &indent) {

			struct SpecToWrite {
				uint32_t id;
				string name;
			};

			// grab all the names specialization constants
			std::map<uint32_t, SpecToWrite> specs;
			auto consts = comp.get_specialization_constants();
			for (auto &c : consts) {
				auto n = comp.get_name(c.id);
				if (n.empty())
					continue;
				specs.emplace(c.constant_id, SpecToWrite{ c.id, move(n) });
			}

			// group the workgroup size constants
			spirv_cross::SpecializationConstant x, y, z;
			comp.get_work_group_size_specialization_constants(x, y, z);
			if (uint32_t(x.id) != 0 || x.constant_id != 0)
				specs.emplace(x.constant_id, SpecToWrite{ x.id, "WorkGroupSizeX" });
			if (uint32_t(y.id) != 0 || y.constant_id != 0)
				specs.emplace(y.constant_id, SpecToWrite{ y.id, "WorkGroupSizeY" });
			if (uint32_t(z.id) != 0 || z.constant_id != 0)
				specs.emplace(z.constant_id, SpecToWrite{ z.id, "WorkGroupSizeZ" });

			// common case - no specialization constant
			if (specs.empty())
				return;

			format_to(std::back_inserter(r), writerSrc, indent, capitalize(pre), specs.size(),
				get_shader_stage_flags(comp));

			for (auto &t : specs) {
				specializer_value(r, comp, indent, specs.size(),
					t.second.id, t.second.name, t.first, capitalize(pre));
			}

			format_to(std::back_inserter(r), "{0}}};\n\n", indent);
		}
	}

	//-------------------------------------------------------------------------------------------
	//-- write out utility methods for creating vk::SpecializationInfo records.

	void specializers(fmt::memory_buffer &r, vector<ShaderRecord> &sh,
			const string &indent) {

		for (auto &s : sh) {
			shader_specializer(r, *s.comp, sh.size() == 1 ? "" : get_execution_string(*s.comp),
				indent);
		}
	}

} // namespace autoshader
