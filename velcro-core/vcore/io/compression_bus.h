#ifndef V_FRAMEWORK_CORE_IO_COMPRESSION_BUS_H
#define V_FRAMEWORK_CORE_IO_COMPRESSION_BUS_H

#include <vcore/event_bus/event_bus.h>
#include <vcore/io/streamer/request_path.h>
#include <vcore/std/parallel/mutex.h>
#include <vcore/std/functional.h>
#include <vcore/std/string/string.h>
#include <vcore/std/string/string_view.h>

namespace V
{
    namespace IO
    {
        union CompressionTag
        {
            uint32_t Code;
            char Name[4];
        };

        enum class ConflictResolution : uint8_t
        {
            PreferFile,
            PreferArchive,
            UseArchiveOnly
        };

        struct CompressionInfo;
        using DecompressionFunc = VStd::function<bool(const CompressionInfo& info, const void* compressed, size_t compressedSize, void* uncompressed, size_t uncompressedBufferSize)>;
    
        struct CompressionInfo
        {
            CompressionInfo() = default;
            CompressionInfo(const CompressionInfo& rhs) = default;
            CompressionInfo& operator=(const CompressionInfo& rhs) = default;

            CompressionInfo(CompressionInfo&& rhs);
            CompressionInfo& operator=(CompressionInfo&& rhs);

            //! Relative path to the archive file.
            RequestPath ArchiveFilename;
            //< The function to use to decompress the data.
            DecompressionFunc Decompressor;
            //< Tag that uniquely identifies the compressor responsible for decompressing the referenced data.
            CompressionTag CompressionTag{ 0 };
            //! Offset into the archive file for the found file.
            size_t Offset = 0;
            //! On disk size of the compressed file.
            size_t CompressedSize = 0;
            //! Size after the file has been decompressed.
            size_t UncompressedSize = 0;
            //! Preferred solution when an archive is found in the archive and as a separate file.
            ConflictResolution ConflictResolutionType = ConflictResolution::UseArchiveOnly;
            //! Whether or not the file is compressed. If the file is not compressed, the compressed and uncompressed sizes should match.
            bool IsCompressed = false;
            //! Whether or not the pak file is used in multiple location or reads can be done exclusively.
            bool IsSharedPak = false; 
        };

        class Compression : public V::EventBusTraits
        {
        public:
            static const V::EventBusHandlerPolicy HandlerPolicy = V::EventBusHandlerPolicy::Multiple;
            using MutexType = VStd::recursive_mutex;
            
            virtual ~Compression() = default;

            virtual void FindCompressionInfo(bool& found, CompressionInfo& info, const VStd::string_view filename) = 0;
        };

        using CompressionBus = V::EventBus<Compression>;

        namespace CompressionUtils
        {
            bool FindCompressionInfo(CompressionInfo& info, const VStd::string_view filename);
        }
    }
}


#endif // V_FRAMEWORK_CORE_IO_COMPRESSION_BUS_H