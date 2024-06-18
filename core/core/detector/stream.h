#ifndef V_FRAMEWORK_CORE_DETECTOR_STREAM_H
#define V_FRAMEWORK_CORE_DETECTOR_STREAM_H

#include <core/memory/osallocator.h>

#include <core/std/delegate/delegate.h>
#include <core/std/containers/vector.h>
#include <core/std/containers/forward_list.h>
#include <core/std/typetraits/is_signed.h>
#include <core/std/typetraits/is_pod.h>

#include <core/io/system_file.h> // for the Detector direct file stream
#include <core/platform_id/platform_id.h>
#include <core/std/string/string.h>

namespace V {
    class ZLib;

    namespace IO {
        class Stream;
    }

    namespace Debug
    {
        template<class T>
        struct vector
        {
            typedef VStd::vector<T, OSStdAllocator> type;
        };

        template<class T>
        struct forward_list
        {
            typedef VStd::forward_list<T, OSStdAllocator> type;
        };

        /**
         * Interface for a string pool which can be used by input/output streams to avoid storing multiple copies of the same
         * string in the stream. Of course this comes at the bookkeeping cost of the table.
         */
        class DetectorStringPool
        {
        public:
            virtual ~DetectorStringPool()    {}

            /**
             * Add a copy of the string to the pool. If we return true the string was added otherwise it was already in the bool.
             * In both cases the crc32 of the string and the pointer to the shared copy is returned (optional for poolStringAddress)!
             */
            virtual bool    InsertCopy(const char* string, unsigned int length, V::u32& crc32, const char** poolStringAddress = NULL) = 0;

            /**
             * Same as the InsertCopy above without actually coping the string into the pool. The pool assumes that
             * none of the strings added to the pool will be deleted.
             */
            virtual bool    Insert(const char* string, unsigned int length, V::u32& crc32) = 0;

            /// Finds a string in the pool by crc32.
            virtual const char* Find(V::u32 crc32) = 0;

            virtual void    Erase(V::u32 crc32) = 0;

            /// Clears all the strings in the pool, make sure you don't reference any strings before you call that function.
            virtual void    Reset() = 0;
        };

        /**
         *
         */
        class DetectorOutputStream
        {
        protected:
            friend class DetectorManagerImpl;
            friend class DetectorSAXParser;
            struct StreamEntry
            {
                enum InternalDataSize // max 8 values as we use 3 bit to store them
                {
                    INT_SIZE = 0,   ///< No internal data, we store the data size. IMPORTANT: INT_SIZE should be 0 the code makes assumptions based on that
                    INT_TAG,        ///< True if this entry is tag
                    INT_DATA_U8,    ///< Internal data u8 stored (1 byte)
                    INT_DATA_U16,   ///< Internal data u16 stored (2 bytes)
                    INT_DATA_U29,   ///< Internal data u32 stored (4 bytes) for which we use only the first 29 bits.
                    INT_POOLED_STRING_CRC32,    ///< Data size should be 4 bytes crc32 that a string CRC and it require string pool.
                    INT_POOLED_STRING,  ///< This data contains a string which should be inserted in the string pool.
                };
                static const u32 dataSizeMask   = 0x1fffffff;
                static const u32 dataInternalMask = 0xE0000000;
                static const u32 dataInternalShift = 29;

                u32     name;               ///< data or tag name
                u32     sizeAndFlags;       ///<
            };

            template<class T, size_t Size, bool isIntegralType>
            struct IntergralType;

