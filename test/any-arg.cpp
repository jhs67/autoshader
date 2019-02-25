//
//  File: any-arg.cpp
//
//  Created by Jon Spencer on 2019-02-11 22:51:50
//  Copyright (c) Jon Spencer. See LICENSE file.
//

#include "catch2/catch.hpp"
#include "autoshader/anyarg.h"

namespace atest {

	template <typename... A>
	void foo(A &&...a) {
		using anyarg::Arg;
		REQUIRE( Arg<float>::count<A...>() == 2 );
		REQUIRE( Arg<int>::contains<A...>() );
		REQUIRE( !Arg<double>::contains<A...>() );
		REQUIRE( Arg<int>::get(std::forward<A>(a)...) == 42 );
		REQUIRE( Arg<float>::gather(std::forward<A>(a)...)[0] == 314 );
		REQUIRE( Arg<float>::gather(std::forward<A>(a)...)[1] == 159 );
	}

}

TEST_CASE( "any-arg" ) {

	SECTION( "handles basic tasks" ) {

		atest::foo(42, 314.0f, 159.0f);

	}

}
