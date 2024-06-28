#ifndef V_FRAMEWORK_CORE_IO_COMPRESSOR_ZSTD_H
#define V_FRAMEWORK_CORE_IO_COMPRESSOR_ZSTD_H

#include <vcore/base.h>
#include <vcore/std/containers/vector.h>

#include <vcore/io/compressor.h>

#include <vcore/memory/system_allocator.h>
#include <vcore/compression/zstd_compression.h>

namespace V
{
    namespace IO
    {
        /**
         * Header stored after the standard compression header.
         * This structure is padded and aligned don't change members.
         */
        struct CompressorZStdHeader
        {
            V::u32     NumSeekPoints;    ///< Number of seek points located at the end of the stream.
        };

        /**
         * Seek points are stored at the end of the archive, we can have
         * 0..N seek points. If 0 the entire file is one seek point.
         */
        struct CompressorZStdSeekPoint
        {
            V::u64     CompressedOffset;         ///< Location in the compressed stream of the sync point.
            V::u64     UncompressedOffset;       ///< Location in the decompressed stream.
        };

        /**
         * ZStd compressor per stream data.
         */
        class CompressorZStdData
            : public CompressorData
        {
        public:
            V_CLASS_ALLOCATOR(CompressorZStdData, V::SystemAllocator, 0);

            CompressorZStdData(IAllocatorAllocate* zstdMemAllocator = 0)
            {
                ZStdHandle = zstdMemAllocator;
            }

            ZStd              ZStdHandle;
            V::u64            DecompressNextOffset{};      ///< Next offset in the compressed stream for decompressing.
            V::u64            DecompressLastOffset{};      ///< Last valid offset in the compressed stream of the compressed data. Used only when we decompress.
            unsigned char*    DecompressedCache{};         ///< Decompressed stream cache.
            unsigned int      DecompressedCacheDataSize{}; ///< Number of valid bytes in the decompressed cache.
            ZStd::Header      ZStdHeader;                  ///< Stored 2 bytes header of the zlib when we reset the decompressor.
            union
            {
                V::u64       DecompressedCacheOffset{};  ///< Used when decompressing. Decompressed cache is the data offset in the uncompressed data stream.
                V::u64       AutoSeekSize;               ///< Used when compressing to define auto seek point window.
            };

            using SeekPointArray = VStd::vector<CompressorZStdSeekPoint> ;
            SeekPointArray   SeekPoints;                ///< List of seek points for the archive, we must have at least one!
        };

        class CompressorZStd
            : public Compressor
        {
        public:
            V_CLASS_ALLOCATOR(CompressorZStd, V::SystemAllocator, 0);

            /**
             * \param decompressionCachePerStream cache of decompressed data stored per stream, the more streams you have open the more memory it will use,
             * You can use 0 size too to save memory, but every read will happen with decompression from the closest seek point. That might be fine if we
             * use stream read cache and it matches the size of the seek points (you will need to make sure of that), otherwise the extra cache can be useful.
             * We can convert this system to use pools of caches, but as of now this should be fine.
             * \param dataBufferSize we have one compressor per device, only one stream can read/write at a time and the data buffer is shared for IO operations,
             * the buffer is refCounted and existing only when we read/write compressed streams.
             */
            CompressorZStd(unsigned int decompressionCachePerStream = 64* 1024, unsigned int dataBufferSize = 128* 1024);

            ~CompressorZStd() override;

            /// Return compressor type id.
            static V::u32 TypeId();
            V::u32 GetTypeId() const override { return TypeId(); }
            /// Called when we open a stream to Read for the first time. Data contains the first. dataSize <= m_maxHeaderSize.
            bool ReadHeaderAndData(CompressorStream* stream, V::u8* data, unsigned int dataSize) override;
            /// Called when we are about to start writing to a compressed stream.
            bool WriteHeaderAndData(CompressorStream* stream) override;
            /// Forwarded function from the Device when we from a compressed stream.
            SizeType Read(CompressorStream* stream, SizeType byteSize, SizeType offset, void* buffer) override;
            /// Forwarded function from the Device when we write to a compressed stream.
            SizeType Write(CompressorStream* stream, SizeType byteSize, const void* data, SizeType offset = SizeType(-1)) override;
            /// Write a seek point.
            bool WriteSeekPoint(CompressorStream* stream) override;
            /// Set auto seek point even dataSize bytes.
            bool StartCompressor(CompressorStream* stream, int compressionLevel, SizeType autoSeekDataSize) override;
            /// Called just before we close the stream. All compression data will be flushed and finalized. (You can't add data afterwards).
            bool Close(CompressorStream* stream) override;

        protected:

            /// Read as much data as possible and adjust the parameters.
            SizeType            FillFromDecompressCache(CompressorZStdData* zlibData, void*& buffer, SizeType& byteSize, SizeType& offset);
            /// Read data from stream into the compression buffer.
            SizeType            FillCompressedBuffer(CompressorStream* stream);
            /// Acquire data buffer resource (if not created it will be allocated) and increment m_dataBufferUseCount.
            void                AcquireDataBuffer();
            /// Release data buffer resource, if m_dataBufferUseCount == 0 all memory will be freed.
            void                ReleaseDataBuffer();

            CompressorStream*   m_lastReadStream = nullptr;                   ///< Cached last stream we read data into the m_dataBuffer.
            SizeType            m_lastReadStreamOffset = 0;                   ///< Offset of the last read (in the m_dataBuffer) in the compressed stream.
            SizeType            m_lastReadStreamSize = 0;                     ///< Size of the data in m_dataBuffer of the last read.

            static const int    m_CompressedDataBufferAlignment = 16;         /// Alignment for data buffer.
            unsigned char*      m_compressedDataBuffer = nullptr;             ///< Data buffer used to read/write compressed data.
            unsigned int        m_compressedDataBufferSize;                   ///< Data buffer size (stored so we can lazy allocate m_dataBuffer as we need).
            unsigned int        m_compressedDataBufferUseCount = 0;           ///< Data buffer use count.
            unsigned int        m_decompressionCachePerStream;      ///< Cache per stream for each compressed stream stream in bytes.
        };
    }   // namespace IO
}   // namespace V

#endif // V_FRAMEWORK_CORE_IO_COMPRESSOR_ZSTD_H