            template<class T>
            struct IntergralType<T, 1, true>
            {
                static void Write(DetectorOutputStream& stream, u32 name, const T& data)
                {
                    StreamEntry de;
                    de.name = name;
                    de.sizeAndFlags = (u32)(StreamEntry::INT_DATA_U8) << StreamEntry::dataInternalShift;
                    de.sizeAndFlags |= *reinterpret_cast<const u8*>(&data);
                    stream.WriteBinary(de);
                }
            };
            template<class T>
            struct IntergralType<T, 2, true>
            {
                static void Write(DetectorOutputStream& stream, u32 name, const T& data)
                {
                    StreamEntry de;
                    de.name = name;
                    de.sizeAndFlags = (u32)(StreamEntry::INT_DATA_U16) << StreamEntry::dataInternalShift;
                    de.sizeAndFlags |= *reinterpret_cast<const u16*>(&data);
                    stream.WriteBinary(de);
                }
            };
            template<class T>
            struct IntergralType<T, 4, true>
            {
                static void Write(DetectorOutputStream& stream, u32 name, const T& data)
                {
                    StreamEntry de;
                    de.name = name;
                    const u32* uintData = reinterpret_cast<const u32*>(&data);
                    if (((*uintData) & StreamEntry::dataSizeMask) == *uintData) // check if we can store it internally
                    {
                        de.sizeAndFlags = (u32)(StreamEntry::INT_DATA_U29) << StreamEntry::dataInternalShift;
                        de.sizeAndFlags |= *uintData;
                        stream.WriteBinary(de);
                    }
                    else
                    {
                        de.sizeAndFlags = 4;
                        stream.WriteBinary(de);
                        stream.WriteBinary(&data, de.sizeAndFlags);
                    }
                }
            };
            template<class T>
            struct IntergralType<T, 8, true>
            {
                static void Write(DetectorOutputStream& stream, u32 name, const T& data)
                {
                    StreamEntry de;
                    de.name = name;
                    const u64* uintData = reinterpret_cast<const u64*>(&data);
                    if (((*uintData) & static_cast<u64>(StreamEntry::dataSizeMask)) == *uintData) // check if we can store it internally
                    {
                        de.sizeAndFlags = (u32)(StreamEntry::INT_DATA_U29) << StreamEntry::dataInternalShift;
                        de.sizeAndFlags |= static_cast<u32>(*uintData);
                        stream.WriteBinary(de);
                    }
                    else
                    {
                        de.sizeAndFlags = 8;
                        stream.WriteBinary(de);
                        stream.WriteBinary(&data, de.sizeAndFlags);
                    }
                }
            };
            template<class T>
            struct IntergralType<T*, sizeof(void*), false>
            {
                static void Write(DetectorOutputStream& stream, u32 name, const T* pointer)
                {
                    size_t id = reinterpret_cast<size_t>(pointer);
                    IntergralType<size_t, sizeof(id), true>::Write(stream, name, id);
                }
            };

        public:
            /**
             * Each stream with start with this header, before anything else.
             */
            struct StreamHeader
            {
                StreamHeader()
                    : platform((u8)g_currentPlatform)    {}
                u8   platform;
            };

            DetectorOutputStream(DetectorStringPool* stringPool = NULL)
                : m_stringPool(stringPool) { }
            virtual ~DetectorOutputStream() {}

            //////////////////////////////////////////////////////////////////////////
            // Write
            inline void BeginTag(u32 name)
            {
                StreamEntry de;
                de.name = name;
                de.sizeAndFlags = (u32)(StreamEntry::INT_TAG) << StreamEntry::dataInternalShift;
                de.sizeAndFlags |= 1; // true - open tag
                WriteBinary(de);
            }
            inline void EndTag(u32 name)
            {
                StreamEntry de;
                de.name = name;
                de.sizeAndFlags = (u32)(StreamEntry::INT_TAG) << StreamEntry::dataInternalShift;
                WriteBinary(de);
            }

            //////////////////////////////////////////////////////////////////////////
            // Generic
            template<class T>
            inline void Write(u32 name, const T& data)
            {
                // User should handle non specialized non integral types.
                IntergralType<T, sizeof(T), VStd::is_integral<T>::value || VStd::is_enum<T>::value>::Write(*this, name, data);
            }

