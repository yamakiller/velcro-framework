#ifndef V_FRAMEWORK_CORE_IO_COMPRESSOR_H
#define V_FRAMEWORK_CORE_IO_COMPRESSOR_H

#include <vcore/base.h>

namespace V
{
    namespace IO
    {
        class CompressorStream;

        /**
         * Compressor/Decompressor base interface.
         * Used for all stream compressors.
         */
        class Compressor
        {
        public:
            typedef V::u64 SizeType;
            static const int m_maxHeaderSize = 4096; /// When we open a stream to check if it's compressed we read the first m_maxHeaderSize bytes.

            virtual ~Compressor() {}
            /// Return compressor type id.
            virtual V::u32     GetTypeId() const = 0;
            /// Called when we open a stream to Read for the first time. Data contains the first. dataSize <= m_maxHeaderSize.
            virtual bool        ReadHeaderAndData(CompressorStream* stream, V::u8* data, unsigned int dataSize) = 0;
            /// Called when we are about to start writing to a compressed stream. (Must be called first to write compressor header)
            virtual bool        WriteHeaderAndData(CompressorStream* stream);
            /// Forwarded function from the Device when we from a compressed stream.
            virtual SizeType    Read(CompressorStream* stream, SizeType byteSize, SizeType offset, void* buffer) = 0;
            /// Forwarded function from the Device when we write to a compressed stream.
            virtual SizeType    Write(CompressorStream* stream, SizeType byteSize, const void* data, SizeType offset = SizeType(-1)) = 0;
            /// Write a seek point.
            virtual bool        WriteSeekPoint(CompressorStream* stream)                                                          { (void)stream; return false; }
            /// Initializes Compressor for writing data.
            virtual bool        StartCompressor(CompressorStream* stream, int compressionLevel, SizeType autoSeekDataSize)        { (void)stream; (void)compressionLevel; (void)autoSeekDataSize; return false; }
            /// Called just before we close the stream. All compression data will be flushed and finalized. (You can't add data afterwards).
            virtual bool        Close(CompressorStream* stream) = 0;
        };

        /**
         * Base compressor data assigned for all compressors.
         */
        class CompressorData
        {
        public:
            virtual ~CompressorData() {}

            Compressor*    CompressorHandle;
            V::u64         UncompressedSize;
        };

        /**
         * All data is stored in network order (big endian).
         */
        struct  CompressorHeader
        {
            CompressorHeader()                              { Vcs[0] = 0; Vcs[1] = 0; Vcs[2] = 0; }

            inline bool IsValid() const                     { return (Vcs[0] == 'V' && Vcs[1] == 'C' && Vcs[2] == 'S'); }
            void SetVCS()                                  { Vcs[0] = 'V'; Vcs[1] = 'C'; Vcs[2] = 'S';}

            char       Vcs[3];          ///< String contains 'AZCS' AmaZon Compressed Stream
            V::u32     CompressorId;     ///< Compression method.
            V::u64     UncompressedSize; ///< Uncompressed file size.
        };
    } // namespace IO
} // namespace AZ


#endif // V_FRAMEWORK_CORE_IO_COMPRESSOR_H