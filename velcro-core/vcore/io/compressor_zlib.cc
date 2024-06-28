#if !defined(VCORE_EXCLUDE_ZLIB)

#include <vcore/io/compressor_zlib.h>
#include <vcore/io/compressor_stream.h>
#include <vcore/math/crc.h>
#include <vcore/math/math_utils.h>

namespace V
{
    namespace IO
    {
        //=========================================================================
        // CompressorZLib
        // [12/13/2012]
        //=========================================================================
        CompressorZLib::CompressorZLib(unsigned int decompressionCachePerStream, unsigned int dataBufferSize)
            : m_lastReadStream(nullptr)
            , m_lastReadStreamOffset(0)
            , m_lastReadStreamSize(0)
            , m_compressedDataBuffer(nullptr)
            , m_compressedDataBufferSize(dataBufferSize)
            , m_compressedDataBufferUseCount(0)
            , m_decompressionCachePerStream(decompressionCachePerStream)

        {
            V_Assert((dataBufferSize % (32 * 1024)) == 0, "Data buffer size %d must be multiple of 32 KB!", dataBufferSize);
            V_Assert((decompressionCachePerStream % (32 * 1024)) == 0, "Decompress cache size %d must be multiple of 32 KB!", decompressionCachePerStream);
        }

        //=========================================================================
        // !CompressorZLib
        //=========================================================================
        CompressorZLib::~CompressorZLib()
        {
            V_Warning("IO", m_compressedDataBufferUseCount == 0, "CompressorZLib has it's data buffer still referenced, it means that %d compressed streams have NOT closed! Freeing data...", m_compressedDataBufferUseCount);
            while (m_compressedDataBufferUseCount)
            {
                ReleaseDataBuffer();
            }
        }

        //=========================================================================
        // GetTypeId
        //=========================================================================
        V::u32     CompressorZLib::TypeId()
        {
            return V_CRC("ZLib", 0x73887d3a);
        }

        //=========================================================================
        // ReadHeaderAndData
        //=========================================================================
        bool        CompressorZLib::ReadHeaderAndData(CompressorStream* stream, V::u8* data, unsigned int dataSize)
        {
            if (stream->GetCompressorData() != nullptr)  // we already have compressor data
            {
                return false;
            }

            // Read the ZLib header should be after the default compression header...
            // We should not be in this function otherwise.
            if (dataSize < sizeof(CompressorZLibHeader) + sizeof(ZLib::Header))
            {
                V_Assert(false, "We did not read enough data, we have only %d bytes left in the buffer and we need %d!", dataSize, sizeof(CompressorZLibHeader) + sizeof(ZLib::Header));
                return false;
            }

            AcquireDataBuffer();

            CompressorZLibHeader* hdr = reinterpret_cast<CompressorZLibHeader*>(data);
            VStd::endian_swap(hdr->NumSeekPoints);
            dataSize -= sizeof(CompressorZLibHeader);
            data += sizeof(CompressorZLibHeader);

            CompressorZLibData* zlibData = vnew CompressorZLibData;
            zlibData->CompressorHandle = this;
            zlibData->UncompressedSize = 0;
            zlibData->ZlibHeader = *reinterpret_cast<ZLib::Header*>(data);
            dataSize -= sizeof(zlibData->ZlibHeader);
            data += sizeof(zlibData->ZlibHeader);
            zlibData->DecompressNextOffset = sizeof(CompressorHeader) + sizeof(CompressorZLibHeader) + sizeof(ZLib::Header); // start after the headers

            V_Assert(hdr->NumSeekPoints > 0, "We should have at least one seek point for the entire stream!");

            // go the end of the file and read all sync points.
            SizeType compressedFileEnd = stream->GetLength();
            if (compressedFileEnd == 0)
            {
                delete zlibData;
                return false;
            }

            zlibData->SeekPoints.resize(hdr->NumSeekPoints);
            SizeType dataToRead = sizeof(CompressorZLibSeekPoint) * static_cast<SizeType>(hdr->NumSeekPoints);
            SizeType seekPointOffset = compressedFileEnd - dataToRead;
            V_Assert(seekPointOffset <= compressedFileEnd, "We have an invalid archive, this is impossible!");
            GenericStream* baseStream = stream->GetWrappedStream();
            if (baseStream->ReadAtOffset(dataToRead, zlibData->SeekPoints.data(), seekPointOffset) != dataToRead)
            {
                delete zlibData;
                return false;
            }
            for (size_t i = 0; i < zlibData->SeekPoints.size(); ++i)
            {
                VStd::endian_swap(zlibData->SeekPoints[i].CompressedOffset);
                VStd::endian_swap(zlibData->SeekPoints[i].UncompressedOffset);
            }

            if (m_decompressionCachePerStream)
            {
                zlibData->DecompressedCache = reinterpret_cast<unsigned char*>(vmalloc(m_decompressionCachePerStream, m_CompressedDataBufferAlignment, V::SystemAllocator, "CompressorZLib"));
            }

            zlibData->DecompressLastOffset = seekPointOffset; // set the start address of the seek points as the last valid read address for the compressed stream.

            zlibData->ZlibHandle.StartDecompressor(&zlibData->ZlibHeader);

            stream->SetCompressorData(zlibData);

            return true;
        }