            //////////////////////////////////////////////////////////////////////////
            // Binary and strings
            inline void Write(u32 name, const void* data, unsigned int dataSize)
            {
                StreamEntry de;
                de.name = name;
                de.sizeAndFlags = dataSize;
                WriteBinary(de);
                WriteBinary(data, dataSize);
            }
            inline void Write(u32 name, const char* string, bool isCopyString = true)
            {
                StreamEntry de;
                de.name = name;
                de.sizeAndFlags = static_cast<unsigned int>(strlen(string));
                V_Assert(de.sizeAndFlags <= StreamEntry::dataSizeMask, "Invalid string length! String is too long, length is limited to %u bytes!", StreamEntry::dataSizeMask);
                ;
                if (m_stringPool)
                {
                    V::u32 crc;
                    bool isInserted = isCopyString ? m_stringPool->InsertCopy(string, de.sizeAndFlags, crc) : m_stringPool->Insert(string, de.sizeAndFlags, crc);
                    if (!isInserted)  // if already inserted, it means it's in the stream, so store the crc only.
                    {
                        de.sizeAndFlags = (u32)(StreamEntry::INT_POOLED_STRING_CRC32) << StreamEntry::dataInternalShift;
                        de.sizeAndFlags |= sizeof(crc);
                        WriteBinary(de);
                        WriteBinary(&crc, sizeof(crc));
                    }
                    else
                    {
                        V::u32 stringSize = de.sizeAndFlags;
                        de.sizeAndFlags |= (u32)(StreamEntry::INT_POOLED_STRING) << StreamEntry::dataInternalShift;
                        WriteBinary(de);
                        WriteBinary(string, stringSize);
                    }
                }
                else
                {
                    WriteBinary(de);
                    WriteBinary(string, de.sizeAndFlags);
                }
            }
            template<class Allocator>
            inline void Write(u32 name, const VStd::basic_string<VStd::string::value_type, VStd::string::traits_type, Allocator>& str, bool isCopyString = true)
            {
                StreamEntry de;
                de.name = name;
                de.sizeAndFlags = static_cast<V::u32>(str.size());
                V_Assert(de.sizeAndFlags <= StreamEntry::dataSizeMask, "Invalid string length! String is too long, length is limited to %u bytes!", StreamEntry::dataSizeMask);
                if (m_stringPool)
                {
                    V::u32 crc;
                    bool isInserted = isCopyString ? m_stringPool->InsertCopy(str.c_str(), de.sizeAndFlags, crc) : m_stringPool->Insert(str.c_str(), de.sizeAndFlags, crc);
                    if (!isInserted)  // if already inserted, it means it's in the stream, so store the crc only.
                    {
                        de.sizeAndFlags = (u32)(StreamEntry::INT_POOLED_STRING_CRC32) << StreamEntry::dataInternalShift;
                        de.sizeAndFlags |= sizeof(crc);
                        WriteBinary(de);
                        WriteBinary(&crc, sizeof(crc));
                    }
                    else
                    {
                        V::u32 stringSize = de.sizeAndFlags;
                        de.sizeAndFlags |= (u32)(StreamEntry::INT_POOLED_STRING) << StreamEntry::dataInternalShift;
                        WriteBinary(de);
                        WriteBinary(str.data(), stringSize);
                    }
                }
                else
                {
                    WriteBinary(de);
                    WriteBinary(str.data(), de.sizeAndFlags);
                }
            }

            template<class Allocator>
            inline void Write(u32 name, const VStd::basic_string<VStd::wstring::value_type, VStd::wstring::traits_type, Allocator>& str)
            {
                StreamEntry de;
                de.name = name;
                de.sizeAndFlags = static_cast<V::u32>(str.size());
                V_Assert(de.sizeAndFlags <= StreamEntry::dataSizeMask, "Invalid string length! String is too long, length is limited to %u bytes!", StreamEntry::dataSizeMask);
                WriteBinary(de);
                WriteBinary(str.data(), de.sizeAndFlags * sizeof(VStd::wstring::value_type));
            }

            //////////////////////////////////////////////////////////////////////////
            // math types
            inline void Write(u32 name, float f)
            {
                Write(name, &f, static_cast<unsigned int>(sizeof(float)));
            }
            inline void Write(u32 name, double d)
            {
                Write(name, &d, static_cast<unsigned int>(sizeof(double)));
            }

