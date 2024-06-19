#if !defined(VCORE_EXCLUDE_ZSTANDARD)

#include <vcore/memory/system_allocator.h>
#include <vcore/casting/lossy_cast.h>
#include <limits>

#include <vcore/compression/zstd_compression.h>

using namespace V;

ZStd::ZStd(IAllocatorAllocate* workMemAllocator) {
    m_workMemoryAllocator = workMemAllocator;
    if (!m_workMemoryAllocator) {
        m_workMemoryAllocator = &AllocatorInstance<SystemAllocator>::Get();
    }
    m_streamCompression = nullptr;
    m_streamDecompression = nullptr;
}

ZStd::~ZStd() {
    if (m_streamCompression) {
        StopCompressor();
    }
    if (m_streamDecompression) {
        StopDecompressor();
    }
}

void* ZStd::AllocateMem(void* userData, size_t size) {
    IAllocatorAllocate* allocator = reinterpret_cast<IAllocatorAllocate*>(userData);
    return allocator->Allocate(size, 4, 0, "ZStandard", __FILE__, __LINE__);
}

void ZStd::FreeMem(void* userData, void* address) {
    IAllocatorAllocate* allocator = reinterpret_cast<IAllocatorAllocate*>(userData);
    allocator->DeAllocate(address);
}

void ZStd::StartCompressor(unsigned int compressionLevel) {
    V_Assert(!m_streamCompression, "Compressor already started!");
    ZSTD_customMem customAlloc;
    customAlloc.customAlloc = reinterpret_cast<ZSTD_allocFunction>(&AllocateMem);
    customAlloc.customFree = &FreeMem;

    V_UNUSED(compressionLevel);
    m_streamCompression = (ZSTD_createCStream_advanced(customAlloc));
    V_Assert( m_streamCompression , "ZStandard internal error - failed to create compression stream\n");
}

void ZStd::StopCompressor() {
    V_Assert(m_streamCompression, "Compressor not started!");
    ZSTD_endStream(m_streamCompression, nullptr);
    FreeMem(m_workMemoryAllocator, m_streamCompression);
    m_streamCompression = nullptr;
}

void ZStd::ResetCompressor() {
    V_Assert(m_streamCompression, "Compressor not started!");
    size_t r = ZSTD_resetCStream(m_streamCompression,0);
    V_UNUSED(r);
    V_Assert(!ZSTD_isError(r), "Can't reset compressor");
}

unsigned int ZStd::Compress(const void* data, unsigned int& dataSize, void* compressedData, unsigned int compressedDataSize, FlushType flushType)
{
    V_UNUSED(data);
    V_UNUSED(dataSize);
    V_UNUSED(compressedData);
    V_UNUSED(compressedDataSize);
    V_UNUSED(flushType);
    return 0;
}

unsigned int ZStd::GetMinCompressedBufferSize(unsigned int sourceDataSize) {
    return vlossy_cast<unsigned int>(ZSTD_compressBound(sourceDataSize));
}

void ZStd::StartDecompressor() {
    V_Assert(!m_streamDecompression, "Decompressor already started!");
    ZSTD_customMem customAlloc;
    customAlloc.customAlloc = reinterpret_cast<ZSTD_allocFunction>(&ZStd::AllocateMem);
    customAlloc.customFree = &ZStd::FreeMem;
    customAlloc.opaque = m_workMemoryAllocator;

    m_streamDecompression = ZSTD_createDStream_advanced(customAlloc);
    m_nextBlockSize = ZSTD_initDStream(m_streamDecompression);
    V_Assert(!ZSTD_isError(m_nextBlockSize), "ZStandard internal error: %s", ZSTD_getErrorName(m_nextBlockSize));

    //pointers will be set later....
    m_inBuffer.pos = 0;
    m_inBuffer.size = 0;
    m_outBuffer.size = 0;
    m_outBuffer.pos = 0;
    m_compressedBufferIndex = 0;
}

void ZStd::StopDecompressor() {
    V_Assert(m_streamDecompression, "Decompressor not started!");
    size_t result = ZSTD_freeDStream(m_streamDecompression);
    V_Verify(!ZSTD_isError(result), "ZStandard internal error: %s", ZSTD_getErrorName(result));
    m_streamDecompression = nullptr;
}

void ZStd::ResetDecompressor(Header* header) {
    V_UNUSED(header);
}

void ZStd::SetupDecompressHeader(Header header) {
    V_UNUSED(header);
}

unsigned int ZStd::Decompress(const void* compressedData, unsigned int compressedDataSize, void* outputData, unsigned int outputDataSize, size_t* sizeOfNextBlock)
{
    V_Assert(m_streamDecompression, "Decompressor not started!");
    V_UNUSED(compressedDataSize);
    const char* data = reinterpret_cast<const char*>(compressedData);
    m_inBuffer.src = reinterpret_cast<const void*>(&data[m_compressedBufferIndex]);
    m_inBuffer.size = m_nextBlockSize;
    m_inBuffer.pos = 0;

    m_outBuffer.dst = outputData;
    m_outBuffer.size = outputDataSize;
    m_outBuffer.pos = 0;

    m_nextBlockSize = ZSTD_decompressStream(m_streamDecompression, &m_outBuffer, &m_inBuffer);
    if (ZSTD_isError(m_nextBlockSize)) {
        V_Assert(false, "ZStd streaming decompression error: %s", ZSTD_getErrorName(m_nextBlockSize));
        return std::numeric_limits<unsigned int>::max();
    }

    if (m_nextBlockSize == 0) {
        m_nextBlockSize = ZSTD_resetDStream(m_streamDecompression);
    }

    m_compressedBufferIndex += vlossy_cast<unsigned int>(m_inBuffer.pos);

    *sizeOfNextBlock = m_nextBlockSize;

    return vlossy_cast<unsigned int>(m_outBuffer.pos); //return number of bytes decompressed
}

bool ZStd::IsCompressorStarted() const {
    return m_streamCompression != nullptr;
}

bool ZStd::IsDecompressorStarted() const {
    return m_streamDecompression != nullptr;
}

#endif // !defined(VCORE_EXCLUDE_ZSTANDARD)