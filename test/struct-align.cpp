//
//  File: struct-align.cpp
//
//  Created by Jon Spencer on 2019-01-27 15:36:23
//  Copyright (c) Jon Spencer. See LICENSE file.
//

#include "catch.hpp"
#include "glm/glm.hpp"

namespace shader {

	using namespace glm;

	#include "struct-align-autoshader.h"

}

TEST_CASE( "struct-align" ) {

	SECTION( "std140 basic rules" ) {

		REQUIRE( offsetof(shader::Uniform1, test0) == 0 );
		REQUIRE( offsetof(shader::Uniform1, test1) == 16 );
		REQUIRE( offsetof(shader::Uniform1, test2) == 32 );
		REQUIRE( offsetof(shader::Uniform1, test3) == 44 );
		REQUIRE( offsetof(shader::Uniform1, test4) == 48 );
		REQUIRE( offsetof(shader::Uniform1, test5) == 56 );
		REQUIRE( offsetof(shader::Uniform1, test6) == 64 );
		REQUIRE( sizeof(shader::Uniform1::test6) == 64 );
		REQUIRE( offsetof(shader::Uniform1, test7) == 128 );
		REQUIRE( sizeof(shader::Uniform1) == 144 );

	}

}
