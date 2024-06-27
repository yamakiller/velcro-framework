#include <vcore/detector/stream.h>



#include <vcore/std/time.h>

#if !defined(VCORE_EXCLUDE_ZLIB)
#   define V_FILE_STREAM_COMPRESSION
#endif // VCORE_EXCLUDE_ZLIB

#if defined(V_FILE_STREAM_COMPRESSION)
#   include <vcore/compression/compression.h>
#endif // V_FILE_STREAM_COMPRESSION

namespace V
{
    namespace Debug
    {
        //////////////////////////////////////////////////////////////////////////
        //////////////////////////////////////////////////////////////////////////
        // Detector output stream
        //////////////////////////////////////////////////////////////////////////
        //////////////////////////////////////////////////////////////////////////
        void DetectorOutputStream::WriteHeader()
        {
            StreamHeader sh; // StreamHeader should be endianess independent.
            WriteBinary(&sh, sizeof(sh));
        }

        void DetectorOutputStream::WriteTimeUTC(u32 name)
        {
            VStd::sys_time_t now = VStd::GetTimeUTCMilliSecond();
            Write(name, now);
        }

        void DetectorOutputStream::WriteTimeMicrosecond(u32 name)
        {
            VStd::sys_time_t now = VStd::GetTimeNowMicroSecond();
            Write(name, now);
        }

        //////////////////////////////////////////////////////////////////////////
        //////////////////////////////////////////////////////////////////////////
        // Detector Input Stream
        //////////////////////////////////////////////////////////////////////////
        //////////////////////////////////////////////////////////////////////////
        bool DetectorInputStream::ReadHeader()
        {
            DetectorOutputStream::StreamHeader sh; // StreamHeader should be endianess independent.
            unsigned int numRead = ReadBinary(&sh, sizeof(sh));
            (void)numRead;
            V_Error("IO", numRead == sizeof(sh), "We should have atleast %d bytes in the stream to read the header!", sizeof(sh));
            if (numRead != sizeof(sh))
            {
                return false;
            }
            m_isEndianSwap = V::IsBigEndian(static_cast<V::PlatformID>(sh.platform)) != V::IsBigEndian(V::g_currentPlatform);
            return true;
        }

        //////////////////////////////////////////////////////////////////////////
        //////////////////////////////////////////////////////////////////////////
        // Detector file stream
        //////////////////////////////////////////////////////////////////////////
        //////////////////////////////////////////////////////////////////////////
        //=========================================================================
        // DetectorOutputFileStream::DetectorOutputFileStream
        //=========================================================================
        DetectorOutputFileStream::DetectorOutputFileStream()
        {
#if defined(V_FILE_STREAM_COMPRESSION)
            m_zlib = vcreate(ZLib, (&AllocatorInstance<OSAllocator>::GetAllocator()), OSAllocator);
            m_zlib->StartCompressor(2);
#endif
        }

        //=========================================================================
        // DetectorOutputFileStream::~DetectorOutputFileStream
        //=========================================================================
        DetectorOutputFileStream::~DetectorOutputFileStream()
        {
#if defined(V_FILE_STREAM_COMPRESSION)
            vdestroy(m_zlib, OSAllocator);
#endif
        }

        //=========================================================================
        // DetectorOutputFileStream::Open
        //=========================================================================
        bool DetectorOutputFileStream::Open(const char* fileName, int mode, int platformFlags)
        {
            if (IO::SystemFile::Open(fileName, mode, platformFlags))
            {
                m_dataBuffer.reserve(100 * 1024);
#if defined(V_FILE_STREAM_COMPRESSION)
                //              // Enable optional: encode the file in the same format as the streamer so they are interchangeable
                //              IO::CompressorHeader ch;
                //              ch.SetVCS();
                //              ch.m_compressorId = IO::CompressorZLib::TypeId();
                //              ch.m_uncompressedSize = 0; // will be updated later
                //              VStd::endian_swap(ch.m_compressorId);
                //              VStd::endian_swap(ch.m_uncompressedSize);
                //              IO::SystemFile::Write(&ch,sizeof(ch));
                //              IO::CompressorZLibHeader zlibHdr;
                //              zlibHdr.m_numSeekPoints = 0;
                //              IO::SystemFile::Write(&zlibHdr,sizeof(zlibHdr));
#endif
                return true;
            }
            return false;
        }