        //=========================================================================
        // WriteHeaderAndData
        //=========================================================================
        bool CompressorZLib::WriteHeaderAndData(CompressorStream* stream)
        {
            if (!Compressor::WriteHeaderAndData(stream))
            {
                return false;
            }

            CompressorZLibData* compressorData = static_cast<CompressorZLibData*>(stream->GetCompressorData());
            CompressorZLibHeader header;
            header.NumSeekPoints = static_cast<V::u32>(compressorData->SeekPoints.size());
            VStd::endian_swap(header.NumSeekPoints);
            GenericStream* baseStream = stream->GetWrappedStream();
            if (baseStream->WriteAtOffset(sizeof(header), &header, sizeof(CompressorHeader)) == sizeof(header))
            {
                return true;
            }

            return false;
        }

        //=========================================================================
        // FillFromDecompressCache
        //=========================================================================
        inline CompressorZLib::SizeType CompressorZLib::FillFromDecompressCache(CompressorZLibData* zlibData, void*& buffer, SizeType& byteSize, SizeType& offset)
        {
            SizeType firstOffsetInCache = zlibData->DecompressedCacheOffset;
            SizeType lastOffsetInCache = firstOffsetInCache + zlibData->DecompressedCacheDataSize;
            SizeType firstDataOffset = offset;
            SizeType lastDataOffset = offset + byteSize;
            SizeType numCopied = 0;
            if (firstOffsetInCache < lastDataOffset && lastOffsetInCache > firstDataOffset)  // check if there is data in the cache
            {
                size_t copyOffsetStart = 0;
                size_t copyOffsetEnd = zlibData->DecompressedCacheDataSize;

                size_t bufferCopyOffset = 0;

                if (firstOffsetInCache < firstDataOffset)
                {
                    copyOffsetStart = static_cast<size_t>(firstDataOffset - firstOffsetInCache);
                }
                else
                {
                    bufferCopyOffset = static_cast<size_t>(firstOffsetInCache - firstDataOffset);
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
                memcpy(static_cast<char*>(buffer) + bufferCopyOffset, zlibData->DecompressedCache + copyOffsetStart, static_cast<size_t>(numCopied));

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

        //=========================================================================
        // FillFromCompressedCache
        //=========================================================================
        inline CompressorZLib::SizeType CompressorZLib::FillCompressedBuffer(CompressorStream* stream)
        {
            CompressorZLibData* zlibData = static_cast<CompressorZLibData*>(stream->GetCompressorData());
            SizeType dataFromBuffer = 0;
            if (stream == m_lastReadStream)  // if the buffer is filled with data from the current stream, try to reuse
            {
                if (zlibData->DecompressNextOffset > m_lastReadStreamOffset)
                {
                    SizeType offsetInCache = zlibData->DecompressNextOffset - m_lastReadStreamOffset;
                    if (offsetInCache < m_lastReadStreamSize)  // last check if there is data overlap
                    {
                        // copy the usable part at the start of the
                        SizeType toMove = m_lastReadStreamSize - offsetInCache;
                        memmove(m_compressedDataBuffer, &m_compressedDataBuffer[static_cast<size_t>(offsetInCache)], static_cast<size_t>(toMove));
                        dataFromBuffer += toMove;
                    }
                }
            }

            SizeType toReadFromStream = m_compressedDataBufferSize - dataFromBuffer;
            SizeType readOffset = zlibData->DecompressNextOffset + dataFromBuffer;
            if (readOffset + toReadFromStream > zlibData->DecompressLastOffset)
            {
                // don't read pass the end
                V_Assert(readOffset <= zlibData->DecompressLastOffset, "Read offset should always be before the end of stream!");
                toReadFromStream = zlibData->DecompressLastOffset - readOffset;
            }

            SizeType numReadFromStream = 0;
            if (toReadFromStream)  // if we did not reuse the whole buffer, read some data from the stream
            {
                GenericStream* baseStream = stream->GetWrappedStream();
                numReadFromStream = baseStream->ReadAtOffset(toReadFromStream, &m_compressedDataBuffer[static_cast<size_t>(dataFromBuffer)], readOffset);
            }

            // update what's actually in the read data buffer.
            m_lastReadStream = stream;
            m_lastReadStreamOffset = zlibData->DecompressNextOffset;
            m_lastReadStreamSize = dataFromBuffer + numReadFromStream;
            return m_lastReadStreamSize;
        }

        /**
         * Helper class to find the best seek point for a specific offset.
         */
        struct CompareUpper
        {
            inline bool operator()(const V::u64& offset, const CompressorZLibSeekPoint& sp) const {return offset < sp.UncompressedOffset; }
        };

        //=========================================================================
        // Read
        //=========================================================================
        CompressorZLib::SizeType    CompressorZLib::Read(CompressorStream* stream, SizeType byteSize, SizeType offset, void* buffer)
        {
            V_Assert(stream->GetCompressorData(), "This stream doesn't have decompression enabled!");
            CompressorZLibData* zlibData = static_cast<CompressorZLibData*>(stream->GetCompressorData());
            V_Assert(!zlibData->ZlibHandle.IsCompressorStarted(), "You can't read/decompress while writing a compressed stream %s!");

            // check if the request can be finished from the decompressed cache
            SizeType numRead = FillFromDecompressCache(zlibData, buffer, byteSize, offset);
            if (byteSize == 0)  // are we done
            {
                return numRead;
            }

            // find the best seek point for current offset
            CompressorZLibData::SeekPointArray::iterator it = VStd::upper_bound(zlibData->SeekPoints.begin(), zlibData->SeekPoints.end(), offset, CompareUpper());
            V_Assert(it != zlibData->SeekPoints.begin(), "This should be impossible, we should always have a valid seek point at 0 offset!");
            const CompressorZLibSeekPoint& bestSeekPoint = *(--it);  // get the previous (so it includes the current offset)

            // if read is continuous continue with decompression
            bool isJumpToSeekPoint = false;
            SizeType lastOffsetInCache = zlibData->DecompressedCacheOffset + zlibData->DecompressedCacheDataSize;
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
                zlibData->DecompressNextOffset = bestSeekPoint.CompressedOffset;  // set next read point
                zlibData->DecompressedCacheOffset = bestSeekPoint.UncompressedOffset; // set uncompressed offset
                zlibData->DecompressedCacheDataSize = 0; // invalidate the cache
                zlibData->ZlibHandle.ResetDecompressor(&zlibData->ZlibHeader); // reset decompressor and setup the header.
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
                    zlibData->DecompressedCacheOffset += zlibData->DecompressedCacheDataSize;

                    // decompress in the cache buffer
                    u32 availDecompressedCacheSize = m_decompressionCachePerStream; // reset buffer size
                    unsigned int processed = zlibData->ZlibHandle.Decompress(&m_compressedDataBuffer[processedCompressedData], static_cast<unsigned int>(compressedDataSize) - processedCompressedData, zlibData->DecompressedCache, availDecompressedCacheSize);
                    zlibData->DecompressedCacheDataSize = m_decompressionCachePerStream - availDecompressedCacheSize;
                    if (processed == 0)
                    {
                        break; // we processed everything we could, load more compressed data.
                    }
                    processedCompressedData += processed;
                    // fill what we can from the cache
                    numRead += FillFromDecompressCache(zlibData, buffer, byteSize, offset);
                }
                // update next read position the the compressed stream
                zlibData->DecompressNextOffset +=  processedCompressedData;
            }
            return numRead;
        }

        //=========================================================================
        // Write
        // [12/13/2012]
        //=========================================================================
        CompressorZLib::SizeType    CompressorZLib::Write(CompressorStream* stream, SizeType byteSize, const void* data, SizeType offset)
        {
            (void)offset;

            V_Assert(stream && stream->GetCompressorData(), "This stream doesn't have compression enabled! Call Stream::WriteCompressed after you create the file!");
            V_Assert(offset == SizeType(-1) || offset == stream->GetCurPos(), "We can write compressed data only at the end of the stream!");

            m_lastReadStream = nullptr; // invalidate last read position, otherwise m_dataBuffer will be corrupted (as we are about to write in it).

            CompressorZLibData* zlibData = static_cast<CompressorZLibData*>(stream->GetCompressorData());
            V_Assert(!zlibData->ZlibHandle.IsDecompressorStarted(), "You can't write while reading/decompressing a compressed stream!");

            const u8* bytes = reinterpret_cast<const u8*>(data);
            unsigned int dataToCompress = static_cast<unsigned int>(byteSize);
            while (dataToCompress != 0)
            {
                unsigned int oldDataToCompress = dataToCompress;
                unsigned int compressedSize = zlibData->ZlibHandle.Compress(bytes, dataToCompress, m_compressedDataBuffer, m_compressedDataBufferSize);
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
            zlibData->UncompressedSize += byteSize;

            if (zlibData->AutoSeekSize > 0)
            {
                // insert a seek point if needed.
                if (zlibData->SeekPoints.empty())
                {
                    if (zlibData->UncompressedSize >=  zlibData->AutoSeekSize)
                    {
                        WriteSeekPoint(stream);
                    }
                }
                else if ((zlibData->UncompressedSize - zlibData->SeekPoints.back().UncompressedOffset) > zlibData->AutoSeekSize)
                {
                    WriteSeekPoint(stream);
                }
            }
            return byteSize;
        }

        //=========================================================================
        // WriteSeekPoint
        //=========================================================================
        bool        CompressorZLib::WriteSeekPoint(CompressorStream* stream)
        {
            V_Assert(stream && stream->GetCompressorData(), "This stream doesn't have compression enabled! Call Stream::WriteCompressed after you create the file!");
            CompressorZLibData* zlibData = static_cast<CompressorZLibData*>(stream->GetCompressorData());

            m_lastReadStream = nullptr; // invalidate last read position, otherwise m_dataBuffer will be corrupted (as we are about to write in it).

            unsigned int compressedSize;
            unsigned int dataToCompress = 0;
            do
            {
                compressedSize = zlibData->ZlibHandle.Compress(nullptr, dataToCompress, m_compressedDataBuffer, m_compressedDataBufferSize, ZLib::FT_FULL_FLUSH);
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

            CompressorZLibSeekPoint sp;
            sp.CompressedOffset = stream->GetLength();
            sp.UncompressedOffset = zlibData->UncompressedSize;
            zlibData->SeekPoints.push_back(sp);
            return true;
        }

        //=========================================================================
        // StartCompressor
        //=========================================================================
        bool        CompressorZLib::StartCompressor(CompressorStream* stream, int compressionLevel, SizeType autoSeekDataSize)
        {
            V_Assert(stream && stream->GetCompressorData() == nullptr, "Stream has compressor already enabled!");

            AcquireDataBuffer();

            CompressorZLibData* zlibData = vnew CompressorZLibData;
            zlibData->CompressorHandle = this;
            zlibData->ZlibHeader = 0; // not used for compression
            zlibData->UncompressedSize = 0;
            zlibData->AutoSeekSize = autoSeekDataSize;
            compressionLevel = V::GetClamp(compressionLevel, 1, 9); // remap to zlib levels

            zlibData->ZlibHandle.StartCompressor(compressionLevel);

            stream->SetCompressorData(zlibData);

            if (WriteHeaderAndData(stream))
            {
                // add the first and always present seek point at the start of the compressed stream
                CompressorZLibSeekPoint sp;
                sp.CompressedOffset = sizeof(CompressorHeader) + sizeof(CompressorZLibHeader) + sizeof(zlibData->ZlibHeader);
                sp.UncompressedOffset = 0;
                zlibData->SeekPoints.push_back(sp);
                return true;
            }
            return false;
        }

        //=========================================================================
        // Close
        //=========================================================================
        bool CompressorZLib::Close(CompressorStream* stream)
        {
            V_Assert(stream->IsOpen(), "Stream is not open to be closed!");

            CompressorZLibData* zlibData = static_cast<CompressorZLibData*>(stream->GetCompressorData());
            GenericStream* baseStream = stream->GetWrappedStream();

            bool result = true;
            if (zlibData->ZlibHandle.IsCompressorStarted())
            {
                m_lastReadStream = nullptr; // invalidate last read position, otherwise m_dataBuffer will be corrupted (as we are about to write in it).

                // flush all compressed data
                unsigned int compressedSize;
                unsigned int dataToCompress = 0;
                do
                {
                    compressedSize = zlibData->ZlibHandle.Compress(nullptr, dataToCompress, m_compressedDataBuffer, m_compressedDataBufferSize, ZLib::FT_FINISH);
                    if (compressedSize)
                    {
                        baseStream->Write(compressedSize, m_compressedDataBuffer);
                    }
                } while (dataToCompress != 0);

                result = WriteHeaderAndData(stream);
                if (result)
                {
                    // now write the seek points and the end of the file
                    for (size_t i = 0; i < zlibData->SeekPoints.size(); ++i)
                    {
                        VStd::endian_swap(zlibData->SeekPoints[i].CompressedOffset);
                        VStd::endian_swap(zlibData->SeekPoints[i].UncompressedOffset);
                    }
                    SizeType dataToWrite = zlibData->SeekPoints.size() * sizeof(CompressorZLibSeekPoint);
                    baseStream->Seek(0U, GenericStream::SeekMode::ST_SEEK_END);
                    result = (baseStream->Write(dataToWrite, zlibData->SeekPoints.data()) == dataToWrite);
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
            if (zlibData->DecompressedCache)
            {
                vfree(zlibData->DecompressedCache, V::SystemAllocator, m_decompressionCachePerStream, m_CompressedDataBufferAlignment);
            }

            ReleaseDataBuffer();

            // last step reset strream compressor data.
            stream->SetCompressorData(nullptr);
            return result;
        }

        //=========================================================================
        // AcquireDataBuffer
        //=========================================================================
        void CompressorZLib::AcquireDataBuffer()
        {
            if (m_compressedDataBuffer == nullptr)
            {
                V_Assert(m_compressedDataBufferUseCount == 0, "Buffer usecount should be 0 if the buffer is NULL");
                m_compressedDataBuffer = reinterpret_cast<unsigned char*>(vmalloc(m_compressedDataBufferSize, m_CompressedDataBufferAlignment, V::SystemAllocator, "CompressorZLib"));
                m_lastReadStream = nullptr; // reset the cache info in the m_dataBuffer
            }
            ++m_compressedDataBufferUseCount;
        }

        //=========================================================================
        // ReleaseDataBuffer
        //=========================================================================
        void CompressorZLib::ReleaseDataBuffer()
        {
            --m_compressedDataBufferUseCount;
            if (m_compressedDataBufferUseCount == 0)
            {
                V_Assert(m_compressedDataBuffer != nullptr, "Invalid data buffer! We should have a non null pointer!");
                vfree(m_compressedDataBuffer, V::SystemAllocator, m_compressedDataBufferSize, m_CompressedDataBufferAlignment);
                m_compressedDataBuffer = nullptr;
                m_lastReadStream = nullptr; // reset the cache info in the m_dataBuffer
            }
        }
    }   // namespace IO
}   // namespace AZ

#endif // #if !defined(AZCORE_EXCLUDE_ZLIB)
