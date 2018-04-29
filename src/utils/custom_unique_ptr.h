#pragma once

#include <memory>

namespace utils {

/***
 * source:
 * https://stackoverflow.com/questions/45893205/wrapping-c-create-and-destroy-functions-using-a-smart-pointer/45893409
 *
 * This trick allows to declare unique_ptr with custom deletor function without
 * the boilerplate code (functor / struct with operator()).
 */
template<auto X> using constant_t=std::integral_constant<std::decay_t<decltype(X)>, X>;
template<auto X> constexpr constant_t<X> constant {};

template<class T, auto dtor> using custom_unique_ptr=std::unique_ptr<T, constant_t<dtor> >;

}
