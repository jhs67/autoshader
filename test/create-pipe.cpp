//
//  File: create-pipe.cpp
//
//  Created by Jon Spencer on 2019-02-12 11:43:15
//  Copyright (c) Jon Spencer. See LICENSE file.
//

#include "catch.hpp"
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
		auto dev = phys[0].createDeviceUnique({});

		shader::Components pcomp(*dev);
		auto pipeline = pcomp.createPipe(*dev, vk::RenderPass());
		REQUIRE( *pipeline != vk::Pipeline() );

	}

}
