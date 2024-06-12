#ifndef V_FRAMEWORK_CORE_STD_TYPETRAITS_ADD_POINTER_H
#define V_FRAMEWORK_CORE_STD_TYPETRAITS_ADD_POINTER_H


#include <core/std/typetraits/config.h>

namespace VStd {
    using std::add_pointer;
    template<class Type>
    using add_pointer_t = std::add_pointer_t<Type>;
}

#endif // V_FRAMEWORK_CORE_STD_TYPETRAITS_ADD_POINTER_H