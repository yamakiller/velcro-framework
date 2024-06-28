#include <vcore/memory/memory.h>

#if defined(V_GLOBAL_NEW_AND_DELETE_DEFINED)
#pragma error("new_and_delete.inl has been included multiple times in a single module")
#endif

#define V_GLOBAL_NEW_AND_DELETE_DEFINED
[[nodiscard]] void* operator new(std::size_t size, const V::Internal::AllocatorDummy*) { return V::OperatorNew(size); }
[[nodiscard]] void* operator new[](std::size_t size, const V::Internal::AllocatorDummy*) { return V::OperatorNewArray(size); }
[[nodiscard]] void* operator new(std::size_t size, const char* fileName, int lineNum, const char* name, const V::Internal::AllocatorDummy*) { return V::OperatorNew(size, fileName, lineNum, name); }
[[nodiscard]] void* operator new[](std::size_t size, const char* fileName, int lineNum, const char* name, const V::Internal::AllocatorDummy*) { return V::OperatorNewArray(size, fileName, lineNum, name); }
[[nodiscard]] void* operator new(std::size_t size) { return V::OperatorNew(size); }
[[nodiscard]] void* operator new[](std::size_t size) { return V::OperatorNewArray(size); }
[[nodiscard]] void* operator new(size_t size, const std::nothrow_t&) noexcept { return V::OperatorNew(size); }
[[nodiscard]] void* operator new[](size_t size, const std::nothrow_t&) noexcept { return V::OperatorNewArray(size); }
void operator delete(void* ptr) noexcept { V::OperatorDelete(ptr); }
void operator delete[](void* ptr) noexcept{ V::OperatorDeleteArray(ptr); }
void operator delete(void* ptr, std::size_t size) noexcept { V::OperatorDelete(ptr, size); }
void operator delete[](void* ptr, std::size_t size) noexcept { V::OperatorDeleteArray(ptr, size); }
void operator delete(void* ptr, const std::nothrow_t&) noexcept { V::OperatorDelete(ptr); }
void operator delete[](void* ptr, const std::nothrow_t&) noexcept { V::OperatorDeleteArray(ptr); }
void operator delete(void* ptr, [[maybe_unused]] const char* fileName, [[maybe_unused]] int lineNum, [[maybe_unused]] const char* name) noexcept { V::OperatorDelete(ptr); }
void operator delete[](void* ptr, [[maybe_unused]] const char* fileName, [[maybe_unused]] int lineNum, [[maybe_unused]] const char* name) noexcept { V::OperatorDeleteArray(ptr); }

#if __cpp_aligned_new
// C++17 provides allocating operator new overloads for over aligned types
//an http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2016/p0035r4.html
[[nodiscard]] void* operator new(std::size_t size, std::align_val_t align) { return V::OperatorNew(size, align); }
[[nodiscard]] void* operator new(std::size_t size, std::align_val_t align, const std::nothrow_t&) noexcept { return V::OperatorNew(size, align); }
[[nodiscard]] void* operator new(std::size_t size, std::align_val_t align, const char* fileName, int lineNum, const char* name, const V::Internal::AllocatorDummy*) { return V::OperatorNew(size, align, fileName, lineNum, name); }
[[nodiscard]] void* operator new[](std::size_t size, std::align_val_t align) { return V::OperatorNewArray(size, align); }
[[nodiscard]] void* operator new[](std::size_t size, std::align_val_t align, const std::nothrow_t&) noexcept { return V::OperatorNewArray(size, align); }
[[nodiscard]] void* operator new[](std::size_t size, std::align_val_t align, const char* fileName, int lineNum, const char* name) { return V::OperatorNewArray(size, align, fileName, lineNum, name); }
void operator delete(void* ptr, std::align_val_t align) noexcept { V::OperatorDelete(ptr, 0, align); }
void operator delete(void* ptr, std::size_t size, std::align_val_t align) noexcept { V::OperatorDelete(ptr, size, align); }
void operator delete(void* ptr, std::align_val_t align, const std::nothrow_t&) noexcept { V::OperatorDelete(ptr, 0, align); }
void operator delete(void* ptr, std::align_val_t align, [[maybe_unused]] const char* fileName, [[maybe_unused]] int lineNum, [[maybe_unused]] const char* name) noexcept { V::OperatorDelete(ptr, 0, align); }
void operator delete[](void* ptr, std::align_val_t align) noexcept { V::OperatorDeleteArray(ptr, 0, align); }
void operator delete[](void* ptr, std::size_t size, std::align_val_t align) noexcept { V::OperatorDeleteArray(ptr, size, align); }
void operator delete[](void* ptr, std::align_val_t align, const std::nothrow_t&) noexcept { V::OperatorDeleteArray(ptr, 0, align); }
void operator delete[](void* ptr, std::align_val_t align, [[maybe_unused]] const char* fileName, [[maybe_unused]] int lineNum, [[maybe_unused]] const char* name) noexcept { V::OperatorDeleteArray(ptr, 0, align); }
#endif
