//
//  File: createpipe.h
//
//  Created by Jon Spencer on 2019-02-12 11:42:13
//  Copyright (c) Jon Spencer. See LICENSE file.
//
#ifndef H_AUTOSHADER_CREATEPIPE_H__
#define H_AUTOSHADER_CREATEPIPE_H__

#include "anyarg.h"
#include "vulkan/vulkan.hpp"

namespace autoshader {

	//----------------------------------------------------------------------------------------
	//-- createComputePipe - create a compute pipeline
	//--   required: vk::Device, vk::PipelineLayout
	//--     also one of vk::PipelineShaderStageCreateInfo or vk::ShaderModule
	//--   options vk::PipelineCreateFlags, vk::PipelineCache

	template <typename... A>
	vk::UniquePipeline createComputePipe(A &&...a) {
		using anyarg::Arg;
		// You have to pass in this stuff
		static_assert(Arg<vk::Device>::contains<A...>(),
			"createComputePipe needs a vk::Device argument");
		static_assert(Arg<vk::PipelineLayout>::contains<A...>(),
			"createGraphicsPipe needs a vk::PipelineLayout argument");
		static_assert(Arg<vk::ShaderModule>::contains<A...>() ||
			Arg<vk::PipelineShaderStageCreateInfo>::contains<A...>(),
			"need a stage or shader module for a compute pipeline");

		// flags
	 	auto flags = Arg<vk::PipelineCreateFlags>::dget({}, std::forward<A>(a)...);

	 	// check for shader module and entry name
		auto module = Arg<vk::ShaderModule>::dget({}, std::forward<A>(a)...);
		auto entry = Arg<std::string>::dget(Arg<const char*>::dget("main",
			std::forward<A>(a)...), std::forward<A>(a)...);

		// get the compute stage
		auto stage = Arg<vk::PipelineShaderStageCreateInfo>::dget({ {},
			vk::ShaderStageFlagBits::eCompute, module, entry.c_str() }, std::forward<A>(a)...);

		// pipeline layout
		auto layout = Arg<vk::PipelineLayout>::get(std::forward<A>(a)...);

		// optional cache
		auto cache = Arg<vk::PipelineCache>::dget(vk::PipelineCache{}, std::forward<A>(a)...);

		// device to create pipeline
		auto dev = Arg<vk::Device>::get(std::forward<A>(a)...);
		assert(dev != vk::Device{});

		return dev.createComputePipelineUnique(cache, { flags, stage, layout });
	}

	struct SubPass {
		SubPass(uint32_t v = 0) : value(v) {}
		operator uint32_t () const { return value; }
		int value;
	};


	//----------------------------------------------------------------------------------------
	//-- createGraphicsPipe - create a compute pipeline

