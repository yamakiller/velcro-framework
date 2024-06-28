#if !defined(VCORE_EXCLUDE_ZSTD)

#include <vcore/io/compressor_zstd.h>
#include <vcore/io/compressor_stream.h>
#include <vcore/math/crc.h>
#include <vcore/math/math_utils.h>
#include <vcore/casting/numeric_cast.h>

namespace V
{
    namespace IO
    {
        CompressorZStd::CompressorZStd(unsigned int decompressionCachePerStream, unsigned int dataBufferSize)
            : m_compressedDataBufferSize(dataBufferSize)
            , m_decompressionCachePerStream(decompressionCachePerStream)

        {
            V_Assert((dataBufferSize % (32 * 1024)) == 0, "Data buffer size %d must be multiple of 32 KB.", dataBufferSize);
            V_Assert((decompressionCachePerStream % (32 * 1024)) == 0, "Decompress cache size %d must be multiple of 32 KB.", decompressionCachePerStream);
        }

        CompressorZStd::~CompressorZStd()
        {
            V_Warning("IO", m_compressedDataBufferUseCount == 0, "CompressorZStd has it's data buffer still referenced, it means that %d compressed streams have NOT closed. Freeing data...", m_compressedDataBufferUseCount);
            while (m_compressedDataBufferUseCount)
            {
                ReleaseDataBuffer();
            }
        }

        V::u32 CompressorZStd::TypeId()
        {
            return V_CRC("ZStd", 0x72fd505e);
        }

        bool CompressorZStd::ReadHeaderAndData(CompressorStream* stream, V::u8* data, unsigned int dataSize)
        {
            if (stream->GetCompressorData() != nullptr)  // we already have compressor data
            {
                return false;
            }

            // Read the ZStd header should be after the default compression header...
            // We should not be in this function otherwise.
            if (dataSize < sizeof(CompressorZStdHeader) + sizeof(ZStd::Header))
            {
                V_Error("CompressorZStd", false, "We did not read enough data, we have only %d bytes left in the buffer and we need %d.", dataSize, sizeof(CompressorZStdHeader) + sizeof(ZStd::Header));
                return false;
            }

            AcquireDataBuffer();

            CompressorZStdHeader* hdr = reinterpret_cast<CompressorZStdHeader*>(data);
            dataSize -= sizeof(CompressorZStdHeader);
            data += sizeof(CompressorZStdHeader);

            VStd::unique_ptr<CompressorZStdData> zstdData = VStd::make_unique<CompressorZStdData>();
            zstdData->CompressorHandle = this;
            zstdData->UncompressedSize = 0;
            zstdData->ZStdHeader = *reinterpret_cast<ZStd::Header*>(data);
            dataSize -= sizeof(zstdData->ZStdHeader);
            data += sizeof(zstdData->ZStdHeader);
            zstdData->DecompressNextOffset = sizeof(CompressorHeader) + sizeof(CompressorZStdHeader) + sizeof(ZStd::Header); // start after the headers

            V_Error("CompressorZStd", hdr->NumSeekPoints > 0, "We should have at least one seek point for the entire stream.");

            // go the end of the file and read all sync points.
            SizeType compressedFileEnd = stream->GetLength();
            if (compressedFileEnd == 0)
            {
                return false;
            }

            zstdData->SeekPoints.resize(hdr->NumSeekPoints);
            SizeType dataToRead = sizeof(CompressorZStdSeekPoint) * static_cast<SizeType>(hdr->NumSeekPoints);
            SizeType seekPointOffset = compressedFileEnd - dataToRead;

            if (seekPointOffset > compressedFileEnd)
            {
                V_Error("CompressorZStd", false, "We have an invalid archive, this is impossible.");
                return false;
            }

            GenericStream* baseStream = stream->GetWrappedStream();
            if (baseStream->ReadAtOffset(dataToRead, zstdData->SeekPoints.data(), seekPointOffset) != dataToRead)
            {
                return false;
            }

            if (m_decompressionCachePerStream)
            {
                zstdData->DecompressedCache = reinterpret_cast<unsigned char*>(vmalloc(m_decompressionCachePerStream, m_CompressedDataBufferAlignment, V::SystemAllocator, "CompressorZStd"));
            }

            zstdData->DecompressLastOffset = seekPointOffset; // set the start address of the seek points as the last valid read address for the compressed stream.

            zstdData->ZStdHandle.StartDecompressor();

            stream->SetCompressorData(zstdData.release());

            return true;
        }

