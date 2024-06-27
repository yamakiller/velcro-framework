#include <vcore/math/crc.h>


#include <string.h>

namespace V {
    //=========================================================================
    //
    // Crc32 constructor
    //
    //=========================================================================
    Crc32::Crc32(const void* data, size_t size, bool forceLowerCase)
        : Crc32{ reinterpret_cast<const uint8_t*>(data), size, forceLowerCase } {
    }

    void Crc32::Set(const void* data, size_t size, bool forceLowerCase) {
        Internal::Crc32Set(reinterpret_cast<const uint8_t*>(data), size, forceLowerCase, m_value);
    }

    //=========================================================================
    // Crc32 - Add
    //=========================================================================
    void Crc32::Add(const void* data, size_t size, bool forceLowerCase) {
        Combine(Crc32{ data, size, forceLowerCase }, size);
    }
}