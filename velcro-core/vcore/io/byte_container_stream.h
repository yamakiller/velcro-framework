#ifndef V_FRAMEWORK_CORE_IO_BYTE_CONTAINER_STREAM_H
#define V_FRAMEWORK_CORE_IO_BYTE_CONTAINER_STREAM_H

#include <vcore/io/generic_streams.h>
#include <vcore/math/math_utils.h>
#include <vcore/std/typetraits/is_const.h>
#include <vcore/std/typetraits/has_member_function.h>
#include <vcore/casting/numeric_cast.h>

namespace V
{
    namespace IO
    {
        /*
         * ByteContainerStream
         * A stream that wraps around a sequence container
         * Currently only VStd::vector and VStd::fixed_vector are supported as
         * we use direct memcpy, so underlying storage has to be guaranteed continuous.
         * It can be generalized if necessary, but for now this should do.
         */
        template<bool V>
        struct WritableStreamType  {};
        template<>
        struct WritableStreamType<true>
        {
            typedef int type;
        };

        template<typename ContainerType>
        class ByteContainerStream
            : public GenericStream
        {
        public:
            ByteContainerStream(ContainerType* buffer, size_t maxGrowSize = 5 * 1024 * 1024);
            ~ByteContainerStream() override {}

            bool        IsOpen() const override                              { return true; }
            bool        CanSeek() const override                             { return true; }
            bool        CanRead() const override                    { return true; }
            bool        CanWrite() const override                   { return true; }
            SizeType    GetCurPos() const override                           { return m_pos; }
            SizeType    GetLength() const override                           { return m_buffer->size(); }
            void        Seek(OffsetType bytes, SeekMode mode) override;
            SizeType    Read(SizeType bytes, void* oBuffer) override;
            SizeType    Write(SizeType bytes, const void* iBuffer) override;
            SizeType    WriteFromStream(SizeType bytes, GenericStream* inputStream) override;
            template<typename T>
            inline SizeType Write(const T* iBuffer)
            {
                return Write(sizeof(T), iBuffer);
            }

            // Truncate the stream to the current position.
            void                Truncate();

            ContainerType*      GetData() const                             { return m_buffer; }

        protected:
            SizeType PrepareToWrite(SizeType bytes);
            template<typename T>
            SizeType Write(SizeType bytes, const void* iBuffer, typename T::type);
            template<typename T>
            SizeType WriteFromStream(SizeType bytes, GenericStream* inputStream, typename T::type);
            template<typename T>
            SizeType Write(SizeType, const void*, unsigned int);
            template<typename T>
            SizeType WriteFromStream(SizeType, GenericStream*, unsigned int);

            ContainerType*  m_buffer;
            size_t          m_pos;
            size_t          m_maxGrowSize; //< Maximum grow size to avoid the buffer growing too fast when it's working on big files
        };

        namespace Internal
        {
            V_HAS_MEMBER(ResizeNoConstruct, resize_no_construct, void, (size_t));

            // As we are about to write bytes to a container, use the fast resize (without constructing elements) if available.
            template<class ContainerType>
            inline void ResizeContainerBuffer(ContainerType* buffer, size_t newLength, const VStd::true_type& /* HasResizeNoConstruct<ContainerType> */)
            {
                buffer->resize_no_construct(newLength);
            }

            // If resize_no_construct if not supported by the container, just call resize.
            template<class ContainerType>
            inline void ResizeContainerBuffer(ContainerType* buffer, size_t newLength, const VStd::false_type& /* HasResizeNoConstruct<ContainerType> */)
            {
                buffer->resize(newLength);
            }

            // Set the container capacity to manage better number of allocations (especially when doing small allocation)
            V_HAS_MEMBER(SetCapacity, set_capacity, void, (size_t));

            template<class ContainerType>
            inline void AdjustContainerBufferCapacity(ContainerType* buffer, size_t requiredSize, size_t maxCapacityGrowSize, const VStd::true_type& /* HasSetCapacity<ContainerType> */)
            {
                if (requiredSize > buffer->capacity())
                {
                    // grow by 50%, if we can.
                    size_t newCapacityGrowSize = AZ::GetClamp<size_t>(requiredSize / 2, 4, maxCapacityGrowSize);
                    size_t newCapacity = requiredSize + newCapacityGrowSize;
                    buffer->set_capacity(newCapacity);
                }
            }

            template<class ContainerType>
            inline void AdjustContainerBufferCapacity(ContainerType* buffer, size_t requiredSize, size_t maxCapacityGrowSize, const VStd::false_type& /* HasSetCapacityt<ContainerType> */)
            {
                (void)buffer; (void)requiredSize; (void)maxCapacityGrowSize;
                // no set capacity support we will need to rely on resize alone
            }
        }

