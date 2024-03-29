//
//  File: component.cpp
//
//  Created by Jon Spencer on 2019-02-10 13:17:29
//  Copyright (c) Jon Spencer. See LICENSE file.
//

#include "component.h"
#include "shadersource.h"

namespace autoshader {

	void generate_components(fmt::memory_buffer &r,
		std::map<uint32_t, DescriptorSet> &sets, vector<ShaderRecord> &sh,
		bool withVertex, bool withPush, const string &indent) {

		size_t arity = 0;
		fmt::memory_buffer a;
		std::map<uint32_t, string> setargs;
		for (auto &s : sets) {
			bool e = false;
			fmt::memory_buffer b;
			for (auto &d : s.second.descriptors) {
				if (d.second.arraysize == 0) {
					format_to(std::back_inserter(a), ", int32_t a{}", arity);
					format_to(std::back_inserter(b), "{}a{}", e ? ", " : "", arity);
					arity += 1;
					e = true;
				}
			}
			setargs[s.first] = to_string(b);
		}

		format_to(std::back_inserter(r), "{}struct Components {{\n", indent);
		format_to(std::back_inserter(r), "{}  static constexpr size_t arity = {};\n\n", indent, arity);
		format_to(std::back_inserter(r), "{}  Components() {{}}\n", indent);
		format_to(std::back_inserter(r), "{}  Components(Components &&o) noexcept {{ swap(o); }}\n", indent);
		format_to(std::back_inserter(r), "{}  Components &operator = (Components &&o) noexcept {{ swap(o); return *this; }}\n", indent);
		format_to(std::back_inserter(r), "\n");

		format_to(std::back_inserter(r), "{}  Components(vk::Device d{}) {{\n", indent, to_string(a));
		for (auto &s : sets) {
			auto sn = sets.size() == 1 ? "" : fmt::format("{}", s.first);
			format_to(std::back_inserter(r), "{0}    auto lb{1} = getDescriptorSet{1}LayoutBindings({2});\n",
				indent, sn, setargs[s.first]);
			format_to(std::back_inserter(r), "{0}    auto slb{1} = d.createDescriptorSetLayoutUnique({{ {{}}, uint32_t(lb{1}.size()), lb{1}.data() }});\n", indent, sn);
		}
		if (withPush) {
			format_to(std::back_inserter(r), "{0}    auto pr = getPushConstantRanges();\n", indent);
		}
		if (!sets.empty()) {
			bool c = false;
			format_to(std::back_inserter(r), "{}    auto l = std::array<vk::DescriptorSetLayout,{}>({{{{", indent, sets.size());
			for (auto &s : sets) {
				auto sn = sets.size() == 1 ? "" : fmt::format("{}", s.first);
				format_to(std::back_inserter(r), "{} *slb{}", c ? "," : "", sn);
				c = true;
			}
			format_to(std::back_inserter(r), " }}}});\n", indent, sets.size());
			format_to(std::back_inserter(r), "{}    auto pl = d.createPipelineLayoutUnique({{ {{}}, {}, l.data()",
				indent, sets.size());
			if (withPush) {
				format_to(std::back_inserter(r), ", pr.size(), pr.data() }});\n");
			}
			else {
				format_to(std::back_inserter(r), " }});\n");
			}
		}
		else if (withPush) {
			format_to(std::back_inserter(r), "{}    auto pl = d.createPipelineLayoutUnique({{ {{}}, 0, nullptr, pr.size(), pr.data() }});\n", indent);
		}
		else {
			format_to(std::back_inserter(r), "{}    auto pl = d.createPipelineLayoutUnique({{}});\n", indent);
		}
		for (auto &s : sh) {
			auto sn = get_execution_string(*s.comp);
			format_to(std::back_inserter(r), "{0}    auto {1}_ = d.createShaderModuleUnique({{ {{}}, {1}_size, {1}_data }});\n", indent, sn);
		}
		format_to(std::back_inserter(r), "{}    device = d;\n", indent);
		for (auto &s : sets) {
			auto sn = sets.size() == 1 ? "" : fmt::format("{}", s.first);
			format_to(std::back_inserter(r), "{0}    set{1}Layout = slb{1}.release();\n", indent, sn);
		}
		format_to(std::back_inserter(r), "{}    layout = pl.release();\n", indent);
		for (auto &s : sh) {
			auto sn = get_execution_string(*s.comp);
			format_to(std::back_inserter(r), "{0}    {1} = {1}_.release();\n", indent, sn);
		}
		format_to(std::back_inserter(r), "{}  }}\n", indent);

		format_to(std::back_inserter(r), "\n");
		format_to(std::back_inserter(r), "{}  ~Components() {{\n", indent);
		format_to(std::back_inserter(r), "{}    if (device == vk::Device())\n", indent);
		format_to(std::back_inserter(r), "{}      return;\n", indent);
		for (auto &s : sh) {
			auto sn = get_execution_string(*s.comp);
			format_to(std::back_inserter(r), "{}    device.destroyShaderModule({});\n", indent, sn);
		}
		format_to(std::back_inserter(r), "{}    device.destroyPipelineLayout(layout);\n", indent);
		for (auto &s : sets) {
			auto sn = sets.size() == 1 ? "" : fmt::format("{}", s.first);
			format_to(std::back_inserter(r), "{0}    device.destroyDescriptorSetLayout(set{1}Layout);\n", indent, sn);
		}
		format_to(std::back_inserter(r), "{}  }}\n", indent);

		format_to(std::back_inserter(r), "\n");
		format_to(std::back_inserter(r), "{}  template <typename... A>\n", indent);
		format_to(std::back_inserter(r), "{}  vk::UniquePipeline createPipe(A &&...a)  {{\n", indent);
		format_to(std::back_inserter(r), "{}    return autoshader::createPipe(std::forward<A>(a)..., device, layout", indent);
		for (auto &s : sh) {
			auto sn = get_execution_string(*s.comp);
			format_to(std::back_inserter(r), ",\n{0}      vk::PipelineShaderStageCreateInfo({{}}, {2}, {1}, \"{3}\")",
				indent, sn, get_shader_stage_flags(*s.comp), get_first_entry_point_name(*s.comp));
		}
		if (withVertex) {
			format_to(std::back_inserter(r), ",\n{}      getVertexBindingDescription(), getVertexAttributeDescriptions()",
				indent);
		}
		format_to(std::back_inserter(r), ");\n");
		format_to(std::back_inserter(r), "{}  }}\n", indent);

		format_to(std::back_inserter(r), "\n");
		format_to(std::back_inserter(r), "{}  void swap(Components &o) noexcept {{\n", indent);
		format_to(std::back_inserter(r), "{}    std::swap(device, o.device);\n", indent);
		for (auto &s : sets) {
			auto sn = sets.size() == 1 ? "" : fmt::format("{}", s.first);
			format_to(std::back_inserter(r), "{0}    std::swap(set{1}Layout, o.set{1}Layout);\n", indent, sn);
		}
		format_to(std::back_inserter(r), "{}    std::swap(layout, o.layout);\n", indent);
		for (auto &s : sh) {
			auto sn = get_execution_string(*s.comp);
			format_to(std::back_inserter(r), "{0}    std::swap({1}, o.{1});\n", indent, sn);
		}
		format_to(std::back_inserter(r), "{}  }}\n", indent);

		format_to(std::back_inserter(r), "\n");
		format_to(std::back_inserter(r), "{}  vk::Device device;\n", indent);
		for (auto &s : sets) {
			auto sn = sets.size() == 1 ? "" : fmt::format("{}", s.first);
			format_to(std::back_inserter(r), "{0}  vk::DescriptorSetLayout set{1}Layout;\n", indent, sn);
		}
		format_to(std::back_inserter(r), "{}  vk::PipelineLayout layout;\n", indent);
		for (size_t i = 0; i < sh.size(); ++i) {
			auto sn = get_execution_string(*sh[i].comp);
			format_to(std::back_inserter(r), "{0}  vk::ShaderModule {1};\n", indent, sn);
		}
		format_to(std::back_inserter(r), "{}}};\n\n", indent);
	}


} // namespace autoshader
