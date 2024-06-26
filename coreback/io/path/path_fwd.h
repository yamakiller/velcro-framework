#ifndef V_FRAMEWORK_CORE_IO_PATH_PATH_FWD_H
#define V_FRAMEWORK_CORE_IO_PATH_PATH_FWD_H

#include <cstddef>

namespace VStd {
    class allocator;

    template <class Element>
    struct char_traits;

    template <class Element, class Traits, class Allocator>
    class basic_string;
    
    template <class Element, size_t MaxElementCount, class Traits>
    class basic_fixed_string;

    template <typename T>
    struct hash;
}

namespace V::IO {
    //! Path Constants
    inline constexpr size_t MaxPathLength = 1024;
    inline constexpr char PosixPathSeparator = '/';
    inline constexpr char WindowsPathSeparator = '\\';

    //! Path aliases
    using FixedMaxPathString = VStd::basic_fixed_string<char, MaxPathLength, VStd::char_traits<char>>;

    //! Forward declarations for the PathView, Path and FixedMaxPath classes
    class PathView;

    template <typename StringType>
    class BasicPath;

    //! Only the following path types are supported
    //! The BasicPath template above is shared only amoung the following instantiations
    using Path = BasicPath<VStd::basic_string<char, VStd::char_traits<char>, VStd::allocator>>;
    using FixedMaxPath = BasicPath<FixedMaxPathString>;

    // Forward declare the PathIterator type.
    // It depends on the path type
    template <typename PathType>
    class PathIterator;
}

namespace VStd
{
    template <>
    struct hash<V::IO::PathView>;

    template <typename StringType>
    struct hash<V::IO::BasicPath<StringType>>;
}



#endif // V_FRAMEWORK_CORE_IO_PATH_PATH_FWD_H