        //=========================================================================
        // DetectorOutputFileStream::Close
        //=========================================================================
        void DetectorOutputFileStream::Close()
        {
            unsigned int dataSizeInBuffer = static_cast<unsigned int>(m_dataBuffer.size());
            {
#if defined(V_FILE_STREAM_COMPRESSION)
                unsigned int minCompressBufferSize = m_zlib->GetMinCompressedBufferSize(dataSizeInBuffer);
                if (m_compressionBuffer.size() < minCompressBufferSize) // grow compression buffer if needed
                {
                    m_compressionBuffer.clear();
                    m_compressionBuffer.resize(minCompressBufferSize);
                }
                unsigned int compressedSize;
                do
                {
                    compressedSize = m_zlib->Compress(m_dataBuffer.data(), dataSizeInBuffer, m_compressionBuffer.data(), (unsigned)m_compressionBuffer.size(), ZLib::FT_FINISH);
                    if (compressedSize)
                    {
                        IO::SystemFile::Write(m_compressionBuffer.data(), compressedSize);
                    }
                } while (compressedSize > 0);
                m_zlib->ResetCompressor();
#else
                if (dataSizeInBuffer) {
                    IO::SystemFile::Write(m_dataBuffer.data(), m_dataBuffer.size());
                }
#endif
                m_dataBuffer.clear();
            }
            IO::SystemFile::Close();
        }
        //=========================================================================
        // DetectorOutputFileStream::WriteBinary
        
        //=========================================================================
        void DetectorOutputFileStream::WriteBinary(const void* data, unsigned int dataSize)
        {
            size_t dataSizeInBuffer = m_dataBuffer.size();
            if (dataSizeInBuffer + dataSize > m_dataBuffer.capacity())
            {
                if (dataSizeInBuffer > 0)
                {
#if defined(V_FILE_STREAM_COMPRESSION)
                    // we need to flush the data
                    unsigned int dataToCompress = static_cast<unsigned int>(dataSizeInBuffer);
                    unsigned int minCompressBufferSize = m_zlib->GetMinCompressedBufferSize(dataToCompress);
                    if (m_compressionBuffer.size() < minCompressBufferSize) // grow compression buffer if needed
                    {
                        m_compressionBuffer.clear();
                        m_compressionBuffer.resize(minCompressBufferSize);
                    }
                    while (dataToCompress > 0)
                    {
                        unsigned int compressedSize = m_zlib->Compress(m_dataBuffer.data(), dataToCompress, m_compressionBuffer.data(), (unsigned)m_compressionBuffer.size());
                        if (compressedSize)
                        {
                            IO::SystemFile::Write(m_compressionBuffer.data(), compressedSize);
                        }
                    }
#else
                    IO::SystemFile::Write(m_dataBuffer.data(), m_dataBuffer.size());
#endif
                    m_dataBuffer.clear();
                }
            }
            m_dataBuffer.insert(m_dataBuffer.end(), reinterpret_cast<const unsigned char*>(data), reinterpret_cast<const unsigned char*>(data) + dataSize);
        }

        //////////////////////////////////////////////////////////////////////////
        //////////////////////////////////////////////////////////////////////////
        // Detector file input stream
        //////////////////////////////////////////////////////////////////////////
        //////////////////////////////////////////////////////////////////////////

        //=========================================================================
        // DetectorInputFileStream::DetectorInputFileStream
        
        //=========================================================================
        DetectorInputFileStream::DetectorInputFileStream()
        {
#if defined(V_FILE_STREAM_COMPRESSION)
            m_zlib = vcreate(ZLib, (&AllocatorInstance<OSAllocator>::GetAllocator()), OSAllocator);
            m_zlib->StartDecompressor();
#endif
        }

        //=========================================================================
        // DetectorInputFileStream::DetectorInputFileStream
        