	template <typename... A>
	vk::UniquePipeline createGraphicsPipe(A &&...a) {
		using anyarg::Arg;
		// You have to pass in this stuff
		static_assert(Arg<vk::Device>::contains<A...>(),
			"createGraphicsPipe needs a vk::Device argument");
		static_assert(Arg<vk::PipelineLayout>::contains<A...>(),
			"createGraphicsPipe needs a vk::PipelineLayout argument");
		static_assert(Arg<vk::RenderPass>::contains<A...>(),
			"createGraphicsPipe needs a vk::RenderPass argument");

		// get the flags and stages
	 	auto flags = Arg<vk::PipelineCreateFlags>::dget({}, std::forward<A>(a)...);
		auto stages = Arg<vk::PipelineShaderStageCreateInfo>::gather(std::forward<A>(a)...);
		static_assert(stages.size() > 1, "need at least two stages for a graphics pipeline");

		// gather any bindings and attributes passed by the user.
		auto bin = Arg<vk::VertexInputBindingDescription>::gather(std::forward<A>(a)...);
		auto att = Arg<vk::VertexInputAttributeDescription>::gather(std::forward<A>(a)...);

		// grab the vertex input from the args or from the bindings and attrs.
		auto vis = Arg<vk::PipelineVertexInputStateCreateInfo>::dget(
			{ {}, uint32_t(bin.size()), bin.data(), uint32_t(att.size()), att.data() },
			std::forward<A>(a)...);

		// get the input assembly
		auto topology = Arg<vk::PrimitiveTopology>::dget(vk::PrimitiveTopology::eTriangleStrip,
			std::forward<A>(a)...);
		auto ass = Arg<vk::PipelineInputAssemblyStateCreateInfo>::dget(
			{ {}, topology, false }, std::forward<A>(a)...);

		// optional tessellation state
		auto ptes = Arg<vk::PipelineTessellationStateCreateInfo>::pget(std::forward<A>(a)...);

		// scissor and viewport
		vk::Rect2D scissor;
		vk::Viewport viewport;
		constexpr bool withviewport = Arg<vk::Rect2D>::contains<A...>() ||
			Arg<vk::Extent2D>::contains<A...>() ||
			Arg<vk::Viewport>::contains<A...>() ||
			Arg<vk::PipelineViewportStateCreateInfo>::contains<A...>();
		if (withviewport) {
			// default the viewport from the scissor or vice-versa
			auto ext = Arg<vk::Extent2D>::dget(scissor.extent, std::forward<A>(a)...);
			scissor = Arg<vk::Rect2D>::dget(vk::Rect2D({}, ext), std::forward<A>(a)...);
			viewport = Arg<vk::Viewport>::dget({
				float(scissor.offset.x), float(scissor.offset.y),
				float(scissor.extent.width), float(scissor.extent.height), 0, 1 },
				std::forward<A>(a)...);
			scissor = Arg<vk::Rect2D>::dget(vk::Rect2D(
				{ int32_t(viewport.x), int32_t(viewport.y) },
				{ uint32_t(viewport.width), uint32_t(viewport.height) }),
				std::forward<A>(a)...);
		}
		auto vps = Arg<vk::PipelineViewportStateCreateInfo>::dget({ {},
			1, &viewport, 1, &scissor }, std::forward<A>(a)...);

		// resterization state
		auto ras = Arg<vk::PipelineRasterizationStateCreateInfo>::dget({ {}, false, false,
			Arg<vk::PolygonMode>::dget(vk::PolygonMode::eFill, std::forward<A>(a)...),
			Arg<vk::CullModeFlags>::dget(vk::CullModeFlagBits::eNone, std::forward<A>(a)...),
			Arg<vk::FrontFace>::dget(vk::FrontFace::eClockwise, std::forward<A>(a)...),
			0, 0, 0, 0, 1 }, std::forward<A>(a)...);

		// multi-sampling
		auto mul = Arg<vk::PipelineMultisampleStateCreateInfo>::dget({ {},
			Arg<vk::SampleCountFlagBits>::dget(vk::SampleCountFlagBits::e1,
				std::forward<A>(a)...)
			}, std::forward<A>(a)...);

		// depth test
		auto dep = Arg<vk::PipelineDepthStencilStateCreateInfo>::dget({ {}, true, true,
			Arg<vk::CompareOp>::dget(vk::CompareOp::eLess, std::forward<A>(a)...) },
			std::forward<A>(a)...);

		// color blending
		auto cba = Arg<vk::PipelineColorBlendAttachmentState>::gather(std::forward<A>(a)...);
		vk::PipelineColorBlendAttachmentState dcba{
			true, vk::BlendFactor::eSrcAlpha, vk::BlendFactor::eOneMinusSrcAlpha,
			vk::BlendOp::eAdd, vk::BlendFactor::eOne,
			vk::BlendFactor::eZero, vk::BlendOp::eAdd,
			vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG
				| vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA
		};
		auto col = Arg<vk::PipelineColorBlendStateCreateInfo>::dget({ {}, false,
			vk::LogicOp::eClear, cba.empty() ? 1 : cba.size(),
			cba.empty() ? &dcba : cba.data() }, std::forward<A>(a)...);

		// grab the optional dynamic state
		auto pdyn = Arg<vk::PipelineDynamicStateCreateInfo>::pget(std::forward<A>(a)...);

		// pipeline layout
		auto lay = Arg<vk::PipelineLayout>::get(std::forward<A>(a)...);

		// render and subpass
		auto pas = Arg<vk::RenderPass>::get(std::forward<A>(a)...);
		auto sub = Arg<SubPass>::dget(0, std::forward<A>(a)...);

		// optional cache
		auto cache = Arg<vk::PipelineCache>::dget(vk::PipelineCache{}, std::forward<A>(a)...);

		// device to create pipeline
		auto dev = Arg<vk::Device>::get(std::forward<A>(a)...);
		assert(dev != vk::Device{});

		return dev.createGraphicsPipelineUnique(cache, { flags, stages.size(), stages.data(),
			&vis, &ass, ptes, &vps, &ras, &mul, &dep, &col, pdyn, lay, pas, sub });
	}


	//----------------------------------------------------------------------------------------
	//-- test if a parameter pack is for a compute or graphics pipeline

	template <typename... A>
	constexpr bool is_compute_pack() {
		using anyarg::Arg;
		return Arg<vk::ShaderModule>::contains<A...>() ||
			(Arg<vk::PipelineShaderStageCreateInfo>::contains<A...>() &&
				Arg<vk::PipelineShaderStageCreateInfo>::count<A...>() == 1);
	}


	//----------------------------------------------------------------------------------------
	//-- createPipe: create a compute or graphics pipeline

	template <typename... A, std::enable_if_t<is_compute_pack<A...>(), int> = 0>
	vk::UniquePipeline createPipe(A &&...a) {
		// single stage means a compute pipeline
		return createComputePipe(std::forward<A>(a)...);
	}

	template <typename... A, std::enable_if_t<!is_compute_pack<A...>(), int> = 0>
	vk::UniquePipeline createPipe(A &&...a) {
		// multiple stages means a graphics pipeline
		return createGraphicsPipe(std::forward<A>(a)...);
	}

} // namespace autoshader

#endif // H_AUTOSHADER_CREATEPIPE_H__
