#pragma once
#include <type_traits>
 namespace stdx
 {
     template<typename _T>
	 using value_type = typename std::remove_const<typename std::remove_reference<_T>::type>::type;
 }
