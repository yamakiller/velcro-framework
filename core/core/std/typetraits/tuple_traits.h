/*
 * Copyright (c) Contributors to the VelcroFramework.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 * 
 */
#ifndef V_FRAMEWORK_CORE_STD_TYPETRAITS_TUPLE_TRAITS_H
#define V_FRAMEWORK_CORE_STD_TYPETRAITS_TUPLE_TRAITS_H

#include <core/std/typetraits/internal/type_sequence_traits.h>
#include <core/std/tuple.h>

namespace VStd {
    namespace Internal {
        template<typename>
        struct tuple_elements_sequence_helper {};

        template<typename ...t_Elements>
        struct tuple_elements_sequence_helper<std::tuple<t_Elements...>> {
            using TupleType = std::tuple<t_Elements...>;
            VSTD_STATIC_CONSTANT(size_t, size = std::tuple_size<TupleType>::value);

            template<size_t index>
            using get_element_t = Internal::pack_traits_get_arg_t<index, t_Elements...>;
            using elements_sequence = Internal::pack_traits_arg_sequence<t_Elements...>;
        };
    }    

    template<typename Tuple>
    struct tuple_elements_sequence : public Internal::tuple_elements_sequence_helper<Tuple> {};
}


#endif // V_FRAMEWORK_CORE_STD_TYPETRAITS_TUPLE_TRAITS_H