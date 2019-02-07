//
//  File: vertex-input.cpp
//
//  Created by Jon Spencer on 2019-02-06 14:38:27
//  Copyright (c) Jon Spencer. See LICENSE file.
//

#include "catch.hpp"
#include "glm/glm.hpp"
#include "vulkan/vulkan.hpp"

namespace shader {

	using namespace glm;

	#include "vertex-input-autoshader.h"

}

TEST_CASE( "vertex-input" ) {

	SECTION( "vertex structure layout" ) {

		REQUIRE( offsetof(shader::Vertex, attr1) == 0 );
		REQUIRE( offsetof(shader::Vertex, attr2) == 16 );
		REQUIRE( offsetof(shader::Vertex, attr3) == 28 );
		REQUIRE( offsetof(shader::Vertex, attr4) == 36 );
		REQUIRE( offsetof(shader::Vertex, attr5) == 60 );
		REQUIRE( offsetof(shader::Vertex, attr6) == 72 );
		REQUIRE( offsetof(shader::Vertex, attr7) == 88 );
		REQUIRE( offsetof(shader::Vertex, attr8) == 112 );
		REQUIRE( sizeof(shader::Vertex) == 128 );

		auto b = shader::getVertexBindingDescription();
		REQUIRE( b.size() == 1 );
		REQUIRE( b[0].binding == 0 );
		REQUIRE( b[0].stride == sizeof(shader::Vertex) );
		REQUIRE( b[0].inputRate == vk::VertexInputRate::eVertex );

		auto a = shader::getVertexAttributeDescriptions();
		REQUIRE( a.size() == 8 );

	}

}