        bool CompressorZStd::WriteHeaderAndData(CompressorStream* stream)
        {
            if (!Compressor::WriteHeaderAndData(stream))
            {
                return false;
            }

            CompressorZStdData* compressorData = static_cast<CompressorZStdData*>(stream->GetCompressorData());
            CompressorZStdHeader header;
            header.NumSeekPoints = v_numeric_caster(compressorData->SeekPoints.size());
            GenericStream* baseStream = stream->GetWrappedStream();
            if (baseStream->WriteAtOffset(sizeof(header), &header, sizeof(CompressorHeader)) == sizeof(header))
            {
                return true;
            }

            return false;
        }

        inline CompressorZStd::SizeType CompressorZStd::FillFromDecompressCache(CompressorZStdData* zstdData, void*& buffer, SizeType& byteSize, SizeType& offset)
        {
            SizeType firstOffsetInCache = zstdData->DecompressedCacheOffset;
            SizeType lastOffsetInCache = firstOffsetInCache + zstdData->DecompressedCacheDataSize;
            SizeType firstDataOffset = offset;
            SizeType lastDataOffset = offset + byteSize;
            SizeType numCopied = 0;
            if (firstOffsetInCache < lastDataOffset && lastOffsetInCache >= firstDataOffset)  // check if there is data in the cache
            {
                size_t copyOffsetStart = 0;
                size_t copyOffsetEnd = zstdData->DecompressedCacheDataSize;

                size_t bufferCopyOffset = 0;

                if (firstOffsetInCache < firstDataOffset)
                {
                    copyOffsetStart = v_numeric_caster(firstDataOffset - firstOffsetInCache);
                }
                else
                {
                    bufferCopyOffset = v_numeric_caster(firstOffsetInCache - firstDataOffset);
                }

                if (lastOffsetInCache >= lastDataOffset)
                {
                    copyOffsetEnd -= static_cast<size_t>(lastOffsetInCache - lastDataOffset);
                }
                else if (bufferCopyOffset > 0)  // the cache block is in the middle of the data, we can't use it (since we need to split buffer request into 2)
                {
                    return 0;
                }

                numCopied = copyOffsetEnd - copyOffsetStart;
                memcpy(static_cast<char*>(buffer) + bufferCopyOffset, zstdData->DecompressedCache + copyOffsetStart, static_cast<size_t>(numCopied));

                // adjust pointers and sizes
                byteSize -= numCopied;
                if (bufferCopyOffset == 0)
                {
                    // copied in the start
                    buffer = reinterpret_cast<char*>(buffer) + numCopied;
                    offset += numCopied;
                }
            }

            return numCopied;
        }

        inline CompressorZStd::SizeType CompressorZStd::FillCompressedBuffer(CompressorStream* stream)
        {
            CompressorZStdData* zstdData = static_cast<CompressorZStdData*>(stream->GetCompressorData());
            SizeType dataFromBuffer = 0;
            if (stream == m_lastReadStream)  // if the buffer is filled with data from the current stream, try to reuse
            {
                if (zstdData->DecompressNextOffset > m_lastReadStreamOffset)
                {
                    SizeType offsetInCache = zstdData->DecompressNextOffset - m_lastReadStreamOffset;
                    if (offsetInCache < m_lastReadStreamSize)  // last check if there is data overlap
                    {
                        // copy the usable part at the start of the buffer
                        SizeType toMove = m_lastReadStreamSize - offsetInCache;
                        memcpy(m_compressedDataBuffer, &m_compressedDataBuffer[static_cast<size_t>(offsetInCache)], static_cast<size_t>(toMove));
                        dataFromBuffer += toMove;
                    }
                }
            }

            SizeType toReadFromStream = m_compressedDataBufferSize - dataFromBuffer;
            SizeType readOffset = zstdData->DecompressNextOffset + dataFromBuffer;
            if (readOffset + toReadFromStream > zstdData->DecompressLastOffset)
            {
                // don't read past the end
                V_Assert(readOffset <= zstdData->DecompressLastOffset, "Read offset should always be before the end of stream.");
                toReadFromStream = zstdData->DecompressLastOffset - readOffset;
            }

            SizeType numReadFromStream = 0;
            if (toReadFromStream)  // if we did not reuse the whole buffer, read some data from the stream
            {
                GenericStream* baseStream = stream->GetWrappedStream();
                numReadFromStream = baseStream->ReadAtOffset(toReadFromStream, &m_compressedDataBuffer[static_cast<size_t>(dataFromBuffer)], readOffset);
            }

            // update what's actually in the read data buffer.
            m_lastReadStream = stream;
            m_lastReadStreamOffset = zstdData->DecompressNextOffset;
            m_lastReadStreamSize = dataFromBuffer + numReadFromStream;
            return m_lastReadStreamSize;
        }

