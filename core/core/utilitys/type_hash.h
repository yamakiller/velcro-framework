#ifndef V_FRAMEWORK_CORE_UTILITYS_TYPE_HASH_H
#define V_FRAMEWORK_CORE_UTILITYS_TYPE_HASH_H

#include <core/base.h>
#include <string>

namespace V {
    typedef u32 HashValue32;
    typedef u64 HashValue64;

    /// @brief 对从 buffer 开始到 buffer + length 结束的连续字节数组进行哈希处理.
    /// @param buffer[in] 指向要散列的内存的缓冲区指针
    /// @param length[in] 用于生成哈希值的内存长度(以字节为单位)
    /// @return 32 位结果哈希
    HashValue32 TypeHash32(const uint8_t* buffer, uint64_t length);

    /// @brief 对从 buffer 开始到 buffer + length 结束的连续字节数组进行哈希处理.
    /// @param buffer[in] 指向要散列的内存的缓冲区指针
    /// @param length[in] 用于生成哈希值的内存长度(以字节为单位)
    /// @return 64 位结果哈希
    HashValue64 TypeHash64(const uint8_t* buffer, uint64_t length);

    /// @brief 对从 buffer 开始到 buffer + length 结束的连续字节数组进行哈希处理.
    /// @param buffer[in] 指向要散列的内存的缓冲区指针
    /// @param length[in] 用于生成哈希值的内存长度(以字节为单位)
    /// @param seed 散列到结果中的种子
    /// @return 64 位结果哈希
    HashValue64 TypeHash64(const uint8_t* buffer, uint64_t length, HashValue64 seed);

    /// @brief 对从 buffer 开始到 buffer + length 结束的连续字节数组进行哈希处理.
    /// @param buffer[in] 指向要散列的内存的缓冲区指针
    /// @param length[in] 用于生成哈希值的内存长度(以字节为单位)
    /// @param seed1 散列到结果中的第一个种子
    /// @param seed2 散列到结果中的第二个种子
    /// @return 64 位结果哈希
    HashValue64 TypeHash64(const uint8_t* buffer, uint64_t length, HashValue64 seed1, HashValue64 seed2);

     template <typename T>
    HashValue32 TypeHash32(const T& t) {
        return TypeHash32(reinterpret_cast<const uint8_t*>(&t), sizeof(T));
    }

    template <typename T>
    HashValue64 TypeHash64(const T& t) {
        return TypeHash64(reinterpret_cast<const uint8_t*>(&t), sizeof(T));
    }

    template <typename T>
    HashValue64 TypeHash64(const T& t, HashValue64 seed) {
        return TypeHash64(reinterpret_cast<const uint8_t*>(&t), sizeof(T), seed);
    }

    template <typename T>
    HashValue64 TypeHash64(const T& t, HashValue64 seed1, HashValue64 seed2) {
        return TypeHash64(reinterpret_cast<const uint8_t*>(&t), sizeof(T), seed1, seed2);
    }

    inline HashValue32 TypeHash32(const char* value) {
        return TypeHash32(reinterpret_cast<const uint8_t*>(value), strlen(value));
    }

    inline HashValue64 TypeHash64(const char* value) {
        return TypeHash64(reinterpret_cast<const uint8_t*>(value), strlen(value));
    }

    inline HashValue64 TypeHash64(const char* value, HashValue64 seed) {
        return TypeHash64(reinterpret_cast<const uint8_t*>(value), strlen(value), seed);
    }

    inline HashValue64 TypeHash64(const char* value, HashValue64 seed1, HashValue64 seed2) {
        return TypeHash64(reinterpret_cast<const uint8_t*>(value), strlen(value), seed1, seed2);
    }
}

#endif // V_FRAMEWORK_CORE_UTILITYS_TYPE_HASH_H