            //////////////////////////////////////////////////////////////////////////
            // containers
            template<class InputIterator>
            inline void Write(u32 name, InputIterator first, InputIterator last)
            {
                // we can specialize for contiguous_iterator_tag so have only 1 write for all elements
                size_t numElements = VStd::distance(first, last);
                size_t elementSize = sizeof(typename VStd::iterator_traits<InputIterator>::value_type);
                unsigned int dataSize = static_cast<unsigned int>(numElements * elementSize);
                StreamEntry de;
                de.name = name;
                de.sizeAndFlags = dataSize;
                V_Assert(dataSize < StreamEntry::dataSizeMask, "Invalid data size, size is limited to %d bytes!", StreamEntry::dataSizeMask - 1);
                WriteBinary(de);
                //WriteBinary(data,dataSize); for contiguous_iterator_tag
                for (; first != last; ++first)
                {
                    WriteBinary(&*first, static_cast<unsigned int>(elementSize));
                }
            }
            //////////////////////////////////////////////////////////////////////////

            //////////////////////////////////////////////////////////////////////////
            // Raw data to the output
            template<class T>
            inline void  WriteBinary(const T& data)
            {
                WriteBinary(&data, sizeof(T));
            }

            virtual void WriteBinary(const void* data, unsigned int dataSize) = 0;
            //////////////////////////////////////////////////////////////////////////

            /**
             * Write a time stamp (VStd::sys_time_t) in millisecond since 1970/01/01 00:00:00 UTC.
             * On older windows this function can have ~15 ms resolution, in such cases use \ref GetTimeNowMicroSecond
             */
            void WriteTimeUTC(u32 name);

            /**
             * Write a time stamp (VStd::sys_time_t) in micriseconds. This function is inaccurate for long periods but it has ms resolution.
             * For long periods use \ref WriteTimeUTC.
             */
            void WriteTimeMicrosecond(u32 name);

            /// Called when the driller is moving on the next frame, so you can flush you current buffer to network/disk.
            virtual void OnEndOfFrame() {}

            /// Sets the string pool used for this stream. To disable the pool just set it to NULL.
            void    SetStringPool(DetectorStringPool* stringPool)    { m_stringPool = stringPool; }

        protected:
            /// Write the Stream header structure (should be endianess independent).
            void WriteHeader();

            DetectorStringPool*  m_stringPool;       ///< Optional pointer to a string pool.
        };

        /**
         * For efficiency all data read functions are placed with the parsers.
         */
        class DetectorInputStream
        {
        public:
            DetectorInputStream(DetectorStringPool* stringPool = NULL)
                : m_isEndianSwap(false)
                , m_stringPool(stringPool) {}
            virtual ~DetectorInputStream() {}

            bool IsEndianSwap() const       { return m_isEndianSwap; }
            /// Reads binary data from a stream to to maxDataSize. Returns 0 if no more data.
            virtual unsigned int ReadBinary(void* data, unsigned int maxDataSize) = 0;

            /// Sets the string pool used for this stream. To disable the pool just set it to NULL.
            void    SetStringPool(DetectorStringPool* stringPool)    { m_stringPool = stringPool; }
            DetectorStringPool* GetStringPool() const                { return m_stringPool; }

            void SetIdentifier(const char* identifier) { m_streamIdentifier = identifier; }
            const char* GetIdentifier() const { return m_streamIdentifier.c_str(); }

        protected:
            /// Read the Stream header structure
            bool ReadHeader();

            bool m_isEndianSwap;
            DetectorStringPool*          m_stringPool;       ///< Optional pointer to a string pool.
            VStd::string               m_streamIdentifier;
        };