        struct ZStdCompareUpper
        {
            bool operator()(const V::u64& offset, const CompressorZStdSeekPoint& sp) const 
            {
                return offset < sp.UncompressedOffset; 
            }
        };

        CompressorZStd::SizeType CompressorZStd::Read(CompressorStream* stream, SizeType byteSize, SizeType offset, void* buffer)
        {
            V_Assert(stream->GetCompressorData(), "This stream doesn't have decompression enabled.");
            CompressorZStdData* zstdData = static_cast<CompressorZStdData*>(stream->GetCompressorData());
            V_Assert(!zstdData->ZStdHandle.IsCompressorStarted(), "You can't read/decompress while writing a compressed stream %s.");

            // check if the request can be finished from the decompressed cache
            SizeType numRead = FillFromDecompressCache(zstdData, buffer, byteSize, offset);
            if (byteSize == 0)  // are we done
            {
                return numRead;
            }

            // find the best seek point for current offset
            CompressorZStdData::SeekPointArray::iterator it = VStd::upper_bound(zstdData->SeekPoints.begin(), zstdData->SeekPoints.end(), offset, ZStdCompareUpper());
            V_Assert(it != zstdData->SeekPoints.begin(), "This should be impossible, we should always have a valid seek point at 0 offset.");
            const CompressorZStdSeekPoint& bestSeekPoint = *(--it);  // get the previous (so it includes the current offset)

            // if read is continuous continue with decompression
            bool isJumpToSeekPoint = false;
            SizeType lastOffsetInCache = zstdData->DecompressedCacheOffset + zstdData->DecompressedCacheDataSize;
            if (bestSeekPoint.UncompressedOffset > lastOffsetInCache)     // if the best seek point is forward, jump forward to it.
            {
                isJumpToSeekPoint = true;
            }
            else if (offset < lastOffsetInCache)    // if the seek point is in the past and the requested offset is not in the cache jump back to it.
            {
                isJumpToSeekPoint = true;
            }

            if (isJumpToSeekPoint)
            {
                zstdData->DecompressNextOffset = bestSeekPoint.CompressedOffset;  // set next read point
                zstdData->DecompressedCacheOffset = bestSeekPoint.UncompressedOffset; // set uncompressed offset
                zstdData->DecompressedCacheDataSize = 0; // invalidate the cache
                zstdData->ZStdHandle.ResetDecompressor(&zstdData->ZStdHeader); // reset decompressor and setup the header.
            }

            // decompress and move forward until the request is done
            while (byteSize > 0)
            {
                // fill buffer with compressed data
                SizeType compressedDataSize = FillCompressedBuffer(stream);
                if (compressedDataSize == 0)
                {
                    return numRead; // we are done reading and obviously we did not managed to read all data
                }
                unsigned int processedCompressedData = 0;
                while (byteSize > 0 && processedCompressedData < compressedDataSize) // decompressed data either until we are done with the request (byteSize == 0) or we need to fill the compression buffer again.
                {
                    // if we have data in the cache move to the next offset, we always move forward by default.
                    zstdData->DecompressedCacheOffset += zstdData->DecompressedCacheDataSize;

                    // decompress in the cache buffer
                    u32 availDecompressedCacheSize = m_decompressionCachePerStream; // reset buffer size
                    size_t nextBlockSize;
                    unsigned int processed = zstdData->ZStdHandle.Decompress(&m_compressedDataBuffer[processedCompressedData], 
                                                                        static_cast<unsigned int>(compressedDataSize) - processedCompressedData,
                                                                        zstdData->DecompressedCache, 
                                                                        availDecompressedCacheSize, 
                                                                        &nextBlockSize);
                    zstdData->DecompressedCacheDataSize = m_decompressionCachePerStream - availDecompressedCacheSize;
                    if (processed == 0)
                    {
                        break; // we processed everything we could, load more compressed data.
                    }
                    processedCompressedData += processed;
                    // fill what we can from the cache
                    numRead += FillFromDecompressCache(zstdData, buffer, byteSize, offset);
                }
                // update next read position the the compressed stream
                zstdData->DecompressNextOffset +=  processedCompressedData;
            }
            return numRead;
        }