        //=========================================================================
        DetectorInputFileStream::~DetectorInputFileStream()
        {
#if defined(V_FILE_STREAM_COMPRESSION)
            vdestroy(m_zlib, OSAllocator);
#endif
        }

        //=========================================================================
        // DetectorInputFileStream::Open
        
        //=========================================================================
        bool DetectorInputFileStream::Open(const char* fileName, int mode, int platformFlags)
        {
            if (IO::SystemFile::Open(fileName, mode, platformFlags))
            {
                DetectorOutputStream::StreamHeader sh;
#if defined(V_FILE_STREAM_COMPRESSION)
                // TODO: optional encode the file in the same format as the streamer so they are interchangeable
#endif
                // first read the header of the stream file.
                return ReadHeader();
            }
            return false;
        }
        //=========================================================================
        // DetectorInputFileStream::ReadBinary
        
        //=========================================================================
        unsigned int DetectorInputFileStream::ReadBinary(void* data, unsigned int maxDataSize)
        {
            // make sure the compressed buffer if full enough...
            size_t dataToLoad = maxDataSize * 2;
            m_compressedData.reserve(dataToLoad);
            while (m_compressedData.size() < dataToLoad)
            {
                unsigned char buffer[10 * 1024];
                IO::SystemFile::SizeType bytesRead = Read(V_ARRAY_SIZE(buffer), buffer);
                if (bytesRead > 0)
                {
                    m_compressedData.insert(m_compressedData.end(), (unsigned char*)buffer, buffer + bytesRead);
                }
                if (bytesRead < V_ARRAY_SIZE(buffer))
                {
                    break;
                }
            }
#if defined(V_FILE_STREAM_COMPRESSION)
            unsigned int dataSize = maxDataSize;
            unsigned int bytesProcessed = m_zlib->Decompress(m_compressedData.data(), (unsigned)m_compressedData.size(), data, dataSize);
            unsigned int readSize = maxDataSize - dataSize; // Zlib::Decompress decrements the dataSize parameter by the amount uncompressed
#else
            unsigned int bytesProcessed = VStd::GetMin((unsigned int)m_compressedData.size(), maxDataSize);
            unsigned int readSize = bytesProcessed;
            memcpy(data, m_compressedData.data(), readSize);
#endif
            m_compressedData.erase(m_compressedData.begin(), m_compressedData.begin() + bytesProcessed);
            return readSize;
        }

        //=========================================================================
        // DetectorInputFileStream::Close
        
        //=========================================================================
        void DetectorInputFileStream::Close()
        {
#if defined(V_FILE_STREAM_COMPRESSION)
            if (m_zlib)
            {
                m_zlib->ResetDecompressor();
            }
#endif // V_FILE_STREAM_COMPRESSION
            V::IO::SystemFile::Close();
        }

        //////////////////////////////////////////////////////////////////////////
        //////////////////////////////////////////////////////////////////////////
        // DetectorSAXParser
        //////////////////////////////////////////////////////////////////////////
        //////////////////////////////////////////////////////////////////////////
        //=========================================================================
        // DetectorSAXParser
        
        //=========================================================================
        DetectorSAXParser::DetectorSAXParser(const TagCallbackType& tcb, const DataCallbackType& dcb)
            : m_tagCallback(tcb)
            , m_dataCallback(dcb)
        {
        }

        //=========================================================================
        // ProcessStream
        