        /**
         * Outputs all stream data into a memory buffer. It will grow automatically.
         */
        class DetectorOutputMemoryStream
            : public DetectorOutputStream
        {
        protected:
            vector<unsigned char>::type m_data;
        public:
            V_CLASS_ALLOCATOR(DetectorOutputMemoryStream, OSAllocator, 0)
            DetectorOutputMemoryStream(size_t memorySize = 2048)  { m_data.reserve(memorySize); }
            const unsigned char* GetData() const    { return m_data.data(); }
            unsigned int GetDataSize() const        { return static_cast<unsigned int>(m_data.size()); }
            inline void  Reset()                    { m_data.clear(); }
            void WriteBinary(const void* data, unsigned int dataSize) override
            {
                m_data.insert(m_data.end(), reinterpret_cast<const unsigned char*>(data), reinterpret_cast<const unsigned char*>(data) + dataSize);
            }
        };
        /**
         * Reads data from a memory stream. Data is NOT copied and must be persistent while we are using it.
         */
        class DetectorInputMemoryStream
            : public DetectorInputStream
        {
            const unsigned char* m_data;
            const unsigned char* m_dataEnd;

        public:
            V_CLASS_ALLOCATOR(DetectorInputMemoryStream, OSAllocator, 0)
            DetectorInputMemoryStream(const char* streamIdentifier = "", const void* data = nullptr, unsigned int dataSize = 0)
                : DetectorInputStream()
                , m_data(nullptr)
                , m_dataEnd(nullptr)
            {
                if (data != nullptr)
                {
                    SetData(streamIdentifier, data, dataSize);
                }
            }

            void SetData(const char* streamIdentifier, const void* data, unsigned int dataSize)
            {
                SetIdentifier(streamIdentifier);                

                V_Assert(data != nullptr && dataSize > 0, "We must have a valid pointer %p and data size %d !", data, dataSize);
                if (m_data == nullptr)  // this is the first data chuck, read the platform
                {
                    m_data = reinterpret_cast<const unsigned char*>(data);
                    m_dataEnd = m_data + dataSize;
                    ReadHeader();
                }
                else
                {
                    m_data = reinterpret_cast<const unsigned char*>(data);
                    m_dataEnd = m_data + dataSize;
                }
            }

            unsigned int GetDataLeft() const { return static_cast<unsigned int>(m_dataEnd - m_data); }
            unsigned int ReadBinary(void* data, unsigned int maxDataSize) override
            {
                V_Assert(m_data != nullptr, "You must call SetData function, before you can read data!");
                V_Assert(data != nullptr && maxDataSize > 0, "We must have a valid pointer and max data size!");
                unsigned int dataToCopy = VStd::GetMin(static_cast<unsigned int>(m_dataEnd - m_data), maxDataSize);
                if (dataToCopy)
                {
                    memcpy(data, m_data, dataToCopy);
                }
                m_data += dataToCopy;
                return dataToCopy;
            }
        };

        /**
         * Outputs driller data to a file (buffered)
         * IMPORTANT: We provide direct IO classes (instead trough Streamer), because the driller
         * framework should NOT use engine systems (for example imagine we are drilling the Streamer, using it to
         * write the drilled data will invalidate all the results as the streamer is unaware which data is driller data and which not)
         */
        class DetectorOutputFileStream
            : public IO::SystemFile
            , public DetectorOutputStream
        {
            ZLib* m_zlib;
            vector<unsigned char>::type m_compressionBuffer;
            vector<unsigned char>::type m_dataBuffer;
        public:
            V_CLASS_ALLOCATOR(DetectorOutputFileStream, OSAllocator, 0)
            DetectorOutputFileStream();
            ~DetectorOutputFileStream();
            bool Open(const char* fileName, int mode, int platformFlags = 0);
            void Close();

            void WriteBinary(const void* data, unsigned int dataSize) override;
        };

        /**
         * Reads driller data from a file.
         */
        class DetectorInputFileStream
            : public V::IO::SystemFile
            , public DetectorInputStream
        {
            ZLib* m_zlib;
            vector<unsigned char>::type m_compressedData;
        public:
            V_CLASS_ALLOCATOR(DetectorInputFileStream, OSAllocator, 0)
            DetectorInputFileStream();
            ~DetectorInputFileStream();
            bool Open(const char* fileName, int mode, int platformFlags = 0);
            unsigned int ReadBinary(void* data, unsigned int maxDataSize) override;
            void Close();
        };

