/*
 * Copyright (c) Contributors to the VelcroFramework.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#ifndef V_FRAMEWORK_CORE_STD_TUPLE_H
#define V_FRAMEWORK_CORE_STD_TUPLE_H

#include <vcore/rtti/type.h>
#include <vcore/std/containers/array.h>
#include <vcore/std/function/invoke.h>
#include <vcore/std/utils.h>
#include <vcore/std/typetraits/is_same.h>
#include <vcore/std/typetraits/void_t.h>
#include <tuple>
#include <vcore/std/typetraits/conjunction.h>

namespace VStd {
    
    template<class... Types>
    using tuple = std::tuple<Types...>;

    template<class T>
    using tuple_size = std::tuple_size<T>;

    template<size_t I, class T>
    using tuple_element = std::tuple_element<I, T>;

    template<size_t I, class T>
    using tuple_element_t = typename std::tuple_element<I, T>::type;

    // Placeholder structure that can be assigned any value with no effect.
    // This is used by VStd::tie as placeholder for unused arugments
    using ignore_t = VStd::decay_t<decltype(std::ignore)>;
    decltype(std::ignore) ignore = std::ignore;

    using std::make_tuple;
    using std::tie;
    using std::forward_as_tuple;
    using std::tuple_cat;
    using std::get;


    //! Creates an hash specialization for tuple types using the hash_combine function
    //! The std::tuple implementation does not have this support. This is an extension
    template <typename... Types>
    struct hash<VStd::tuple<Types...>> {
        template<size_t... Indices>
        constexpr size_t ElementHasher(const VStd::tuple<Types...>& value, VStd::index_sequence<Indices...>) const {
            size_t seed = 0;
            int dummy[]{ 0, (hash_combine(seed, VStd::get<Indices>(value)), 0)... };
            (void)dummy;
            return seed;
        }

        template<typename... TupleTypes, typename = VStd::enable_if_t<hash_enabled_concept_v<TupleTypes...>>>
        constexpr size_t operator()(const VStd::tuple<TupleTypes...>& value) const {
            return ElementHasher(value, VStd::make_index_sequence<sizeof...(Types)>{});
        }
    };
}

namespace VStd {
    // pair code to inter operate with tuples
    template<class T1, class T2>
    template<template<class...> class TupleType, class... Args1, class... Args2, size_t... I1, size_t... I2>
    constexpr pair<T1, T2>::pair(piecewise_construct_t, TupleType<Args1...>& first_args, TupleType<Args2...>& second_args,
        VStd::index_sequence<I1...>, VStd::index_sequence<I2...>)
        : first(VStd::forward<Args1>(VStd::get<I1>(first_args))...)
        , second(VStd::forward<Args2>(VStd::get<I2>(second_args))...)
    {
        (void)first_args;
        (void)second_args;
        static_assert(VStd::is_same_v<TupleType<Args2...>, tuple<Args2...>>, "VStd::pair tuple constructor can be called with VStd::tuple instances");
    }

    // Pair constructor overloads which take in a tuple is implemented here as tuple is not included at the place where pair declares the constructor
    template<class T1, class T2>
    template<template<class...> class TupleType, class... Args1, class... Args2>
    constexpr pair<T1, T2>::pair(piecewise_construct_t piecewise_construct,
        TupleType<Args1...> first_args,
        TupleType<Args2...> second_args)
        : pair(piecewise_construct, first_args, second_args, VStd::make_index_sequence<sizeof...(Args1)>{}, VStd::make_index_sequence<sizeof...(Args2)>{})
    {
        static_assert(VStd::is_same_v<TupleType<Args1...>, tuple<Args1...>>, "VStd::pair tuple constructor can be called with VStd::tuple instances");
    }

    namespace Internal
    {
        template<size_t> struct get_pair;

        template<>
        struct get_pair<0>
        {
            template<class T1, class T2>
            static constexpr T1& get(VStd::pair<T1, T2>& pairObj) { return pairObj.first; }

            template<class T1, class T2>
            static constexpr const T1& get(const VStd::pair<T1, T2>& pairObj) { return pairObj.first; }

            template<class T1, class T2>
            static constexpr T1&& get(VStd::pair<T1, T2>&& pairObj) { return VStd::forward<T1>(pairObj.first); }

            template<class T1, class T2>
            static constexpr const T1&& get(const VStd::pair<T1, T2>&& pairObj) { return VStd::forward<const T1>(pairObj.first); }
        };
        template<>
        struct get_pair<1>
        {
            template<class T1, class T2>
            static constexpr T2& get(VStd::pair<T1, T2>& pairObj) { return pairObj.second; }

            template<class T1, class T2>
            static constexpr const T2& get(const VStd::pair<T1, T2>& pairObj) { return pairObj.second; }

            template<class T1, class T2>
            static constexpr T2&& get(VStd::pair<T1, T2>&& pairObj) { return VStd::forward<T2>(pairObj.second); }

            template<class T1, class T2>
            static constexpr const T2&& get(const VStd::pair<T1, T2>&& pairObj) { return VStd::forward<const T2>(pairObj.second); }
        };
    }

    //! Wraps the std::get function in the VStd namespace
    //! This methods retrieves the tuple element at a particular index within the pair
    template<size_t I, class T1, class T2>
    constexpr VStd::tuple_element_t<I, VStd::pair<T1, T2>>& get(VStd::pair<T1, T2>& pairObj)
    {
        return Internal::get_pair<I>::get(pairObj);
    }

    //! Wraps the std::get function in the VStd namespace
    //! This methods retrieves the tuple element at a particular index within the pair
    template<size_t I, class T1, class T2>
    constexpr const VStd::tuple_element_t<I, VStd::pair<T1, T2>>& get(const VStd::pair<T1, T2>& pairObj)
    {
        return Internal::get_pair<I>::get(pairObj);
    }

    //! Wraps the std::get function in the VStd namespace
    //! This methods retrieves the tuple element at a particular index within the pair
    template<size_t I, class T1, class T2>
    constexpr VStd::tuple_element_t<I, VStd::pair<T1, T2>>&& get(VStd::pair<T1, T2>&& pairObj)
    {
        return Internal::get_pair<I>::get(VStd::move(pairObj));
    }

    //! Wraps the std::get function in the VStd namespace
    //! This methods retrieves the tuple element at a particular index within the pair
    template<size_t I, class T1, class T2>
    constexpr const VStd::tuple_element_t<I, VStd::pair<T1, T2>>&& get(const VStd::pair<T1, T2>&& pairObj)
    {
        return Internal::get_pair<I>::get(VStd::move(pairObj));
    }

    //! Wraps the std::get function in the VStd namespace
    //! This methods extracts an element from the pair with the specified type T
    //! If there is more than one T in the pair, then this function fails to compile
    template<class T, class U>
    constexpr T& get(VStd::pair<T, U>& pairObj)
    {
        return Internal::get_pair<0>::get(pairObj);
    }

    //! Wraps the std::get function in the VStd namespace
    //! This methods extracts an element from the pair with the specified type T
    //! If there is more than one T in the pair, then this function fails to compile
    template<class T, class U>
    constexpr T& get(VStd::pair<U, T>& pairObj)
    {
        return Internal::get_pair<1>::get(pairObj);
    }

    //! Wraps the std::get function in the VStd namespace
    //! This methods extracts an element from the pair with the specified type T
    //! If there is more than one T in the pair, then this function fails to compile
    template<class T, class U>
    constexpr const T& get(const VStd::pair<T, U>& pairObj)
    {
        return Internal::get_pair<0>::get(pairObj);
    }

    //! Wraps the std::get function in the VStd namespace
    //! This methods extracts an element from the pair with the specified type T
    //! If there is more than one T in the pair, then this function fails to compile
    template<class T, class U>
    constexpr const T& get(const VStd::pair<U, T>& pairObj)
    {
        return Internal::get_pair<1>::get(pairObj);
    }

    //! Wraps the std::get function in the VStd namespace
    //! This methods extracts an element from the pair with the specified type T
    //! If there is more than one T in the pair, then this function fails to compile
    template<class T, class U>
    constexpr T&& get(VStd::pair<T, U>&& pairObj)
    {
        return Internal::get_pair<0>::get(VStd::move(pairObj));
    }

    //! Wraps the std::get function in the VStd namespace
    //! This methods extracts an element from the pair with the specified type T
    //! If there is more than one T in the pair, then this function fails to compile
    template<class T, class U>
    constexpr T&& get(VStd::pair<U, T>&& pairObj)
    {
        return Internal::get_pair<1>::get(VStd::move(pairObj));
    }

    //! Wraps the std::get function in the VStd namespace
    //! This methods extracts an element from the pair with the specified type T
    //! If there is more than one T in the pair, then this function fails to compile
    template<class T, class U>
    constexpr const T&& get(const VStd::pair<T, U>&& pairObj)
    {
        return Internal::get_pair<0>::get(VStd::move(pairObj));
    }

    //! Wraps the std::get function in the VStd namespace
    //! This methods extracts an element from the pair with the specified type T
    //! If there is more than one T in the pair, then this function fails to compile
    template<class T, class U>
    constexpr const T&& get(const VStd::pair<U, T>&& pairObj)
    {
        return Internal::get_pair<1>::get(VStd::move(pairObj));
    }

    //! VStd::pair to std::tuple function for replicating the functionality of std::tuple assignment operator from std::pair
    template<class T1, class T2>
    constexpr tuple<T1, T2> tuple_assign(const VStd::pair<T1, T2>& azPair)
    {
        return std::make_tuple(azPair.first, azPair.second);
    }

    //! VStd::pair to std::tuple function for replicating the functionality of std::tuple assignment operator from std::pair
    template<class T1, class T2>
    constexpr tuple<T1, T2> tuple_assign(VStd::pair<T1, T2>&& azPair)
    {
        return std::make_tuple(VStd::move(azPair.first), VStd::move(azPair.second));
    }
}

namespace VStd
{
    // implementation of the std::get function within the VStd::namespace which allows VStd::apply to be used
    // with VStd::array
    template<size_t I, class T, size_t N>
    constexpr T& get(VStd::array<T, N>& arr)
    {
        static_assert(I < N, "VStd::get has been called on array with an index that is out of bounds");
        return arr[I];
    };

    template<size_t I, class T, size_t N>
    constexpr const T& get(const VStd::array<T, N>& arr)
    {
        static_assert(I < N, "VStd::get has been called on array with an index that is out of bounds");
        return arr[I];
    };

    template<size_t I, class T, size_t N>
    constexpr T&& get(VStd::array<T, N>&& arr)
    {
        static_assert(I < N, "VStd::get has been called on array with an index that is out of bounds");
        return VStd::move(arr[I]);
    };

    template<size_t I, class T, size_t N>
    constexpr const T&& get(const VStd::array<T, N>&& arr)
    {
        static_assert(I < N, "VStd::get has been called on array with an index that is out of bounds");
        return VStd::move(arr[I]);
    };
}

// VStd::apply implemenation helper block 
namespace VStd
{
    namespace Internal
    {
        template<class Fn, class Tuple, size_t... Is>
        constexpr auto apply_impl(Fn&& f, Tuple&& tupleObj, VStd::index_sequence<Is...>) -> decltype(VStd::invoke(VStd::declval<Fn>(), VStd::get<Is>(VStd::declval<Tuple>())...))
        {
            (void)tupleObj;
            return VStd::invoke(VStd::forward<Fn>(f), VStd::get<Is>(VStd::forward<Tuple>(tupleObj))...);
        }
    }

    template<class Fn, class Tuple>
    constexpr auto apply(Fn&& f, Tuple&& tupleObj)
        -> decltype(Internal::apply_impl(VStd::declval<Fn>(), VStd::declval<Tuple>(), VStd::make_index_sequence<VStd::tuple_size<VStd::decay_t<Tuple>>::value>{}))
    {
        return Internal::apply_impl(VStd::forward<Fn>(f), VStd::forward<Tuple>(tupleObj), VStd::make_index_sequence<VStd::tuple_size<VStd::decay_t<Tuple>>::value>{});
    }
}

// The tuple_size and tuple_element classes need to be specialized in the std:: namespace since the VStd:: namespace alias them
// The tuple_size and tuple_element classes is to be specialized here for the VStd::pair class
// The tuple_size and tuple_element classes is to be specialized here for the VStd::array class

//std::tuple_size<std::pair> as defined by C++ 11 until C++ 14
//template< class T1, class T2 >
//struct tuple_size<std::pair<T1, T2>>;
//std::tuple_size<std::pair> as defined since C++ 14
//template <class T1, class T2>
//struct tuple_size<std::pair<T1, T2>> : std::integral_constant<std::size_t, 2> { };

//std::tuple_element<std::pair> as defined since C++ 11
//template< class T1, class T2 >
//struct tuple_element<0, std::pair<T1,T2> >;
//template< class T1, class T2 >
//struct tuple_element<1, std::pair<T1,T2> >;

namespace std
{
    // Suppressing clang warning error: 'tuple_size' defined as a class template here but previously declared as a struct template [-Werror,-Wmismatched-tags]
    V_PUSH_DISABLE_WARNING(, "-Wmismatched-tags")
    template<class T1, class T2>
    struct tuple_size<VStd::pair<T1, T2>> : public VStd::integral_constant<size_t, 2> {};

    template<class T1, class T2>
    struct tuple_element<0, VStd::pair<T1, T2>>
    {
    public:
        using type = T1;
    };

    template<class T1, class T2>
    struct tuple_element<1, VStd::pair<T1, T2>>
    {
    public:
        using type = T2;
    };

    template<class T, size_t N>
    struct tuple_size<VStd::array<T, N>> : public VStd::integral_constant<size_t, N> {};

    template<size_t I, class T, size_t N>
    struct tuple_element<I, VStd::array<T, N>>
    {
        static_assert(I < N, "VStd::tuple_element has been called on array with an index that is out of bounds");
        using type = T;
    };
    V_POP_DISABLE_WARNING
} // namespace VStd

#endif