        //=========================================================================
        void
        DetectorSAXParser::ProcessStream(DetectorInputStream& stream)
        {
            static const int processChunkSize = 15 * 1024;
            char buffer[processChunkSize];
            unsigned int dataSize;
            bool isEndianSwap = stream.IsEndianSwap();
            while ((dataSize = stream.ReadBinary(buffer, processChunkSize)) > 0)
            {
                char* dataStart = buffer;
                char* dataEnd = dataStart + dataSize;
                bool dataInBuffer = false;
                if (!m_buffer.empty())
                {
                    m_buffer.insert(m_buffer.end(), dataStart, dataEnd);
                    dataStart = m_buffer.data();
                    dataEnd = dataStart + m_buffer.size();
                    dataInBuffer = true;
                }
                const int entrySize = sizeof(DetectorOutputStream::StreamEntry);
                while (dataStart != dataEnd)
                {
                    if ((dataEnd - dataStart) < entrySize) // we need at least one entry to proceed
                    {
                        // not enough data to process, buffer it.
                        if (!dataInBuffer)
                        {
                            m_buffer.insert(m_buffer.end(), dataStart, dataEnd);
                        }
                        break;
                    }

                    DetectorOutputStream::StreamEntry* se = reinterpret_cast<DetectorOutputStream::StreamEntry*>(dataStart);
                    if (isEndianSwap)
                    {
                        // endian swap
                        VStd::endian_swap(se->name);
                        VStd::endian_swap(se->sizeAndFlags);
                    }

                    u32 dataType = (se->sizeAndFlags & DetectorOutputStream::StreamEntry::dataInternalMask) >> DetectorOutputStream::StreamEntry::dataInternalShift;
                    u32 value = se->sizeAndFlags & DetectorOutputStream::StreamEntry::dataSizeMask;
                    Data de;
                    de.m_name = se->name;
                    de.m_stringPool = stream.GetStringPool();
                    de.m_isPooledString = false;
                    de.m_isPooledStringCrc32 = false;
                    switch (dataType)
                    {
                    case DetectorOutputStream::StreamEntry::INT_TAG:
                    {
                        bool isStart = (value != 0);
                        m_tagCallback(se->name, isStart);
                        dataStart += entrySize;
                    } break;
                    case DetectorOutputStream::StreamEntry::INT_DATA_U8:
                    {
                        u8 value8 = static_cast<u8>(value);
                        de.m_data = &value8;
                        de.m_dataSize = 1;
                        de.m_isEndianSwap = false;
                        m_dataCallback(de);
                        dataStart += entrySize;
                    } break;
                    case DetectorOutputStream::StreamEntry::INT_DATA_U16:
                    {
                        u16 value16 = static_cast<u16>(value);
                        de.m_data = &value16;
                        de.m_dataSize = 2;
                        de.m_isEndianSwap = false;
                        m_dataCallback(de);
                        dataStart += entrySize;
                    } break;
                    case DetectorOutputStream::StreamEntry::INT_DATA_U29:
                    {
                        de.m_data = &value;
                        de.m_dataSize = 4;
                        de.m_isEndianSwap = false;
                        m_dataCallback(de);
                        dataStart += entrySize;
                    } break;
                    case DetectorOutputStream::StreamEntry::INT_POOLED_STRING:
                    {
                        unsigned int userDataSize = value;
                        if ((userDataSize + entrySize) <= (unsigned)(dataEnd - dataStart))
                        {
                            // Add string to the pool
                            V_Assert(de.m_stringPool != nullptr, "We require a string pool to parse this stream");
                            V::u32 crc32;
                            const char* stringPtr;
                            dataStart += entrySize;
                            de.m_stringPool->InsertCopy(reinterpret_cast<const char*>(dataStart), userDataSize, crc32, &stringPtr);
                            de.m_dataSize = userDataSize;
                            de.m_isEndianSwap = isEndianSwap;
                            de.m_isPooledString = true;
                            de.m_data = const_cast<void*>(static_cast<const void*>(stringPtr));
                            m_dataCallback(de);
                            dataStart += userDataSize;
                        }
                        else
                        {
                            // we can't process data right now add it to the buffer (if we have not done that already)
                            if (!dataInBuffer)
                            {
                                m_buffer.insert(m_buffer.end(), dataStart, dataEnd);
                            }
                            dataEnd = dataStart;     // exit the loop
                        }
                    } break;
                    case DetectorOutputStream::StreamEntry::INT_POOLED_STRING_CRC32:
                    {
                        de.m_isPooledStringCrc32 = true;
                        V_Assert(value == 4, "The data size for a pooled string crc32 should be 4 bytes!");
                    }     // continue to INT_SIZE
                    case DetectorOutputStream::StreamEntry::INT_SIZE:
                    {
                        unsigned int userDataSize = value;
                        if ((userDataSize + entrySize) <= (unsigned)(dataEnd - dataStart)) // do we have all the date we need to process...
                        {
                            dataStart += entrySize;
                            de.m_data = dataStart;
                            de.m_dataSize = userDataSize;
                            de.m_isEndianSwap = isEndianSwap;
                            m_dataCallback(de);
                            dataStart += userDataSize;
                        }
                        else
                        {
                            // we can't process data right now add it to the buffer (if we have not done that already)
                            if (!dataInBuffer)
                            {
                                m_buffer.insert(m_buffer.end(), dataStart, dataEnd);
                            }
                            dataEnd = dataStart;     // exit the loop
                        }
                    } break;
                    default:
                    {                        
                        V_Error("DetectorSAXParser",false,"Encounted unknown symbol (%i) while processing stream (%s). Aborting stream.\n",dataType, stream.GetIdentifier());

                        // If we can't process anything, we want to just escape the loop, to avoid spinning infinitely
                        dataEnd = dataStart;                        
                    } break;
                    }
                }
                if (dataInBuffer) // if the data was in the buffer remove the processed data!
                {
                    m_buffer.erase(m_buffer.begin(), m_buffer.begin() + (dataStart - m_buffer.data()));
                }
            }
        }

