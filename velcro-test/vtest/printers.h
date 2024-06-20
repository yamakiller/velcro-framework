#ifndef V_FRAMEWORK_VTEST_PRINTERS_H
#define V_FRAMEWORK_VTEST_PRINTERS_H

#include <iosfwd>
#include <vcore/io/path/path_fwd.h>

namespace VStd
{
    template<class Element, class Traits, class Allocator>
    class basic_string;

    template <class Element, class Traits>
    class basic_string_view;

    template <class Element, size_t MaxElementCount, class Traits>
    class basic_fixed_string;

    template<class Element, class Traits, class Allocator>
    void PrintTo(const VStd::basic_string<Element, Traits, Allocator>& value, ::std::ostream* os);
    template<class Element, class Traits>
    void PrintTo(const VStd::basic_string_view<Element, Traits>& value, ::std::ostream* os);
    template <class Element, size_t MaxElementCount, class Traits>
    void PrintTo(const VStd::basic_fixed_string<Element, MaxElementCount, Traits>& value, ::std::ostream* os);
}

namespace V::IO
{
    // Add Googletest printers for the V::IO::Path classes
    void PrintTo(const V::IO::PathView& path, ::std::ostream* os);
    void PrintTo(const V::IO::Path& path, ::std::ostream* os);
    void PrintTo(const V::IO::FixedMaxPath& path, ::std::ostream* os);
}

namespace V
{
    struct Uuid;
    void PrintTo(V::Uuid& uuid, ::std::ostream* os);
}

#include <vtest/printers.inl>

#endif // V_FRAMEWORK_VTEST_PRINTERS_H