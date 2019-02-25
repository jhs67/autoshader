//
//  File: struct-align.cpp
//
//  Created by Jon Spencer on 2019-01-27 15:36:23
//  Copyright (c) Jon Spencer. See LICENSE file.
//

#include "catch2/catch.hpp"
#include "glm/glm.hpp"
#include "vulkan/vulkan.hpp"
#include "autoshader/createpipe.h"

namespace shader {

	using namespace glm;

	#include "struct-align-autoshader.h"

}

TEST_CASE( "struct-align" ) {

	SECTION( "std140 basic rules" ) {

		shader::Uniform1 uni1;
		REQUIRE( offsetof(shader::Uniform1, test0) == 0 );
		REQUIRE( offsetof(shader::Uniform1, test1) == 16 );
		REQUIRE( offsetof(shader::Uniform1, test2) == 32 );
		REQUIRE( offsetof(shader::Uniform1, test3) == 44 );
		REQUIRE( offsetof(shader::Uniform1, test4) == 48 );
		REQUIRE( offsetof(shader::Uniform1, test5) == 56 );
		REQUIRE( offsetof(shader::Uniform1, test6) == 64 );
		REQUIRE( sizeof(shader::Uniform1::test6) == 64 );
		REQUIRE( offsetof(shader::Uniform1, test7) == 128 );
		REQUIRE( (&uni1.test8[1].v.x - &uni1.test8[0].v.x) == 4 );
		REQUIRE( (size_t(&uni1.test10[1][0]) - size_t(&uni1.test10[0][0])) == 48 );
		REQUIRE( (size_t(&uni1.test11[1]) - size_t(&uni1.test11[0])) == 16 );
		REQUIRE( sizeof(shader::Uniform1) == 448 );
	}

	SECTION( "std430 integers rules" ) {

		REQUIRE( offsetof(shader::Buffer1, a) == 0 );
		REQUIRE( offsetof(shader::Buffer1, c) == 4 );
		REQUIRE( sizeof(shader::Buffer1) == 8 );
	}

}