        const char*  DetectorSAXParser::Data::PrepareString(unsigned int& stringLength) const
        {
            const char* srcData = reinterpret_cast<const char*>(m_data);
            stringLength = m_dataSize;
            if (m_stringPool)
            {
                V::u32 crc32;
                const char* stringPtr;
                if (m_isPooledStringCrc32)
                {
                    crc32 = *reinterpret_cast<V::u32*>(m_data);
                    if (m_isEndianSwap)
                    {
                        VStd::endian_swap(crc32);
                    }
                    stringPtr = m_stringPool->Find(crc32);
                    V_Assert(stringPtr != nullptr, "Failed to find string with id 0x%08x in the string pool, proper stream read is impossible!", crc32);
                    stringLength = static_cast<unsigned int>(strlen(stringPtr));
                }
                else if (m_isPooledString)
                {
                    stringPtr = srcData; // already stored in the pool just transfer the pointer
                }
                else
                {
                    // Store copy of the string in the pool to save memory (keep only one reference of the string).
                    m_stringPool->InsertCopy(reinterpret_cast<const char*>(srcData), stringLength, crc32, &stringPtr);
                }
                srcData = stringPtr;
            }
            else
            {
                V_Assert(m_isPooledString == false && m_isPooledStringCrc32 == false, "This stream requires using of a string pool as the string is send only once and afterwards only the Crc32 is used!");
            }
            return srcData;
        }

        //////////////////////////////////////////////////////////////////////////
        //////////////////////////////////////////////////////////////////////////
        // DetectorDOMParser
        //////////////////////////////////////////////////////////////////////////
        //////////////////////////////////////////////////////////////////////////

        //=========================================================================
        // Node::GetTag
        // [1/23/2013]
        //=========================================================================
        const DetectorDOMParser::Node* DetectorDOMParser::Node::GetTag(u32 tagName) const
        {
            const Node* tagNode = nullptr;
            for (Node::NodeListType::const_iterator i = m_tags.begin(); i != m_tags.end(); ++i)
            {
                if ((*i).m_name == tagName)
                {
                    tagNode = &*i;
                    break;
                }
            }
            return tagNode;
        }

        //=========================================================================
        // Node::GetData
        
        //=========================================================================
        const DetectorDOMParser::Data* DetectorDOMParser::Node::GetData(u32 dataName) const
        {
            const Data* dataNode = nullptr;
            for (Node::DataListType::const_iterator i = m_data.begin(); i != m_data.end(); ++i)
            {
                if (i->m_name == dataName)
                {
                    dataNode = &*i;
                    break;
                }
            }
            return dataNode;
        }

        //=========================================================================
        // DetectorDOMParser
        
        //=========================================================================
        DetectorDOMParser::DetectorDOMParser(bool isPersistentInputData)
            : DetectorSAXParser(TagCallbackType(this, &DetectorDOMParser::OnTag), DataCallbackType(this, &DetectorDOMParser::OnData))
            , m_isPersistentInputData(isPersistentInputData)
        {
            m_root.m_name = 0;
            m_root.m_parent = nullptr;
            m_topNode = &m_root;
        }
        static int g_numFree = 0;
        //=========================================================================
        // ~DetectorDOMParser
        
