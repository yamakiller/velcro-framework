#ifndef V_FRAMEWORK_CORE_STD_BIND_BIND_H
#define V_FRAMEWORK_CORE_STD_BIND_BIND_H

#include <functional>

namespace VStd {
    namespace placeholders {
        using std::placeholders::_1;
        using std::placeholders::_2;
        using std::placeholders::_3;
        using std::placeholders::_4;
        using std::placeholders::_5;
        using std::placeholders::_6;
        using std::placeholders::_7;
        using std::placeholders::_8;
        using std::placeholders::_9;
        using std::placeholders::_10;
    }

    using std::bind;

    using std::is_bind_expression;
    template<class T>
    constexpr bool is_bind_expression_v = std::is_bind_expression<T>::value;

    using std::is_placeholder;
    template<class T>
    constexpr size_t is_placeholder_v = is_placeholder<T>::value;
}

#endif // V_FRAMEWORK_CORE_STD_BIND_BIND_H