        /**
         * SAX like stream parser for driller data. We can stream the data
         * and we will trigger events as tags and data (attributes) arrive. We use less memory this way.
         * \note SAX is used as reference name, we are NOT trying to compatible with
         * any specs. (not that SAX has specs)
         * IMPORTANT: All data callbacks (tag and data) are called in the order they were at store. You can
         * use this order as event index.
         */
        class DetectorSAXParser
        {
        public:
            struct Data
            {
                u32                 m_name;             ///< Crc name of the data entry.
                void*               m_data;             ///< Pointer to copy if the loaded data.
                unsigned int        m_dataSize;         ///< Data size in bytes.
                mutable bool        m_isEndianSwap;     ///< True if the user will need to swap the endian when he access the data. We swap the data is the storage so we can read it multiple times without swap.
                DetectorStringPool*  m_stringPool;       ///< Pointer to optional data string pool.
                bool                m_isPooledString;   ///< True if we have a pooled string (stored in the stringPool already).
                bool                m_isPooledStringCrc32;  ///< True is we have stored a crc32 (4 bytes) which refers to a string from the String Pool.

                //////////////////////////////////////////////////////////////////////////
                // Generic
                template<class T>
                inline void Read(T& t) const
                {
                    static_assert(VStd::is_pod<T>::value, "T must be plain-old-data");

                    V_Assert(sizeof(t) >= m_dataSize, "You are about to lose some data, this is wrong.");
                    if (m_dataSize == sizeof(t))
                    {
                        // do a memcpy as alignment might be required for some data types! This is not performance critical as we usually load drill files on x86/x64
                        // which doesn't care about alignment.
                        memcpy(&t, m_data, m_dataSize);
                    }
                    else
                    {
                        V_Assert(VStd::is_pointer<T>::value || VStd::is_integral<T>::value, "We support extending only for integral types, float and pointers up to 8 bytes!");

                        if (VStd::is_signed<T>::value)
                        {
                            switch (m_dataSize)
                            {
                            case 1:
                                t = static_cast<T>(*reinterpret_cast<s8*>(m_data));
                                break;
                            case 2:
                                t = static_cast<T>(*reinterpret_cast<s16*>(m_data));
                                break;
                            case 4:
                                t = static_cast<T>(*reinterpret_cast<s32*>(m_data));
                                break;
                            default:
                                V_Assert(false, "Source data size unsupported... we can extend only 1,2,4 bytes into 2,4,8 bytes integrals");
                            }
                        }
                        else
                        {
                            switch (m_dataSize)
                            {
                            case 1:
                                t = static_cast<T>(*reinterpret_cast<u8*>(m_data));
                                break;
                            case 2:
                                t = static_cast<T>(*reinterpret_cast<u16*>(m_data));
                                break;
                            case 4:
                                t = static_cast<T>(*reinterpret_cast<u32*>(m_data));
                                break;
                            default:
                                V_Assert(false, "Source data size unsupported... we can extend only 1,2,4 bytes into 2,4,8 bytes integrals");
                            }
                        }
                    }
                    if (m_isEndianSwap)
                    {
                        VStd::endian_swap(t);
                    }
                }