        //=========================================================================
        DetectorDOMParser::~DetectorDOMParser()
        {
            DeleteNode(m_root);
        }

        //=========================================================================
        // OnTag
        
        //=========================================================================
        void
        DetectorDOMParser::OnTag(V::u32 name, bool isOpen)
        {
            if (isOpen)
            {
                m_topNode->m_tags.push_back();
                Node& node = m_topNode->m_tags.back();
                node.m_name = name;
                node.m_parent = m_topNode;

                m_topNode = &node;
            }
            else
            {
                V_Assert(m_topNode->m_name == name, "We have opened tag with name 0x%08x and closing with name 0x%08x", m_topNode->m_name, name);
                m_topNode = m_topNode->m_parent;
            }
        }
        //=========================================================================
        // OnData
        
        //=========================================================================
        void
        DetectorDOMParser::OnData(const Data& data)
        {
            Data de = data;
            if (!m_isPersistentInputData)
            {
                de.m_data = vmalloc(data.m_dataSize, 1, OSAllocator);
                memcpy(const_cast<void*>(de.m_data), data.m_data, data.m_dataSize);
            }
            m_topNode->m_data.push_back(de);
        }
        //=========================================================================
        // DeleteNode
        
        //=========================================================================
        void
        DetectorDOMParser::DeleteNode(Node& node)
        {
            if (!m_isPersistentInputData)
            {
                for (Node::DataListType::iterator iter = node.m_data.begin(); iter != node.m_data.end(); ++iter)
                {
                    vfree(iter->m_data, OSAllocator, iter->m_dataSize);
                    ++g_numFree;
                }
                node.m_data.clear();
            }
            for (Node::NodeListType::iterator iter = node.m_tags.begin(); iter != node.m_tags.end(); ++iter)
            {
                DeleteNode(*iter);
            }
            node.m_tags.clear();
        }

        //////////////////////////////////////////////////////////////////////////
        //////////////////////////////////////////////////////////////////////////
        // DetectorSAXParserHandler
        //////////////////////////////////////////////////////////////////////////
        //////////////////////////////////////////////////////////////////////////

        //=========================================================================
        // DetectorSAXParserHandler
        //=========================================================================
        DetectorSAXParserHandler::DetectorSAXParserHandler(DetectorHandlerParser* rootHandler)
            : DetectorSAXParser(TagCallbackType(this, &DetectorSAXParserHandler::OnTag), DataCallbackType(this, &DetectorSAXParserHandler::OnData))
        {
            // Push the root element
            m_stack.push_back(rootHandler);
        }

        //=========================================================================
        // OnTag
        //=========================================================================
        void DetectorSAXParserHandler::OnTag(u32 name, bool isOpen)
        {
            if (m_stack.size() == 0)
            {
                return;
            }

            DetectorHandlerParser* childHandler = nullptr;
            DetectorHandlerParser* currentHandler = m_stack.back();
            if (isOpen)
            {
                if (currentHandler != nullptr)
                {
                    childHandler = currentHandler->OnEnterTag(name);
                    V_Warning("Detector", !currentHandler->IsWarnOnUnsupportedTags() || childHandler != nullptr, "Could not find handler for tag 0x%08x", name);
                }
                m_stack.push_back(childHandler);
            }
            else
            {
                m_stack.pop_back();
                if (m_stack.size() > 0)
                {
                    DetectorHandlerParser* parentHandler = m_stack.back();
                    if (parentHandler)
                    {
                        parentHandler->OnExitTag(currentHandler, name);
                    }
                }
            }
        }

        //=========================================================================
        // OnData
        //=========================================================================
        void DetectorSAXParserHandler::OnData(const DetectorSAXParser::Data& data)
        {
            if (m_stack.size() == 0)
            {
                return;
            }

            DetectorHandlerParser* currentHandler = m_stack.back();
            if (currentHandler)
            {
                currentHandler->OnData(data);
            }
        }
    } // namespace Debug
} // namespace V