        CompressorZStd::SizeType CompressorZStd::Write(CompressorStream* stream, SizeType byteSize, const void* data, SizeType offset)
        {
            V_UNUSED(offset);

            V_Assert(stream && stream->GetCompressorData(), "This stream doesn't have compression enabled. Call Stream::WriteCompressed after you create the file.");
            V_Assert(offset == SizeType(-1) || offset == stream->GetCurPos(), "We can write compressed data only at the end of the stream.");

            m_lastReadStream = nullptr; // invalidate last read position, otherwise m_dataBuffer will be corrupted (as we are about to write in it).

            CompressorZStdData* zstdData = static_cast<CompressorZStdData*>(stream->GetCompressorData());
            V_Assert(!zstdData->ZStdHandle.IsDecompressorStarted(), "You can't write while reading/decompressing a compressed stream.");

            const u8* bytes = reinterpret_cast<const u8*>(data);
            unsigned int dataToCompress = v_numeric_caster(byteSize);
            while (dataToCompress != 0)
            {
                unsigned int oldDataToCompress = dataToCompress;
                unsigned int compressedSize = zstdData->ZStdHandle.Compress(bytes, dataToCompress, m_compressedDataBuffer, m_compressedDataBufferSize);
                if (compressedSize)
                {
                    GenericStream* baseStream = stream->GetWrappedStream();
                    SizeType numWritten = baseStream->Write(compressedSize, m_compressedDataBuffer);
                    if (numWritten != compressedSize)
                    {
                        return numWritten; // error we could not write all data
                    }
                }
                bytes += oldDataToCompress - dataToCompress;
            }
            zstdData->UncompressedSize += byteSize;

            if (zstdData->AutoSeekSize > 0)
            {
                // insert a seek point if needed.
                if (zstdData->SeekPoints.empty())
                {
                    if (zstdData->UncompressedSize >=  zstdData->AutoSeekSize)
                    {
                        WriteSeekPoint(stream);
                    }
                }
                else if ((zstdData->UncompressedSize - zstdData->SeekPoints.back().UncompressedOffset) > zstdData->AutoSeekSize)
                {
                    WriteSeekPoint(stream);
                }
            }
            return byteSize;
        }

        bool CompressorZStd::WriteSeekPoint(CompressorStream* stream)
        {
            V_Assert(stream && stream->GetCompressorData(), "This stream doesn't have compression enabled. Call Stream::WriteCompressed after you create the file.");
            CompressorZStdData* zstdData = static_cast<CompressorZStdData*>(stream->GetCompressorData());

            m_lastReadStream = nullptr; // invalidate last read position, otherwise m_dataBuffer will be corrupted (as we are about to write in it).

            unsigned int compressedSize;
            unsigned int dataToCompress = 0;
            do
            {
                compressedSize = zstdData->ZStdHandle.Compress(nullptr, dataToCompress, m_compressedDataBuffer, m_compressedDataBufferSize, ZStd::FT_FULL_FLUSH);
                if (compressedSize)
                {
                    GenericStream* baseStream = stream->GetWrappedStream();
                    SizeType numWritten = baseStream->Write(compressedSize, m_compressedDataBuffer);
                    if (numWritten != compressedSize)
                    {
                        return false; // error we wrote less than than requested!
                    }
                }
            } while (dataToCompress != 0);

            CompressorZStdSeekPoint sp;
            sp.CompressedOffset = stream->GetLength();
            sp.UncompressedOffset = zstdData->UncompressedSize;
            zstdData->SeekPoints.push_back(sp);
            return true;
        }

