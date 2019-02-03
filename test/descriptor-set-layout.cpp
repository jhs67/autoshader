//
//  File: descriptor-set-layout.cpp
//
//  Created by Jon Spencer on 2019-02-03 14:09:25
//  Copyright (c) Jon Spencer. See LICENSE file.
//

#include "catch.hpp"
#include "glm/glm.hpp"
#include "vulkan/vulkan.hpp"

namespace shader {

	using namespace glm;

	#include "descriptor-set-layout-autoshader.h"

}

TEST_CASE( "descriptor-set-layout" ) {

	SECTION( "creates the correct layout" ) {
		auto s0 = shader::getDescriptorSet0LayoutBindings();
		REQUIRE( s0.size() == 2 );
		REQUIRE( s0[0].binding == 0 );
		REQUIRE( s0[0].descriptorType == vk::DescriptorType::eUniformBuffer );
		REQUIRE( s0[0].descriptorCount == 1 );
		REQUIRE( s0[0].stageFlags == vk::ShaderStageFlagBits::eVertex );
		REQUIRE( s0[1].binding == 1 );
		REQUIRE( s0[1].descriptorType == vk::DescriptorType::eCombinedImageSampler );
		REQUIRE( s0[1].descriptorCount == 8 );
		REQUIRE( s0[1].stageFlags == vk::ShaderStageFlagBits::eFragment );

		auto s1 = shader::getDescriptorSet1LayoutBindings();
		REQUIRE( s1.size() == 2 );
		REQUIRE( s1[0].binding == 0 );
		REQUIRE( s1[0].descriptorType == vk::DescriptorType::eStorageImage );
		REQUIRE( s1[0].descriptorCount == 1 );
		REQUIRE( s1[0].stageFlags == (vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment) );
		REQUIRE( s1[1].binding == 1 );
		REQUIRE( s1[1].descriptorType == vk::DescriptorType::eStorageBuffer );
		REQUIRE( s1[1].descriptorCount == 1 );
		REQUIRE( s1[1].stageFlags == vk::ShaderStageFlagBits::eVertex );
	}

}
