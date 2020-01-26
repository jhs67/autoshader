//
//  File: create-pipe.cpp
//
//  Created by Jon Spencer on 2019-02-12 11:43:15
//  Copyright (c) Jon Spencer. See LICENSE file.
//

#include "catch2/catch.hpp"
#include "glm/glm.hpp"
#include "autoshader/createpipe.h"

namespace shader {

	using namespace glm;

	#define AUTOSHADER_SOURCE_DATA
	#include "create-pipe-autoshader.h"

}

TEST_CASE( "create-pipe" ) {

	SECTION( "create pipe" ) {

		vk::ApplicationInfo appinfo{ "create-pipe", 0x010000, "autoshader", 0x010000,
			VK_API_VERSION_1_0 };
		auto inst = vk::createInstanceUnique({ {}, &appinfo });
		auto phys = inst->enumeratePhysicalDevices();
		REQUIRE( phys.size() > 0 );
		float priority = 1.0f;
		auto que = vk::DeviceQueueCreateInfo{ {}, 0, 1, &priority };
		auto dev = phys[0].createDeviceUnique({ {}, 1, &que });

		vk::AttachmentDescription attachment{{}, vk::Format::eR8G8B8A8Unorm,
			vk::SampleCountFlagBits::e1, vk::AttachmentLoadOp::eClear,
			vk::AttachmentStoreOp::eStore, vk::AttachmentLoadOp::eDontCare,
			vk::AttachmentStoreOp::eDontCare, vk::ImageLayout::eTransferSrcOptimal,
			vk::ImageLayout::eTransferSrcOptimal };
		vk::AttachmentReference colorReference{ 0, vk::ImageLayout::eColorAttachmentOptimal };
		vk::SubpassDescription subpass({}, vk::PipelineBindPoint::eGraphics, 0, nullptr, 1,
			&colorReference, nullptr, nullptr);
		auto rp = dev->createRenderPassUnique({ {},
			1, &attachment,
			1, &subpass });

		// auto rp = dev->createRenderPassUnique({});
		REQUIRE( *rp != vk::RenderPass() );

		shader::Components pcomp(*dev);
		auto pipeline = pcomp.createPipe(*dev, *rp);
		REQUIRE( *pipeline != vk::Pipeline() );

	}

}
