#include <vcore/io/compression_bus.h>

namespace V
{
    namespace IO
    {
        CompressionInfo::CompressionInfo(CompressionInfo&& rhs)
        {
            *this = VStd::move(rhs);
        }

        CompressionInfo& CompressionInfo::operator=(CompressionInfo&& rhs)
        {
            Decompressor = VStd::move(rhs.Decompressor);
            ArchiveFilename = VStd::move(rhs.ArchiveFilename);
            CompressionTag = rhs.CompressionTag;
            Offset = rhs.Offset;
            CompressedSize = rhs.CompressedSize;
            UncompressedSize = rhs.UncompressedSize;
            ConflictResolutionType = rhs.ConflictResolutionType;
            IsCompressed = rhs.IsCompressed;
            IsSharedPak = rhs.IsSharedPak;

            return *this;
        }

        namespace CompressionUtils
        {
            bool FindCompressionInfo(CompressionInfo& info, const VStd::string_view filename)
            {
                bool result = false;
                CompressionBus::Broadcast(&CompressionBus::Events::FindCompressionInfo, result, info, filename);
                return result;
            }
        }
    }
}
