#include <ostream>
#include <core/io/path/path.h>

namespace V::IO
{
    void PrintTo(const V::IO::PathView& path, ::std::ostream* os)
    {
        *os << "path: " << V::IO::Path(path.Native(), V::IO::PosixPathSeparator).MakePreferred().c_str();
    }

    void PrintTo(const V::IO::Path& path, ::std::ostream* os)
    {
        *os << "path: " << V::IO::Path(path.Native(), V::IO::PosixPathSeparator).MakePreferred().c_str();
    }

    void PrintTo(const V::IO::FixedMaxPath& path, ::std::ostream* os)
    {
        *os << "path: " << V::IO::FixedMaxPath(path.Native(), V::IO::PosixPathSeparator).MakePreferred().c_str();
    }
}
