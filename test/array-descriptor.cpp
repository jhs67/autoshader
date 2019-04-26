//
//  File: array-descriptor.cpp
//
//  Created by Jon Spencer on 2019-04-26 10:10:32
//  Copyright (c) Jon Spencer. See LICENSE file.
//

#include "catch2/catch.hpp"
#include "glm/glm.hpp"
#include "vulkan/vulkan.hpp"
#include "autoshader/createpipe.h"

namespace shader {

	using namespace glm;

	#include "array-descriptor-autoshader.h"

}

TEST_CASE( "array-descriptor" ) {

	SECTION( "creates the correct layout" ) {
		auto s0 = shader::getDescriptorSet0LayoutBindings(12);
		REQUIRE( s0.size() == 1 );
		REQUIRE( s0[0].binding == 0 );
		REQUIRE( s0[0].descriptorType == vk::DescriptorType::eStorageBuffer );
		REQUIRE( s0[0].descriptorCount == 12 );
		REQUIRE( s0[0].stageFlags == vk::ShaderStageFlagBits::eCompute );

		auto s1 = shader::getDescriptorSet1LayoutBindings();
		REQUIRE( s1.size() == 1 );
		REQUIRE( s1[0].binding == 0 );
		REQUIRE( s1[0].descriptorType == vk::DescriptorType::eCombinedImageSampler );
		REQUIRE( s1[0].descriptorCount == 64 );
		REQUIRE( s1[0].stageFlags == (vk::ShaderStageFlagBits::eCompute) );
	}

	SECTION( "ok to write many descriptors" ) {
		shader::descriptorSet1Writer(vk::DescriptorSet{})
		.settextures(vk::Sampler{}, vk::ImageView{})
		.settextures(vk::Sampler{}, vk::ImageView{})
		.settextures(vk::Sampler{}, vk::ImageView{})
		.settextures(vk::Sampler{}, vk::ImageView{});

		shader::descriptorSet0Writer(vk::DescriptorSet{})
		.setUniform1(vk::Buffer{})
		.setUniform1(vk::Buffer{})
		.setUniform1(vk::Buffer{})
		.setUniform1(vk::Buffer{});
	}

}
