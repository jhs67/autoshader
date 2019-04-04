//
//  File: anyarg.h
//
//  Created by Jon Spencer on 2019-02-11 15:14:11
//  Copyright (c) Jon Spencer. See LICENSE file.
//
#ifndef H_AUTOSHADER_ANYARG_H__
#define H_AUTOSHADER_ANYARG_H__

#include <type_traits>
#include <utility>
#include <array>

namespace anyarg {

	namespace i {

		//------------------------------------------------------------------------------------------
		//--

		template <typename T>
		using remove_cvref_t = std::remove_cv_t<std::remove_reference_t<T>>;

		template <typename T, typename E>
		using type_match_t = std::is_same<T, remove_cvref_t<E>>;

		template <typename T>
		constexpr bool args_contains() { return false; }

		template <typename T, typename E, typename... A>
		constexpr bool args_contains() { return type_match_t<T, E>() ||
			args_contains<T, A...>(); }

		template <typename T> struct Arg;

		template <typename T, typename E>
		struct GatherCount :
			std::integral_constant<size_t, 0> {};

		template <typename T>
		struct GatherCount<T, T> :
			std::integral_constant<size_t, 1> {};

		template <typename T, size_t N>
		struct GatherCount<T, std::array<T, N>> :
			std::integral_constant<size_t, N> {};

		template <typename T>
		constexpr size_t gather_count() { return 0; }

		template <typename T, typename E, typename... A>
		constexpr size_t gather_count() {
			return GatherCount<T, remove_cvref_t<E>>{} + gather_count<T, A...>(); }

		template <typename T, typename E>
		struct is_array_of : std::integral_constant<bool, false> {};

		template <typename T, size_t N>
		struct is_array_of<T, std::array<T, N>> : std::integral_constant<bool, true> {};

	} // namespace i


	// Class to help extract values of type T from an argument pack.
	template <typename T>
	struct Arg {
		template <typename... A>
		struct contains : std::integral_constant<bool, i::args_contains<T, A...>()> {};

		template <typename... A>
		struct count : std::integral_constant<size_t, i::gather_count<T, A...>()> {};

		// Get a value that must be in the argument pack
		template <typename E, typename... A,
			std::enable_if_t<i::type_match_t<T, E>{}, int> = 0>
		static T get(E e, A &&...a) { return e; }

		template <typename E, typename... A,
			std::enable_if_t<!i::type_match_t<T, E>{}, int> = 0>
		static T get(E e, A &&...a) { return get(std::forward<A>(a)...); }

		template <typename... A>
		static T get() {
			static_assert(contains<A...>::value, "required argument missing from pack"); }

		// Get a value or return a default
		static T dget(T d) { return d; }

		template <typename E, typename... A,
			std::enable_if_t<i::type_match_t<T, E>{}, int> = 0>
		static T dget(T d, E e, A &&...a) { return e; }

		template <typename E, typename... A,
			std::enable_if_t<!i::type_match_t<T, E>{}, int> = 0>
		static T dget(T d, E e, A &&...a) { return dget(d, std::forward<A>(a)...); }

		// Get a pointer to a value or return nullptr
		static T* pget() { return nullptr; }

		template <typename E, typename... A,
			std::enable_if_t<i::type_match_t<T, E>{}, int> = 0>
		static T* pget(E && e, A &&...a) { return &e; }

		template <typename E, typename... A,
			std::enable_if_t<!i::type_match_t<T, E>{}, int> = 0>
		static T* pget(E && e, A &&...a) { return pget(std::forward<A>(a)...); }

		// Gather all the values into an array
		template <typename R>
		static void gather_(R &r, size_t i) {}

		template <typename R, typename E, typename... A,
			std::enable_if_t<i::type_match_t<T, E>{}, int> = 0>
		static void gather_(R &r, size_t i, E &&e, A &&...a) {
			r[i] = e; gather_(r, i + 1, std::forward<A>(a)...); }

		template <typename R, typename E, typename... A,
			std::enable_if_t<i::is_array_of<T, i::remove_cvref_t<E>>{}, int> = 0>
		static void gather_(R &r, size_t i, E &&e, A &&...a) {
			std::copy(e.begin(), e.end(), r.begin() + i);
			gather_(r, i + e.size(), std::forward<A>(a)...); }

		template <typename R, typename E, typename... A,
			std::enable_if_t<!i::type_match_t<T, E>{} &&
				!i::is_array_of<T, i::remove_cvref_t<E>>{}, int> = 0>
		static void gather_(R &r, size_t i, E &&e, A &&...a) {
			gather_(r, i, std::forward<A>(a)...); }

		template <typename... A>
		static auto gather(A ...a) { std::array<T, count<A...>::value> r;
			gather_(r, 0, std::forward<A>(a)...); return r; }
	};

} // namespace anyarg

#endif // H_AUTOSHADER_ANYARG_H__
