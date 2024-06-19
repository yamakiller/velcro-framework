
#include <ostream>
#include <string_view>
#include <core/std/string/string.h>
#include <core/std/string/fixed_string.h>

namespace VStd
{
    template<class Element, class Traits, class Allocator>
    void PrintTo(const VStd::basic_string<Element, Traits, Allocator>& value, ::std::ostream* os)
    {
        *os << value.c_str();
    }

    template<class Element, class Traits>
    void PrintTo(const VStd::basic_string_view<Element, Traits>& value, ::std::ostream* os)
    {
        *os << ::std::string_view(value.data(), value.size());
    }

    template <class Element, size_t MaxElementCount, class Traits>
    void PrintTo(const VStd::basic_fixed_string<Element, MaxElementCount, Traits>& value, ::std::ostream* os)
    {
        *os << value.c_str();
    }
}