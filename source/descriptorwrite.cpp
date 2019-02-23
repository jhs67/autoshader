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
{0}  DescriptorSet{1}Writer(vk::DescriptorSet s) : descriptorSet(s), writeIndex(0) {{}}

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

		auto setUniformSrc =
R"({0}  DescriptorSet{1}Writer& set{2}(vk::Buffer b, vk::DeviceSize o, vk::DeviceSize r = VK_WHOLE_SIZE) {{
{0}    di{3} = vk::DescriptorBufferInfo{{ b, r == VK_WHOLE_SIZE ? 0 : o, r == VK_WHOLE_SIZE ? o : r }};
{0}    if (writeIndex >= {5})
{0}      throw std::runtime_error("autoshader descriptor set writer overflow");
{0}    writes[writeIndex++] = {{ descriptorSet, {3}, 0, 1, {4}, nullptr, &di{3} }};
{0}    return *this;
{0}  }}
)";

		auto setBufferSrc =
R"({0}  DescriptorSet{1}Writer& set{2}(vk::Buffer b, vk::DeviceSize o = 0, vk::DeviceSize r = VK_WHOLE_SIZE) {{
{0}    di{3} = vk::DescriptorBufferInfo{{ b, o, r }};
{0}    if (writeIndex >= {5})
{0}      throw std::runtime_error("autoshader descriptor set writer overflow");
{0}    writes[writeIndex++] = {{ descriptorSet, {3}, 0, 1, {4}, nullptr, &di{3} }};
{0}    return *this;
{0}  }}
)";

		auto setImageSamplerSrc =
R"({0}  DescriptorSet{1}Writer& set{2}(vk::Sampler b, vk::ImageView i, vk::ImageLayout l = vk::ImageLayout::eShaderReadOnlyOptimal) {{
{0}    di{3} = vk::DescriptorImageInfo{{ b, i, l }};
{0}    if (writeIndex >= {5})
{0}      throw std::runtime_error("autoshader descriptor set writer overflow");
{0}    writes[writeIndex++] = {{ descriptorSet, {3}, 0, 1, {4}, &di{3} }};
{0}    return *this;
{0}  }}
)";

		auto setSamplerSrc =
R"({0}  DescriptorSet{1}Writer& set{2}(vk::Sampler b) {{
{0}    di{3} = vk::DescriptorImageInfo{{ b }};
{0}    if (writeIndex >= {5})
{0}      throw std::runtime_error("autoshader descriptor set writer overflow");
{0}    writes[writeIndex++] = {{ descriptorSet, {3}, 0, 1, {4}, &di{3} }};
{0}    return *this;
{0}  }}
)";

		auto setImageSrc =
R"({0}  DescriptorSet{1}Writer& set{2}(vk::ImageView i, vk::ImageLayout l = vk::ImageLayout::eGeneral) {{
{0}    di{3} = vk::DescriptorImageInfo{{ {{}}, i, l }};
{0}    if (writeIndex >= {5})
{0}      throw std::runtime_error("autoshader descriptor set writer overflow");
{0}    writes[writeIndex++] = {{ descriptorSet, {3}, 0, 1, {4}, &di{3} }};
{0}    return *this;
{0}  }}
)";

		auto bufferInfoSrc = "{0}  vk::DescriptorBufferInfo di{1};\n";

		auto imageInfoSrc = "{0}  vk::DescriptorImageInfo di{1};\n";


		//-------------------------------------------------------------------------------------------
		//-- write out writer for a single descriptor set

		void descriptor_writer(fmt::memory_buffer &r, const DescriptorSet &set,
				const string &name, const string &indent) {

			fmt::memory_buffer b;
			for (auto &d : set.descriptors) {
				switch (d.second.type) {
					case DescriptorType::Sampler:
					case DescriptorType::ImageSampler:
					case DescriptorType::SampledImage:
					case DescriptorType::StorageImage:
						format_to(b, imageInfoSrc, indent, d.first);
						break;
					case DescriptorType::Uniform:
					case DescriptorType::StorageBuffer:
						format_to(b, bufferInfoSrc, indent, d.first);
						break;
				}
			}
			format_to(b, "\n");
			for (auto &d : set.descriptors) {
				switch (d.second.type) {
					case DescriptorType::Sampler:
						format_to(b, setSamplerSrc, indent, name, d.second.name, d.first,
							vulkan_descriptor_type(d.second.type), set.descriptors.size());
						break;
					case DescriptorType::ImageSampler:
						format_to(b, setImageSamplerSrc, indent, name, d.second.name, d.first,
							vulkan_descriptor_type(d.second.type), set.descriptors.size());
						break;
					case DescriptorType::SampledImage:
					case DescriptorType::StorageImage:
						format_to(b, setImageSrc, indent, name, d.second.name, d.first,
							vulkan_descriptor_type(d.second.type), set.descriptors.size());
						break;
					case DescriptorType::Uniform:
						format_to(b, setUniformSrc, indent, name, d.second.name, d.first,
							vulkan_descriptor_type(d.second.type), set.descriptors.size());
						break;
					case DescriptorType::StorageBuffer:
						format_to(b, setBufferSrc, indent, name, d.second.name, d.first,
							vulkan_descriptor_type(d.second.type), set.descriptors.size());
						break;
				}
			}

			format_to(r, writerSrc, indent, name, set.descriptors.size(), to_string(b));
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
