#ifndef V_FRAMEWORK_CORE_IO_COMPRESSOR_STREAM_H
#define V_FRAMEWORK_CORE_IO_COMPRESSOR_STREAM_H

#include <vcore/io/generic_streams.h>
#include <vcore/std/smart_ptr/unique_ptr.h>
#include <vcore/memory/system_allocator.h>

namespace V
{
    namespace IO
    {
        class Compressor;
        class CompressorData;
        enum class OpenMode : V::u32;

        /*!
         * CompressorStream wrap a GenericStream and runs the streaming functions through the supplied compressor
         *
         */
        class CompressorStream
            : public GenericStream
        {
        public:
            V_CLASS_ALLOCATOR(CompressorStream, SystemAllocator, 0);
            CompressorStream(const char* filename, OpenMode flags = OpenMode());
            CompressorStream(GenericStream* stream, bool ownStream);
            virtual ~CompressorStream();

            bool IsOpen() const override;
            bool CanSeek() const override { return true; }
            bool CanRead() const override;
            bool CanWrite() const override;
            void Seek(OffsetType bytes, SeekMode mode) override;
            SizeType Read(SizeType bytes, void* oBuffer) override;
            SizeType Write(SizeType bytes, const void* iBuffer) override;
            SizeType GetCurPos() const override;

            /*!
            \brief Retrieves the length of the stream(which is the compressed length)
            \return length of the stream
            */
            SizeType GetLength() const override ///< Calls GetCompressedLength underneath the hood
            {
                return GetCompressedLength();
            }

            SizeType ReadAtOffset(SizeType bytes, void *oBuffer, OffsetType offset) override;
            SizeType WriteAtOffset(SizeType bytes, const void *iBuffer, OffsetType offset) override;
            bool IsCompressed() const override { return m_compressorData != nullptr; }
            const char* GetFilename() const override;
            OpenMode GetModeFlags() const override;
            bool ReOpen() override;
            void Close() override;

            GenericStream* GetWrappedStream() const;
            bool WriteCompressedHeader(V::u32 compressorId, int compressionLevel = 10, SizeType autoSeekDataSize = 0);
            bool WriteCompressedSeekPoint();
            SizeType GetCompressedLength() const; ///< Retrieves the length of the stream, which corresponds to the compressed length
            SizeType GetUncompressedLength() const; ///< Retrieves the length of the uncompressed data from the CompressorData structure
            
            void SetCompressorData(CompressorData* compressorData);
            CompressorData* GetCompressorData() const;

        protected:
            bool ReadCompressedHeader();
            Compressor* CreateCompressor(V::u32 compressorId);

        protected:
            GenericStream* m_stream; ///< Underlying stream to use for reading and writing raw data
            bool m_isStreamOwner; ///< Boolean which determines whether this class is responsible for ownership of the stream
            VStd::unique_ptr<CompressorData> m_compressorData; ///< CompressorData structure used for containing metadata related to the compressor in use
            VStd::unique_ptr<Compressor> m_compressor; ///< Compressor object responsible for performing compressions/decompression
        };

    } // namespace IO
}   // namespace AZ

#endif // V_FRAMEWORK_CORE_IO_COMPRESSOR_STREAM_H