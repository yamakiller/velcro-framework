#include <core/io/path/path.h>


// Explicit instantations of our support Path classes
namespace V::IO
{
    // Class template instantations
    template class BasicPath<VStd::string>;
    template class BasicPath<FixedMaxPathString>;
    template class PathIterator<PathView>;
    template class PathIterator<Path>;
    template class PathIterator<FixedMaxPath>;

    // Swap function instantiations
    template void swap<VStd::string>(Path& lhs, Path& rhs) noexcept;
    template void swap<FixedMaxPathString>(FixedMaxPath& lhs, FixedMaxPath& rhs) noexcept;

    // Hash function instantiations
    template size_t hash_value<VStd::string>(const Path& pathToHash);
    template size_t hash_value<FixedMaxPathString>(const FixedMaxPath& pathToHash);

    // Append operator instantiations
    template BasicPath<VStd::string> operator/<VStd::string>(const BasicPath<VStd::string>& lhs, const PathView& rhs);
    template BasicPath<FixedMaxPathString> operator/<FixedMaxPathString>(const BasicPath<FixedMaxPathString>& lhs, const PathView& rhs);
    template BasicPath<VStd::string> operator/<VStd::string>(const BasicPath<VStd::string>& lhs, VStd::string_view rhs);
    template BasicPath<FixedMaxPathString> operator/<FixedMaxPathString>(const BasicPath<FixedMaxPathString>& lhs, VStd::string_view rhs);
    template BasicPath<VStd::string> operator/<VStd::string>(const BasicPath<VStd::string>& lhs,
        const typename BasicPath<VStd::string>::value_type* rhs);
    template BasicPath<FixedMaxPathString> operator/<FixedMaxPathString>(const BasicPath<FixedMaxPathString>& lhs,
        const typename BasicPath<FixedMaxPathString>::value_type* rhs);

    // Iterator compare instantiations
    template bool operator==<PathView>(const PathIterator<PathView>& lhs,
        const PathIterator<PathView>& rhs);
    template bool operator==<Path>(const PathIterator<Path>& lhs,
        const PathIterator<Path>& rhs);
    template bool operator==<FixedMaxPath>(const PathIterator<FixedMaxPath>& lhs,
        const PathIterator<FixedMaxPath>& rhs);
    template bool operator!=<PathView>(const PathIterator<PathView>& lhs,
        const PathIterator<PathView>& rhs);
    template bool operator!=<Path>(const PathIterator<Path>& lhs,
        const PathIterator<Path>& rhs);
    template bool operator!=<FixedMaxPath>(const PathIterator<FixedMaxPath>& lhs,
        const PathIterator<FixedMaxPath>& rhs);
}