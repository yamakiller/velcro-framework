#include <vcore/std/algorithm.h>
#include <vcore/io/compressor.h>
#include <vcore/io/compressor_stream.h>

namespace V
{
    namespace IO
    {
        //=========================================================================
        // WriteHeaderAndData
        //=========================================================================
        bool Compressor::WriteHeaderAndData(CompressorStream* compressorStream)
        {
            V_Assert(compressorStream->CanWrite(), "Stream is not open for write!");
            V_Assert(compressorStream->GetCompressorData(), "Stream doesn't have attached compressor, call WriteCompressed first!");
            V_Assert(compressorStream->GetCompressorData()->CompressorHandle == this, "Invalid compressor data! Data belongs to a different compressor");
            CompressorHeader header;
            header.SetVCS();
            header.CompressorId = GetTypeId();
            header.UncompressedSize = compressorStream->GetCompressorData()->UncompressedSize;
            VStd::endian_swap(header.CompressorId);
            VStd::endian_swap(header.UncompressedSize);
            GenericStream* baseStream = compressorStream->GetWrappedStream();
            if (baseStream->WriteAtOffset(sizeof(CompressorHeader), &header, 0U) == sizeof(CompressorHeader))
            {
                return true;
            }

            return false;
        }
    } // namespace IO
} // namespace V
