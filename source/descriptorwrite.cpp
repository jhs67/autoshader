//
//  File: descriptorwrite.cpp
//
//  Created by Jon Spencer on 2019-02-20 23:16:03
//  Copyright (c) Jon Spencer. See LICENSE file.
//

#include "descriptorwrite.h"

namespace autoshader {

	namespace {

		auto writerSrc =
R"({0}struct DescriptorSet{1}Writer {{
{0}  DescriptorSet{1}Writer(vk::DescriptorSet s) : descriptorSet(s), writeIndex(0){4} {{}}

{0}  vk::DescriptorSet descriptorSet;
{0}  vk::WriteDescriptorSet writes[{2}];
{0}  size_t writeIndex;
{3}{0}  void update(vk::Device d) {{
{0}    d.updateDescriptorSets(writeIndex, writes, 0, nullptr);
{0}  }}
{0}}};

{0}inline auto descriptorSet{1}Writer(vk::DescriptorSet s) {{
{0}  return DescriptorSet{1}Writer(s);
{0}}}

)";

		auto setUniformArgs = "vk::Buffer b, vk::DeviceSize o, vk::DeviceSize r = VK_WHOLE_SIZE";
		auto setUniformInfo = "vk::DescriptorBufferInfo{ b, r == VK_WHOLE_SIZE ? 0 : o, r == VK_WHOLE_SIZE ? o : r }";
		auto setUniformWrite = "nullptr, ";
		auto setUniformMember = "pBufferInfo";

		auto setBufferArgs = "vk::Buffer b, vk::DeviceSize o = 0, vk::DeviceSize r = VK_WHOLE_SIZE";
		auto setBufferInfo = "vk::DescriptorBufferInfo{ b, o, r }";
		auto setBufferWrite = "nullptr, ";
		auto setBufferMember = "pBufferInfo";

		auto setImageSamplerArgs = "vk::Sampler b, vk::ImageView i, vk::ImageLayout l = vk::ImageLayout::eShaderReadOnlyOptimal";
		auto setImageSamplerInfo = "vk::DescriptorImageInfo{ b, i, l }";
		auto setImageSamplerWrite = "";
		auto setImageSamplerMember = "pImageInfo";

		auto setSamplerArgs = "vk::Sampler b";
		auto setSamplerInfo = "vk::DescriptorImageInfo{ b }";
		auto setSamplerWrite = "";
		auto setSamplerMember = "pImageInfo";

		auto setImageArgs = "vk::ImageView i, vk::ImageLayout l = vk::ImageLayout::eGeneral";
		auto setImageInfo = "vk::DescriptorImageInfo{ {}, i, l }";
		auto setImageWrite = "";
		auto setImageMember = "pImageInfo";

		auto bufferInfoSrc = "{0}  vk::DescriptorBufferInfo di{1};\n";
		auto bufferInfoArraySrc = "{0}  static constexpr size_t {3}Size = {2};\n{0}  vk::DescriptorBufferInfo di{1}[{2}];\n{0}  size_t ic{1};\n";
		auto bufferInfoVectorSrc = "{0}  std::vector<vk::DescriptorBufferInfo> di{1};\n{0}  size_t ic{1};\n";

		auto imageInfoSrc = "{0}  vk::DescriptorImageInfo di{1};\n";
		auto imageInfoArraySrc = "{0}  static constexpr size_t {3}Size = {2};\n{0}  vk::DescriptorImageInfo di{1}[{2}];\n{0}  size_t ic{1};\n";
		auto imageInfoVectorSrc = "{0}  std::vector<vk::DescriptorImageInfo> di{1};\n{0}  size_t ic{1};\n";

		auto setSingleSrc =
R"({0}  DescriptorSet{1}Writer& set{2}({4}) {{
{0}    di{3} = {5};
{0}    if (writeIndex >= {6})
{0}      throw std::runtime_error("autoshader descriptor set writer overflow");
{0}    writes[writeIndex++] = {{ descriptorSet, {3}, 0, 1, {7}, {8}&di{3} }};
{0}    return *this;
{0}  }}

)";

		auto setArraySrc =
R"({0}  DescriptorSet{1}Writer& set{2}({4}) {{
{0}    if (ic{3} == size_t(-1)) {{
{0}      if (writeIndex >= {6})
{0}        throw std::runtime_error("autoshader descriptor set writer overflow");
{0}      ic{3} = writeIndex;
{0}      writes[writeIndex++] = {{ descriptorSet, {3}, 0, 0, {7}, {8}di{3} }};
{0}    }}
{0}    if (writes[ic{3}].descriptorCount >= {9})
{0}      throw std::runtime_error("autoshader descriptor set writer overflow");
{0}    di{3}[writes[ic{3}].descriptorCount++] = {5};
{0}    return *this;
{0}  }}

)";

		auto setVectorSrc =
