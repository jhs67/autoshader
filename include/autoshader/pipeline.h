//
//  File: pipeline.h
//
//  Created by Jon Spencer on 2019-02-16 12:14:29
//  Copyright (c) Jon Spencer. See LICENSE file.
//
#ifndef H_AUTOSHADER_PIPELINE_H__
#define H_AUTOSHADER_PIPELINE_H__

#include "createpipe.h"

namespace autoshader {

	template <typename Components>
	struct Pipeline {
		Pipeline() {}
		Pipeline(Pipeline &&o) noexcept { swap(o); }
		Pipeline &operator = (Pipeline &&o) noexcept { swap(o); return *this; }

		template <typename... A>
		Pipeline(vk::Device d, A &&...a) : components(d) { createPipe(std::forward<A>(a)...); }

		~Pipeline() { if (pipeline != vk::Pipeline()) components.device.destroyPipeline(pipeline); }

		template <class Tuple, std::size_t... I>
		void init_(vk::Device d, Tuple &t, std::index_sequence<I...>) {
			components = Components(d, std::get<I>(t)...);
		}

		template <class Tuple, std::size_t... I>
		void create_(Tuple &t, std::index_sequence<I...>) {
			createPipe(std::forward<std::tuple_element_t<I + Components::arity, Tuple>>(
				std::get<I + Components::arity>(t))...);
		}

	    template <typename... A>
		void init(vk::Device d, A &&...a)  {
			auto i = std::forward_as_tuple(std::forward<A>(a)...);
			init_(d, i, std::make_index_sequence<Components::arity>());
			create_(i, std::make_index_sequence<sizeof...(A) - Components::arity>());
		}

	    template <typename... A>
		void createPipe(A &&...a)  {
			if (pipeline != vk::Pipeline()) {
				components.device.destroyPipeline(pipeline);
				pipeline = vk::Pipeline{};
			}
			auto x = components.createPipe(std::forward<A>(a)...);
			pipeline = x.release();
		}

		operator vk::Pipeline () { return pipeline; }

		Components components;
		vk::Pipeline pipeline;
	};

} // namespace autoshader

#endif // H_AUTOSHADER_PIPELINE_H__
