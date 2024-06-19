
#define V_STRING_EXPLICIT_SPECIALIZATION
#include <vcore/std/string/string.h>
#undef V_STRING_EXPLICIT_SPECIALIZATION

namespace VStd {
    // explicit specialization
    template class basic_string<char>;
    //template class basic_string<wchar_t>;
}