        template<typename ContainerType>
        ByteContainerStream<ContainerType>::ByteContainerStream(ContainerType* buffer, size_t maxGrowSize)
            : m_buffer(buffer)
            , m_pos(0)
            , m_maxGrowSize(maxGrowSize)
        {
            static_assert(sizeof(typename ContainerType::value_type) == 1, "The buffer is not a container of bytes!");
            V_Assert(m_buffer, "You cannot make a ByteContainerStream with buffer = null!");
        }

        template<typename ContainerType>
        void ByteContainerStream<ContainerType>::Seek(OffsetType bytes, SeekMode mode)
        {
            m_pos = static_cast<size_t>(ComputeSeekPosition(bytes, mode));
        }

        template<typename ContainerType>
        SizeType ByteContainerStream<ContainerType>::Read(SizeType bytes, void* oBuffer)
        {
            size_t len = m_buffer->size();
            size_t bytesToRead = 0;
            if (m_pos + static_cast<size_t>(bytes) < len)
            {
                bytesToRead = static_cast<size_t>(bytes);
            }
            else if (len > m_pos)
            {
                bytesToRead = len - m_pos;
            }
            if (bytesToRead > 0)
            {
                V_Assert(oBuffer, "Output buffer can't be null!");
                memcpy(oBuffer, m_buffer->data() + m_pos, bytesToRead);
                m_pos += bytesToRead;
            }
            return bytesToRead;
        }

        template<typename ContainerType>
        SizeType ByteContainerStream<ContainerType>::Write(SizeType bytes, const void* iBuffer)
        {
            return Write<WritableStreamType<!VStd::is_const<ContainerType>::value> >(bytes, iBuffer, 0);
        }

        template<typename ContainerType>
        SizeType ByteContainerStream<ContainerType>::WriteFromStream(SizeType bytes, GenericStream* inputStream)
        {
            return WriteFromStream<WritableStreamType<!VStd::is_const<ContainerType>::value> >(bytes, inputStream, 0);
        }

        template<typename ContainerType>
        SizeType ByteContainerStream<ContainerType>::PrepareToWrite(SizeType bytes)
        {
            size_t len = m_buffer->size();
            size_t requiredLen = static_cast<size_t>(m_pos + bytes);

            if (requiredLen > len)
            {
                // Make sure we grow the write buffer in a reasonable manner to avoid too many allocations
                Internal::AdjustContainerBufferCapacity(m_buffer, requiredLen, m_maxGrowSize, typename Internal::HasSetCapacity<ContainerType>::type());

                Internal::ResizeContainerBuffer(m_buffer, requiredLen, typename Internal::HasResizeNoConstruct<ContainerType>::type());
            }

            // For now, just return back the value that was handed in.  This could also be used to return 0 if error checking gets added
            // to the buffer adjustments above.
            return bytes;
        }

        template<typename ContainerType>
        template<typename T>
        SizeType ByteContainerStream<ContainerType>::Write(SizeType bytes, const void* iBuffer, typename T::type)
        {
            size_t bytesToCopy = aznumeric_cast<size_t>(PrepareToWrite(bytes));
            memcpy(m_buffer->data() + m_pos, iBuffer, bytesToCopy);
            m_pos += bytesToCopy;
            return bytes;
        }

        template<typename ContainerType>
        template<typename T>
        SizeType ByteContainerStream<ContainerType>::WriteFromStream(SizeType bytes, GenericStream* inputStream, typename T::type)
        {
            V_Assert(inputStream, "Input stream is null!");
            V_Assert(inputStream != this, "Can't write and read from the same stream.");
            size_t bytesToCopy = aznumeric_cast<size_t>(PrepareToWrite(bytes));
            bytesToCopy = inputStream->Read(bytesToCopy, m_buffer->data() + m_pos);
            m_pos += bytesToCopy;
            return bytesToCopy;
        }

        template<typename ContainerType>
        template<typename T>
        SizeType ByteContainerStream<ContainerType>::Write(SizeType, const void*, unsigned int)
        {
            V_Assert(false, "This stream is read-only!");
            return 0;
        }

        template<typename ContainerType>
        template<typename T>
        SizeType ByteContainerStream<ContainerType>::WriteFromStream(SizeType, GenericStream*, unsigned int)
        {
            V_Assert(false, "This stream is read-only!");
            return 0;
        }


        template<typename ContainerType>
        void ByteContainerStream<ContainerType>::Truncate()
        {
            Internal::ResizeContainerBuffer(m_buffer, m_pos, typename Internal::HasResizeNoConstruct<ContainerType>::type());
        }
    }   // IO
}   // namespace V

#endif // V_FRAMEWORK_CORE_IO_BYTE_CONTAINER_STREAM_H