        bool CompressorZStd::StartCompressor(CompressorStream* stream, int compressionLevel, SizeType autoSeekDataSize)
        {
            V_Assert(stream && !stream->GetCompressorData(), "Stream has compressor already enabled.");

            AcquireDataBuffer();

            CompressorZStdData* zstdData = vnew CompressorZStdData;
            zstdData->CompressorHandle = this;
            zstdData->ZStdHeader       = 0; // not used for compression
            zstdData->UncompressedSize = 0;
            zstdData->AutoSeekSize = autoSeekDataSize;
            compressionLevel = V::GetClamp(compressionLevel, 1, 9); // remap to zlib levels

            zstdData->ZStdHandle.StartCompressor(compressionLevel);

            stream->SetCompressorData(zstdData);

            if (WriteHeaderAndData(stream))
            {
                // add the first and always present seek point at the start of the compressed stream
                CompressorZStdSeekPoint sp;
                sp.CompressedOffset = sizeof(CompressorHeader) + sizeof(CompressorZStdHeader) + sizeof(zstdData->ZStdHeader);
                sp.UncompressedOffset = 0;
                zstdData->SeekPoints.push_back(sp);
                return true;
            }
            return false;
        }

        bool CompressorZStd::Close(CompressorStream* stream)
        {
            V_Assert(stream->IsOpen(), "Stream is not open to be closed.");

            CompressorZStdData* zstdData = static_cast<CompressorZStdData*>(stream->GetCompressorData());
            GenericStream* baseStream = stream->GetWrappedStream();

            bool result = true;
            if (zstdData->ZStdHandle.IsCompressorStarted())
            {
                m_lastReadStream = nullptr; // invalidate last read position, otherwise m_dataBuffer will be corrupted (as we are about to write in it).

                // flush all compressed data
                unsigned int compressedSize;
                unsigned int dataToCompress = 0;
                do
                {
                    compressedSize = zstdData->ZStdHandle.Compress(nullptr, dataToCompress, m_compressedDataBuffer, m_compressedDataBufferSize, ZStd::FT_FINISH);
                    if (compressedSize)
                    {
                        baseStream->Write(compressedSize, m_compressedDataBuffer);
                    }
                } while (dataToCompress != 0);

                result = WriteHeaderAndData(stream);
                if (result)
                {
                    SizeType dataToWrite = zstdData->SeekPoints.size() * sizeof(CompressorZStdSeekPoint);
                    baseStream->Seek(0U, GenericStream::SeekMode::ST_SEEK_END);
                    result = (baseStream->Write(dataToWrite, zstdData->SeekPoints.data()) == dataToWrite);
                }
            }
            else
            {
                if (m_lastReadStream == stream)
                {
                    m_lastReadStream = nullptr; // invalidate the data in m_dataBuffer if it was from the current stream.
                }
            }

            // if we have decompressor cache delete it
            if (zstdData->DecompressedCache)
            {
                vfree(zstdData->DecompressedCache, V::SystemAllocator, m_decompressionCachePerStream, m_CompressedDataBufferAlignment);
            }

            ReleaseDataBuffer();

            // last step reset strream compressor data.
            stream->SetCompressorData(nullptr);
            return result;
        }

        void CompressorZStd::AcquireDataBuffer()
        {
            if (m_compressedDataBuffer == nullptr)
            {
                V_Assert(m_compressedDataBufferUseCount == 0, "Buffer usecount should be 0 if the buffer is NULL");
                m_compressedDataBuffer = reinterpret_cast<unsigned char*>(vmalloc(m_compressedDataBufferSize, m_CompressedDataBufferAlignment, V::SystemAllocator, "CompressorZStd"));
                m_lastReadStream = nullptr; // reset the cache info in the m_dataBuffer
            }
            ++m_compressedDataBufferUseCount;
        }

        void CompressorZStd::ReleaseDataBuffer()
        {
            --m_compressedDataBufferUseCount;
            if (m_compressedDataBufferUseCount == 0)
            {
                V_Assert(m_compressedDataBuffer, "Invalid data buffer. We should have a non null pointer.");
                vfree(m_compressedDataBuffer, V::SystemAllocator, m_compressedDataBufferSize, m_CompressedDataBufferAlignment);
                m_compressedDataBuffer = nullptr;
                m_lastReadStream = nullptr; // reset the cache info in the m_dataBuffer
            }
        }
    }   // namespace IO
}   // namespace V

#endif // #if !defined(VCORE_EXCLUDE_ZSTD)