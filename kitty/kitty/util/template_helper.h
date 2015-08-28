#ifndef KITTY_UTIL_TEMPLATE_HELPER_H
#define KITTY_UTIL_TEMPLATE_HELPER_H

#include <type_traits>

namespace util {
  
// True if <Optional, Optional<T>>
template<template<typename...> class X, class T, class...Y>
struct instantiation_of : public std::false_type {};

template<template<typename...> class X, class T, class... Y>
struct instantiation_of<X, X<T, Y...>> : public std::true_type {};

// True if <Optional, X<Optional<T>>>
template<template<typename...> class X, class T, class...Y>
struct contains_instantiation_of : public std::false_type {};
  
template<template<typename...> class X, class T, class... Y>
struct contains_instantiation_of<X, X<T, Y...>> : public std::true_type {};
  
template<template<typename...> class X, template<typename...> class T, class... Y>
struct contains_instantiation_of<X, T<Y...>> : public contains_instantiation_of<X, Y...> {};

}
#endif