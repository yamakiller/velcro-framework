#ifndef V_FRAMEWORK_CORE_STD_REFERENCE_WRAPPER_H
#define V_FRAMEWORK_CORE_STD_REFERENCE_WRAPPER_H

#include <vcore/std/typetraits/integral_constant.h>
#include <vcore/std/typetraits/decay.h>
#include <functional>

namespace VStd {
    using std::reference_wrapper;
    using std::ref;
    using std::cref;

      template<typename T>
    class is_reference_wrapper
        : public VStd::false_type {
    };

    template<class T>
    struct unwrap_reference {
        typedef typename VStd::decay<T>::type type;
    };

#define AUX_REFERENCE_WRAPPER_METAFUNCTIONS_DEF(X) \
    template<typename T>                           \
    struct is_reference_wrapper< X >               \
        : public VStd::true_type {                 \
    };                                             \
                                                   \
    template<typename T>                           \
    struct unwrap_reference< X > {                 \
        typedef T type;                            \
    };                                             \

    AUX_REFERENCE_WRAPPER_METAFUNCTIONS_DEF(reference_wrapper<T>)
    AUX_REFERENCE_WRAPPER_METAFUNCTIONS_DEF(reference_wrapper<T> const)
    AUX_REFERENCE_WRAPPER_METAFUNCTIONS_DEF(reference_wrapper<T> volatile)
    AUX_REFERENCE_WRAPPER_METAFUNCTIONS_DEF(reference_wrapper<T> const volatile)
    #undef AUX_REFERENCE_WRAPPER_METAFUNCTIONS_DEF
}

#endif // V_FRAMEWORK_CORE_STD_REFERENCE_WRAPPER_H