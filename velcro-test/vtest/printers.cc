#include <vcore/io/path/path.h>
#include <vcore/math/uuid.h>

#include <ostream>

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
} // namespace V::IO

namespace V
{
    void PrintTo(const V::Uuid& uuid, ::std::ostream* os)
    {
        *os << uuid.ToFixedString().c_str();
    }
} // namespace V