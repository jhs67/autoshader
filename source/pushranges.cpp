//
//  File: pushranges.cpp
//
//  Created by Jon Spencer on 2019-02-19 10:52:13
//  Copyright (c) Jon Spencer. See LICENSE file.
//

#include "pushranges.h"
#include "descriptorset.h"
#include <set>

namespace autoshader {

	namespace {

		struct Range {
			uint32_t start, end;
		};

		struct FlagRange {
			uint32_t start, end;
			std::set<spv::ExecutionModel> stages;
		};

		typedef vector<Range> Ranges;
		typedef std::map<spv::ExecutionModel, Ranges> RangeMap;

		uint32_t nextRangeAtOrAfter(const Ranges &r, uint32_t at) {
			for (auto &i : r) {
				if (i.start >= at)
					return i.start;
				if (i.end >= at)
					return i.end;
			}
			return ~0u;
		}

		uint32_t nextRangeAtOrAfter(const RangeMap &rangemap, uint32_t at) {
			uint32_t a = ~0u;
			for (auto &i : rangemap)
				a = std::min(a, nextRangeAtOrAfter(i.second, at));
			return a;
		}

		bool active_at_spot(const Ranges &r, uint32_t at) {
			for (auto &i : r) {
				if (i.end <= at)
					continue;
				return i.start <= at;
			}
			return false;
		}

		//------------------------------------------------------------------------------------------
		// the size of the type as declared in c

		size_t push_type_size(spirv_cross::Compiler &comp, spirv_cross::SPIRType stype, uint32_t i) {
			using spirv_cross::SPIRType;

			auto type = comp.get_type(stype.member_types[i]);
			if (!type.array.empty())
				return type.array.front() * comp.type_struct_member_array_stride(stype, i);

			switch (type.basetype) {
				default:
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
					return comp.get_declared_struct_size(type);
			}

			throw std::runtime_error("invalid type for push_type_size");
		}

	}


	//------------------------------------------------------------------------------------------
	//-- format the push ranges into the buffer

	bool push_ranges(fmt::memory_buffer &r, vector<ShaderRecord> &sh, const string& indent) {
		// gather the range of used push constants for each shader stage
		std::map<spv::ExecutionModel, vector<Range>> rangemap;
		for (auto &s : sh) {
			auto res = s.comp->get_shader_resources();
			if (res.push_constant_buffers.empty())
				continue;
			auto em = get_execution_model(*s.comp);
			auto &pr = res.push_constant_buffers.front();
			auto type = s.comp->get_type(pr.base_type_id);
			uint32_t rangeStart = 0, rangeEnd = 0;
			for (uint32_t i = 0; size_t(i) < type.member_types.size(); ++i) {
				uint32_t o = s.comp->type_struct_member_offset(type, i);
				if (o != rangeEnd) {
					if (rangeEnd != 0)
						rangemap[em].push_back({ rangeStart, rangeEnd });
					rangeStart = o;
				}
				auto sz = push_type_size(*s.comp, type, i);
				rangeEnd = o + sz;
			}
			if (rangeEnd != 0)
				rangemap[em].push_back({ rangeStart, rangeEnd });
		}

		// no push contants
		if (rangemap.empty())
			return false;

		// find the start of the first range.
		vector<FlagRange> flagranges;
		for (uint32_t rangeStart = nextRangeAtOrAfter(rangemap, 0); rangeStart != ~0u;) {
			// find the next stop point
			auto rangeEnd = nextRangeAtOrAfter(rangemap, rangeStart + 1);
			if (rangeEnd == ~0u)
				break;

			// grab the stages active here
			std::set<spv::ExecutionModel> models;
			for (auto &i : rangemap) {
				if (active_at_spot(i.second, rangeStart))
					models.insert(i.first);
			}

			if (!models.empty()) {
				// store the range if active
				flagranges.emplace_back(FlagRange{ rangeStart, rangeEnd, move(models) });
			}

			// start again
			rangeStart = rangeEnd;
		}

		bool c = false;
		format_to(std::back_inserter(r), "{}inline auto getPushConstantRanges() {{\n", indent);
		format_to(std::back_inserter(r), "{}  return std::array<vk::PushConstantRange, {}>({{{{", indent,
			flagranges.size());
		for (auto &d : flagranges) {
			format_to(std::back_inserter(r), c ? ",\n" : "\n");
			format_to(std::back_inserter(r), "{}    {{ ", indent);
			vulkan_stage_flags(r, d.stages);
			format_to(std::back_inserter(r), ", {}, {}, ", d.start, d.end - d.start);
			format_to(std::back_inserter(r), " }}");
			c = true;
		}
		format_to(std::back_inserter(r), "\n{}  }}}});\n{}}}\n\n", indent, indent);

		return true;
	}

} // namespace autoshader
