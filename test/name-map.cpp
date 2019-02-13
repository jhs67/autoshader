//
//  File: name-map.cpp
//
//  Created by Jon Spencer on 2019-02-01 11:29:07
//  Copyright (c) Jon Spencer. See LICENSE file.
//

#include "catch.hpp"
#include "glm/glm.hpp"
#include "vulkan/vulkan.hpp"
#include "autoshader/createpipe.h"

namespace shader {

	using namespace glm;

	#include "name-map-autoshader.h"

}

TEST_CASE( "name-map" ) {

	SECTION( "structure name maps" ) {

		REQUIRE( sizeof(shader::Foo_vert) == 8 );
		REQUIRE( sizeof(shader::Faa_1) == 68 );
		REQUIRE( sizeof(shader::Uniform1) == 84 );
		REQUIRE( sizeof(shader::Faa_0) == 52 );
		REQUIRE( sizeof(shader::Buffer1) == 52 );
		REQUIRE( sizeof(shader::Foo_frag) == 32 );
		REQUIRE( sizeof(shader::Uniform2) == 32 );
	}

}