                inline void Read(bool& b) const
                {
                    u8* data = reinterpret_cast<u8*>(m_data);
                    b = false;
                    for (unsigned int i = 0; i < m_dataSize; ++i)
                    {
                        if (data[i] != 0)
                        {
                            b = true;
                            return;
                        }
                    }
                }
                //////////////////////////////////////////////////////////////////////////
                // Binary and strings
                inline unsigned int Read(void* buffer, unsigned int bufferSize) const
                {
                    unsigned int dataToCopy = VStd::GetMin(m_dataSize, bufferSize);
                    memcpy(buffer, m_data, dataToCopy);
                    // no data swap
                    return dataToCopy;
                }
                // a call avilable only when we use a string pool, it will return the pointer of string in the pool, so you don't need to copy it or do any fancy procedures.
                inline const char* ReadPooledString() const
                {
                    V_Assert(m_stringPool != nullptr, "This read type is supported only when we use string pool!");
                    unsigned int srcDataSize;
                    return PrepareString(srcDataSize);
                }
                inline unsigned int Read(char* string, unsigned int maxNumChars) const
                {
                    unsigned int srcDataSize;
                    const char* srcData = PrepareString(srcDataSize);
                    unsigned int dataToCopy = VStd::GetMin(maxNumChars - 1, srcDataSize);
                    memcpy(string, srcData, dataToCopy);
                    string[dataToCopy] = '\0';
                    return dataToCopy;
                }
                template<class Allocator>
                inline unsigned int Read(VStd::basic_string<VStd::string::value_type, VStd::string::traits_type, Allocator>& str) const
                {
                    unsigned int srcDataSize;
                    const char* srcData = PrepareString(srcDataSize);
                    str = VStd::basic_string<VStd::string::value_type, VStd::string::traits_type, Allocator>(static_cast<const VStd::string::value_type*>(srcData), srcDataSize);
                    return m_dataSize;
                }
                template<class Allocator>
                inline unsigned int Read(VStd::basic_string<VStd::wstring::value_type, VStd::wstring::traits_type, Allocator>& str) const
                {
                    // wstring pooling not supported yet
                    str = VStd::basic_string<VStd::wstring::value_type, VStd::wstring::traits_type, Allocator>(static_cast<const VStd::wstring::value_type*>(m_data), m_dataSize / 2);
                    if (m_isEndianSwap)
                    {
                        VStd::endian_swap(str.begin(), str.end());
                    }
                    return m_dataSize;
                }

                //////////////////////////////////////////////////////////////////////////
                // other types

                //////////////////////////////////////////////////////////////////////////
                // containers
                template<class Container>
                inline void Read(VStd::insert_iterator<Container>& iter) const
                {
                    typedef typename VStd::insert_iterator<Container> InsertIterator;
                    // we can specialize for contiguous_iterator_tag so have only 1 write for all elements
                    const size_t elementSize = sizeof(InsertIterator::container_type::value_type);
                    size_t numElements = m_dataSize / elementSize;
                    V_Assert(m_dataSize % elementSize == 0, "Stored elements size doesn't match the read parameters!");
                    Data elementEntry = *this;
                    elementEntry.m_dataSize = elementSize;
                    char* dataPtr = reinterpret_cast<char*>(m_data);
                    for (size_t i = 0; i < numElements; ++i, ++iter)
                    {
                        typename InsertIterator::container_type::value_type value;
                        elementEntry.m_data = dataPtr;
                        Read(elementEntry, value);
                        iter = value;
                        dataPtr += elementSize;
                    }
                }
                //////////////////////////////////////////////////////////////////////////
            private:
                const char*  PrepareString(unsigned int& stringLength) const;
            };

            typedef VStd::delegate<void (u32 /*name*/, bool /*isOpen*/)> TagCallbackType;
            typedef VStd::delegate<void (const Data&)> DataCallbackType;

            V_CLASS_ALLOCATOR(DetectorSAXParser, OSAllocator, 0)
            DetectorSAXParser(const TagCallbackType& tcb, const DataCallbackType& dcb);
            /// Processes an input stream until all data is consumed (read returns 0 bytes).
            void ProcessStream(DetectorInputStream& stream);
        protected:

            typedef vector<char>::type  BufferType;
            BufferType                  m_buffer;
            TagCallbackType             m_tagCallback;
            DataCallbackType            m_dataCallback;
        };

