#ifndef V_FRAMEWORK_CORE_STD_TYPETRAITS_ADD_REFERENCE_H
#define V_FRAMEWORK_CORE_STD_TYPETRAITS_ADD_REFERENCE_H

#include <core/std/typetraits/config.h>

namespace VStd {
    using std::add_lvalue_reference;
    template< class T >
    using add_lvalue_reference_t = typename add_lvalue_reference<T>::type;

    using std::add_rvalue_reference;
    template< class T >
    using add_rvalue_reference_t = typename add_rvalue_reference<T>::type;
}


#endif // V_FRAMEWORK_CORE_STD_TYPETRAITS_ADD_REFERENCE_H