R"({0}  DescriptorSet{1}Writer& set{2}({4}) {{
{0}    if (ic{3} == size_t(-1)) {{
{0}      if (writeIndex >= {6})
{0}        throw std::runtime_error("autoshader descriptor set writer overflow");
{0}      ic{3} = writeIndex;
{0}      writes[writeIndex++] = {{ descriptorSet, {3}, 0, 0, {7}, {8}nullptr }};
{0}    }}
{0}    di{3}.emplace_back({5});
{0}    writes[ic{3}].descriptorCount = uint32_t(di{3}.size());
{0}    writes[ic{3}].{10} = di{3}.data();
{0}    return *this;
{0}  }}

)";

		void set_generic_src(fmt::memory_buffer &r, const string &name, const string &indent,
				uint32_t set, const DescriptorRecord &d, size_t writeLimit) {

			// get the type specific parts
			const char *argf, *infof, *writef, *memberf;
			switch (d.type) {
				case DescriptorType::Sampler:
					argf = setSamplerArgs;
					infof = setSamplerInfo;
					writef = setSamplerWrite;
					memberf = setSamplerMember;
					break;
				case DescriptorType::ImageSampler:
					argf = setImageSamplerArgs;
					infof = setImageSamplerInfo;
					writef = setImageSamplerWrite;
					memberf = setImageSamplerMember;
					break;
				case DescriptorType::SampledImage:
				case DescriptorType::StorageImage:
					argf = setImageArgs;
					infof = setImageInfo;
					writef = setImageWrite;
					memberf = setImageMember;
					break;
				case DescriptorType::Uniform:
					argf = setUniformArgs;
					infof = setUniformInfo;
					writef = setUniformWrite;
					memberf = setUniformMember;
					break;
				case DescriptorType::StorageBuffer:
					argf = setBufferArgs;
					infof = setBufferInfo;
					writef = setBufferWrite;
					memberf = setBufferMember;
					break;
			}

			// get the array type setter
			const char *srcf;
			if (d.arraysize == 1)
				srcf = setSingleSrc;
			else if (d.arraysize == 0)
				srcf = setVectorSrc;
			else
				srcf = setArraySrc;

			// format the source
			format_to(std::back_inserter(r), srcf, indent, name, d.name, set, argf, infof, writeLimit,
				vulkan_descriptor_type(d.type), writef, d.arraysize, memberf);
		}


		//-------------------------------------------------------------------------------------------
		//-- write out writer for a single descriptor set

		void descriptor_writer(fmt::memory_buffer &r, const DescriptorSet &set,
				const string &name, const string &indent) {

			fmt::memory_buffer b;
			fmt::memory_buffer i;
			for (auto &d : set.descriptors) {
				switch (d.second.type) {
					case DescriptorType::Sampler:
					case DescriptorType::ImageSampler:
					case DescriptorType::SampledImage:
					case DescriptorType::StorageImage:
						if (d.second.arraysize == 1) {
							format_to(std::back_inserter(b), imageInfoSrc, indent, d.first);
						}
						else if (d.second.arraysize == 0) {
							format_to(std::back_inserter(b), imageInfoVectorSrc, indent, d.first);
							format_to(std::back_inserter(i), ", ic{}(size_t(-1))", d.first);
						}
						else {
							format_to(std::back_inserter(b), imageInfoArraySrc, indent, d.first, d.second.arraysize, d.second.name);
							format_to(std::back_inserter(i), ", ic{}(size_t(-1))", d.first);
						}
						break;
					case DescriptorType::Uniform:
					case DescriptorType::StorageBuffer:
						if (d.second.arraysize == 1) {
							format_to(std::back_inserter(b), bufferInfoSrc, indent, d.first);
						}
						else if (d.second.arraysize == 0) {
							format_to(std::back_inserter(b), bufferInfoVectorSrc, indent, d.first);
							format_to(std::back_inserter(i), ", ic{}(size_t(-1))", d.first);
						}
						else {
							format_to(std::back_inserter(b), bufferInfoArraySrc, indent, d.first, d.second.arraysize, d.second.name);
							format_to(std::back_inserter(i), ", ic{}(size_t(-1))", d.first);
						}
						break;
				}
			}
			format_to(std::back_inserter(b), "\n");
			for (auto &d : set.descriptors) {
				set_generic_src(b, name, indent, d.first, d.second,
				set.descriptors.size());
			}

			format_to(std::back_inserter(r), writerSrc, indent, name, set.descriptors.size(), to_string(b),
				to_string(i));
		}

	}


	//-------------------------------------------------------------------------------------------
	//-- write out utility methods for writing descriptor set updates.

	void descriptor_writer(fmt::memory_buffer &r, const std::map<uint32_t, DescriptorSet> &set,
			const string &indent) {
		for (auto &i : set) {
			descriptor_writer(r, i.second, set.size() == 1 ? "" :
				fmt::format("{}", i.first), indent);
		}
	}

} // namespace autoshader
