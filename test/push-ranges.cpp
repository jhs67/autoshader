//
//  File: push-ranges.cpp
//
//  Created by Jon Spencer on 2019-02-19 10:34:00
//  Copyright (c) Jon Spencer. See LICENSE file.
//

#include "catch2/catch.hpp"
#include "glm/glm.hpp"
#include "vulkan/vulkan.hpp"
#include "autoshader/createpipe.h"

namespace shader {

	using namespace glm;

	#include "push-ranges-autoshader.h"

}

TEST_CASE( "push-ranges" ) {

	SECTION( "std140 basic rules" ) {

		auto pr = shader::getPushConstantRanges();
		REQUIRE( pr.size() == 4 );
		REQUIRE( pr[0].stageFlags == vk::ShaderStageFlagBits::eVertex );
		REQUIRE( pr[0].offset == 0 );
		REQUIRE( pr[0].size == 64 );
		REQUIRE( pr[1].stageFlags == vk::ShaderStageFlagBits::eFragment );
		REQUIRE( pr[1].offset == 64 );
		REQUIRE( pr[1].size == 16 );
		REQUIRE( pr[2].stageFlags == (vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment) );
		REQUIRE( pr[2].offset == 80 );
		REQUIRE( pr[2].size == 64 );
		REQUIRE( pr[3].stageFlags == vk::ShaderStageFlagBits::eFragment );
		REQUIRE( pr[3].offset == 144 );
		REQUIRE( pr[3].size == 48 );

	}

}