        /**
        * DOM like parser, we will load the entire stream in memory (ProcessStream function).
        * Depending on the data size this can be very memory consuming.
        * \note DOM is used as reference we are NOT compliant with the DOM specs in any way.
        * IMPORTANT: All data is stored (for parsing) in the same order the events occurred
        * or the remote machine. Each next tad or data was recorded in the way. You can use
        * this as an event index.
        */
        class DetectorDOMParser
            : public DetectorSAXParser
        {
        public:
            struct Node
            {
                typedef forward_list<Data>::type    DataListType;
                typedef forward_list<Node>::type    NodeListType;

                u32             m_name;
                Node*           m_parent;
                DataListType    m_data;
                NodeListType    m_tags;

                /// Return a pointer to the first tag with specific name.
                const Node* GetTag(u32 tagName) const;
                /// Returns pointer to the first data entry with specific name. NULL if not data has been found.
                const Data* GetData(u32 dataName) const;
                /// Returns pointer to the first data entry with specific name. If it can't be found it will assert
                const Data* GetDataRequired(u32 dataName) const
                {
                    const Data* dataNode = GetData(dataName);
                    V_Assert(dataNode != NULL, "Data node in tag 0x%08x with name 0x%08x is required but missing!", m_name, dataName);
                    return dataNode;
                }
            };

            V_CLASS_ALLOCATOR(DetectorDOMParser, OSAllocator, 0)

            DetectorDOMParser(bool isPersistentInputData = false);
            ~DetectorDOMParser();
            /// return true if we are at top level of the tree and we can parse the data safely (there may be still more data, but it's top level only).
            bool  CanParse() const          { return m_topNode == &m_root; }

            const Node* GetRootNode() const { return &m_root; }
        protected:
            Node m_root;
            Node* m_topNode;
            bool m_isPersistentInputData;   ///< true if data that we process is persistent so we don't need to copy it internally, false otherwise.

            void OnTag(u32 name, bool isOpen);
            void OnData(const Data& data);
            void DeleteNode(Node& node);
        };

        /**
         * Base class for handling a Tag with a specific name. Handlers are kept in a hierarchy
         * with one required by DetectorSAXParserHandler to be able to handle tags at a root
         * level for the driller data stream.
         */
        class DetectorHandlerParser
        {
        public:
            DetectorHandlerParser(bool isWarnOnUnsupportedTags = true)
                : m_isWarnOnUnsupportedTags(isWarnOnUnsupportedTags) {}

            virtual ~DetectorHandlerParser() {}
            /// Enumerate all the child tags that we support for the tag we are handling. If the tag is not know you should return NULL
            virtual DetectorHandlerParser*   OnEnterTag(u32 tagName)                                         { (void)tagName; return NULL; }
            /// Exit tag you are not required to implement this, we always exist tags in order FILO.
            virtual void                    OnExitTag(DetectorHandlerParser* handler, u32 tagName)           { (void)handler; (void)tagName; }
            /// Handle that data for the tag we are handling.
            virtual void                    OnData(const DetectorSAXParser::Data& dataNode)                  { (void)dataNode; }
            /// Return the warning state on unsupported tags (sometime you might want to warn usually) and sometimes not (if you load newer drills, etc.)
            inline bool                     IsWarnOnUnsupportedTags() const                                 { return m_isWarnOnUnsupportedTags; }

        protected:
            bool                            m_isWarnOnUnsupportedTags;
        };

        /**
         * Processes a driller driller and dispatches the data based on the
         * the DetectorHandlerParser (handlers) and their ability to handle specific tags.
         * If a tag is NOT found as a child of the current one it will display a warning with the tag name
         * (useless it's allowed by DetectorHandlerParser::IsWarnOnUnsupportedTags) and process the stream
         * is a safe manner by skipping all the data and tags we can't handle.
         */
        class DetectorSAXParserHandler
            : public DetectorSAXParser
        {
        public:
            V_CLASS_ALLOCATOR(DetectorSAXParserHandler, OSAllocator, 0)

            DetectorSAXParserHandler(DetectorHandlerParser* rootHandler);

        protected:

            /// Called from DetectorSAXParser when we have an open tag.
            void OnTag(u32 name, bool isOpen);
            /// Called from DetectorSAXParser when we have data, which will be forwarded to the handler.
            void OnData(const DetectorSAXParser::Data& data);

            typedef vector<DetectorHandlerParser*>::type DetectorHandlerStackType;
            DetectorHandlerStackType     m_stack;
        };
    } // namespace Debug
} // namespace V

#endif //V_FRAMEWORK_CORE_DETECTOR_STREAM_H