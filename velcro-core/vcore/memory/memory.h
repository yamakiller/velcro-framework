/*
 * Copyright (c) Contributors to the VelcroFramework.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 * 
 */
#ifndef V_FRAMEWORK_CORE_MEMORY_MEMMORY_H
#define V_FRAMEWORK_CORE_MEMORY_MEMMORY_H

#include <vcore/base.h>
#include <vcore/memory/allocator_wrapper.h>
#include <vcore/memory/Config.h>
#include <vcore/memory/simple_schema_allocator.h>
#include <vcore/std/typetraits/alignment_of.h>
#include <vcore/std/typetraits/aligned_storage.h>

#include <vcore/std/typetraits/has_member_function.h>
#include <vcore/module/environment.h>

// 新重载的声明、定义位于 NewAndDelete.inl 中，或者您可以链接您自己的定义的版本
// V::Internal::AllocatorDummy is merely to differentiate our overloads from any other operator new signatures
void* operator new(std::size_t, const V::Internal::AllocatorDummy*);
void* operator new[](std::size_t, const V::Internal::AllocatorDummy*);

/**
 * Velcro 内存分配支持所有最知名的分配方案. 尽管我们强烈建议使用我们为其提供的 ClassAllocators 的 new/delete 运算符的类覆盖.
 * 我们不限制使用您需要的任何方式, 每种方式都有其优点和缺点. 我们将逐一进行描述. 在每个不需要指定分配器的宏中都隐含了 
 * V::SystemAllocator.
 */
#if !defined(_RELEASE)
    #define vnew                                                   vnewex((const char*)nullptr)
    #define vnewex(_Name)                                          new(__FILE__, __LINE__, _Name)
    /// vmalloc(size)
    #define vmalloc_1(_1)                                          V::AllocatorInstance< V::SystemAllocator >::Get().Allocate(_1, 0, 0, "vmalloc", __FILE__, __LINE__)
    /// vmalloc(size,alignment)
    #define vmalloc_2(_1, _2)                                      V::AllocatorInstance< V::SystemAllocator >::Get().Allocate(_1, _2, 0, "vmalloc", __FILE__, __LINE__)
    /// vmalloc(size,alignment,Allocator)
    #define vmalloc_3(_1, _2, _3)                                  V::AllocatorInstance< _3 >::Get().Allocate(_1, _2, 0, "vmalloc", __FILE__, __LINE__)
    /// vmalloc(size,alignment,Allocator,allocationName)
    #define vmalloc_4(_1, _2, _3, _4)                              V::AllocatorInstance< _3 >::Get().Allocate(_1, _2, 0, _4, __FILE__, __LINE__)
    /// vmalloc(size,alignment,Allocator,allocationName,flags)
    #define vmalloc_5(_1, _2, _3, _4, _5)                          V::AllocatorInstance< _3 >::Get().Allocate(_1, _2, _5, _4, __FILE__, __LINE__)

    /// vcreate(class,params)
    #define vcreate_2(_1, _2)                                      new(vmalloc_4(sizeof(_1), VStd::alignment_of< _1 >::value, V::SystemAllocator,#_1)) _1 _2
    /// vcreate(class,params,Allocator)
    #define vcreate_3(_1, _2, _3)                                  new(vmalloc_4(sizeof(_1), VStd::alignment_of< _1 >::value, _3,#_1)) _1 _2
    /// vcreate(class,params,Allocator,allocationName)
    #define vcreate_4(_1, _2, _3, _4)                              new(vmalloc_4(sizeof(_1), VStd::alignment_of< _1 >::value, _3, _4)) _1 _2
    /// vcreate(class,params,Allocator,allocationName,flags)
    #define vcreate_5(_1, _2, _3, _4, _5)                          new(vmalloc_5(sizeof(_1), VStd::alignment_of< _1 >::value, _3, _4, _5)) _1 _2
#else // defined(_RELEASE)
    #define vnew           new((const V::Internal::AllocatorDummy*)nullptr)
    #define vnewex(_Name)  vnew

    /// vmalloc(size)
        #define vmalloc_1(_1)                                          V::AllocatorInstance< V::SystemAllocator >::Get().Allocate(_1, 0, 0)
    /// vmalloc(size,alignment)
        #define vmalloc_2(_1, _2)                                      V::AllocatorInstance< V::SystemAllocator >::Get().Allocate(_1, _2, 0)
    /// vmalloc(size,alignment,Allocator)
        #define vmalloc_3(_1, _2, _3)                                  V::AllocatorInstance< _3 >::Get().Allocate(_1, _2, 0)
    /// vmalloc(size,alignment,Allocator,allocationName)
        #define vmalloc_4(_1, _2, _3, _4)                              V::AllocatorInstance< _3 >::Get().Allocate(_1, _2, 0, _4)
    /// vmalloc(size,alignment,Allocator,allocationName,flags)
        #define vmalloc_5(_1, _2, _3, _4, _5)                          V::AllocatorInstance< _3 >::Get().Allocate(_1, _2, _5, _4)

    /// vcreate(class)
        #define vcreate_1(_1)                                          new(vmalloc_4(sizeof(_1), VStd::alignment_of< _1 >::value, V::SystemAllocator, #_1)) _1()
    /// vcreate(class,params)
        #define vcreate_2(_1, _2)                                      new(vmalloc_4(sizeof(_1), VStd::alignment_of< _1 >::value, V::SystemAllocator, #_1)) _1 _2
    /// vcreate(class,params,Allocator)
        #define vcreate_3(_1, _2, _3)                                  new(vmalloc_4(sizeof(_1), VStd::alignment_of< _1 >::value, _3, #_1)) _1 _2
    /// vcreate(class,params,Allocator,allocationName)
        #define vcreate_4(_1, _2, _3, _4)                              new(vmalloc_4(sizeof(_1), VStd::alignment_of< _1 >::value, _3, _4)) _1 _2
    /// vcreate(class,params,Allocator,allocationName,flags)
        #define vcreate_5(_1, _2, _3, _4, _5)                          new(vmalloc_5(sizeof(_1), VStd::alignment_of< _1 >::value, _3, _4, _5)) _1 _2
#endif // !defined(_RELEASE)

/**
* vmalloc 相当于::malloc(...). 它应该与相应的 vfree 调用一起使用.
* 宏定义: vmalloc(size_t byteSize, size_t alignment = DefaultAlignment, AllocatorType = V::SystemAllocator, const char* name = "Default Name", int flags = 0)
*/
#define vmalloc(...)       V_MACRO_SPECIALIZE(vmalloc_, V_VA_NUM_ARGS(__VA_ARGS__), (__VA_ARGS__))

/// vcalloc(size)
#define vcalloc_1(_1)                  ::memset(vmalloc_1(_1), 0, _1)
/// vcalloc(size, alignment)
#define vcalloc_2(_1, _2)              ::memset(vmalloc_2(_1, _2), 0, _1);
/// vcalloc(size, alignment, Allocator)
#define vcalloc_3(_1, _2, _3)          ::memset(vmalloc_3(_1, _2, _3), 0, _1);
/// vcalloc(size, alignment, allocationName)
#define vcalloc_4(_1, _2, _3, _4)      ::memset(vmalloc_4(_1, _2, _3, _4), 0, _1);
/// vcalloc(size, alignment, allocationName, flags)
#define vcalloc_5(_1, _2, _3, _4, _5)  ::memset(vmalloc_5(_1, _2, _3, _4, _5), 0, _1);

/**
* vcalloc 相当于 ::memset(vmalloc(...), 0, size);
* 宏定义: vcalloc(size, alignment = DefaultAlignment, AllocatorType = V::SystemAllocator, const char* name = "Default Name", int flags = 0)
*/
#define vcalloc(...)       V_MACRO_SPECIALIZE(vcalloc_, V_VA_NUM_ARGS(__VA_ARGS__), (__VA_ARGS__))

/// vrealloc(ptr, size)
#define vrealloc_2(_1, _2)             V::AllocatorInstance<V::SystemAllocator>::Get().ReAllocate(_1, _2, 0)
/// vrealloc(ptr, size, alignment)
#define vrealloc_3(_1, _2, _3)         V::AllocatorInstance<V::SystemAllocator>::Get().ReAllocate(_1, _2, _3)
/// vrealloc(ptr, size, alignment, Allocator)
#define vrealloc_4(_1, _2, _3, _4)     V::AllocatorInstance<_4>::Get().ReAllocate(_1, _2, _3)

/**
* vrealloc 相当于::realloc(...)
* 宏定义: vrealloc(void* ptr, size_t size, size_t alignment = DefaultAlignment, AllocatorType = V::SystemAllocator)
*/
#define vrealloc(...)      V_MACRO_SPECIALIZE(vrealloc_, V_VA_NUM_ARGS(__VA_ARGS__), (__VA_ARGS__))

/**
 * vcreate is customized vnew function call. vnew can be used anywhere where we use new, while vcreate has a function call signature.
 * vcreate allows you to override the operator new and by this you can override the allocator per object instance. It should
 * be used with corresponding vdestroy call.
 * 宏定义: vcreate(ClassName, CtorParams = (), AllocatorType = V::SystemAllocator, AllocationName = "ClassName", int flags = 0)
 */
#define vcreate(...)       V_MACRO_SPECIALIZE(vcreate_, V_VA_NUM_ARGS(__VA_ARGS__), (__VA_ARGS__))

/// vfree(pointer)
#define vfree_1(_1)                do { if (_1) { V::AllocatorInstance< V::SystemAllocator >::Get().DeAllocate(_1); } }   while (0)
/// vfree(pointer,allocator)
#define vfree_2(_1, _2)            do { if (_1) { V::AllocatorInstance< _2 >::Get().DeAllocate(_1); } }                    while (0)
/// vfree(pointer,allocator,size)
#define vfree_3(_1, _2, _3)        do { if (_1) { V::AllocatorInstance< _2 >::Get().DeAllocate(_1, _3); } }                while (0)
/// vfree(pointer,allocator,size,alignment)
#define vfree_4(_1, _2, _3, _4)    do { if (_1) { V::AllocatorInstance< _2 >::Get().DeAllocate(_1, _3, _4); } }            while (0)

/**
 * vfree 相当于::free(...)。应与相应的 vmalloc 调用一起使用.
 * 宏定义: vfree(Pointer* ptr, AllocatorType = V::SystemAllocator, size_t byteSize = Unknown, size_t alignment = DefaultAlignment);
 * \note 提供分配大小 (byteSize) 和对齐方式是可选的,但建议尽可能提供.它将生成更快的代码.
 */
#define vfree(...)         V_MACRO_SPECIALIZE(vfree_, V_VA_NUM_ARGS(__VA_ARGS__), (__VA_ARGS__))

/// 根据其指针返回分配大小 \ref V::IAllocatorAllocate::AllocationSize.
#define vallocsize(_Ptr, _Allocator)    V::AllocatorInstance< _Allocator >::Get().AllocationSize(_Ptr)
/// 返回新的扩展大小，如果分配器不支持则返回 0 \ref V::IAllocatorAllocate::Resize.
#define vallocresize(_Ptr, _NewSize, _Allocator) V::AllocatorInstance< _Allocator >::Get().Resize(_Ptr, _NewSize)

namespace V {
    // \note we can use VStd::Internal::destroy<pointer_type>::single(ptr) if we template the entire function.
    namespace Memory {
        namespace Internal {
            template<class T>
            V_FORCE_INLINE void call_dtor(T* ptr)          { (void)ptr; ptr->~T(); }
        }
    }
}

#define vdestroy_1(_1)         do { V::Memory::Internal::call_dtor(_1); vfree_1(_1); } while (0)
#define vdestroy_2(_1, _2)      do { V::Memory::Internal::call_dtor(_1); vfree_2(_1, _2); } while (0)
#define vdestroy_3(_1, _2, _3)     do { V::Memory::Internal::call_dtor(reinterpret_cast<_3*>(_1)); vfree_4(_1, _2, sizeof(_3), VStd::alignment_of< _3 >::value); } while (0)

/**
 * vdestroy 只能与相应的 vcreate 一起使用.
 * 宏定义: vdestroy(Pointer*, AllocatorType = V::SystemAllocator, ClassName = Unknown)
 * \note 提供 ClassName 是可选的, 但建议尽可能提供. 它将生成更快的代码.
 */
#define vdestroy(...)      V_MACRO_SPECIALIZE(vdestroy_, V_VA_NUM_ARGS(__VA_ARGS__), (__VA_ARGS__))


/**
 * Class Allocator (override new/delete operators)
 *
 * use this macro inside your objects to define it's default allocation (ex.):
 * class MyObj {
 *   public:
 *          V_CLASS_ALLOCATOR(MyObj,SystemAllocator,0) - inline version requires to include the allocator (in this case sysallocator.h)
 *      or
 *          V_CLASS_ALLOCATOR_DECL in the header and V_CLASS_ALLOCATOR_IMPL(MyObj,SystemAllocator,0) in the cpp file. This way you don't need
 *          to include the allocator header where you decl your class (MyObj).
 *      ...
 * };
 *
 * \note 我们不支持数组运算符 [], 因为它们会插入编译器/平台相关的数组头, 这会在某些情况下破坏对齐.
 * 如果要使用动态数组, 请使用 VStd::vector 或 VStd::fixed_vector.
 * 当然，如果确实需要, 您可以使用placement new 并进行数组分配.
 */
#if __cpp_aligned_new
// C++17 对齐大于 sizeof(max_align_t) 的类型的对齐分配重载.
#define _V_CLASS_ALLOCATOR_ALIGNED_NEW(_Class, _Allocator, _Flags)                                                                                                                          \
    /* class-specific allocation functions */                                                                                                                                                \
    [[nodiscard]] void* operator new(std::size_t size, std::align_val_t align) {                                                                                                             \
        return operator new(size, align, nullptr, 0, #_Class);                                                                                                                               \
    }                                                                                                                                                                                        \
    [[nodiscard]] void* operator new(std::size_t size, std::align_val_t align, const std::nothrow_t&) noexcept {                                                                             \
        return operator new(size, align, nullptr, 0, #_Class);                                                                                                                               \
    }                                                                                                                                                                                        \
    [[nodiscard]] void* operator new(std::size_t size, std::align_val_t align, const char* fileName, int lineNum, const char* name) {                                                        \
        V_Assert(size >= sizeof(_Class), "Size mismatch! Did you forget to declare the macro in derived class? Size: %d sizeof(%s): %d", size, #_Class, sizeof(_Class));                    \
        return V::AllocatorInstance< _Allocator >::Get().Allocate(size, static_cast<std::size_t>(align), _Flags, name ? name : #_Class, fileName, lineNum);                                 \
    }                                                                                                                                                                                        \
    [[nodiscard]] void* operator new[]([[maybe_unused]] std::size_t size, [[maybe_unused]] std::align_val_t align) {                                                                         \
        V_Assert(false, "We DO NOT support array operators, because it's really hard/impossible to handle alignment without proper tracking!\n"                                             \
                          "new[] inserts a header (platform dependent) to keep track of the array size!\n"                                                                                   \
                          "Use VStd::vector,VStd::array,VStd::fixed_vector or placement new and it's your responsibility!");                                                              \
        return V_INVALID_POINTER;                                                                                                                                                           \
    }                                                                                                                                                                                        \
    [[nodiscard]] void* operator new[](std::size_t size, std::align_val_t align, const std::nothrow_t&) noexcept {                                                                           \
        return operator new[](size, align);                                                                                                                                                  \
    }                                                                                                                                                                                        \
    [[nodiscard]] void* operator new[](std::size_t size, std::align_val_t align, const char*, int, const char*) {                                                                            \
        return operator new[](size, align);                                                                                                                                                  \
    }                                                                                                                                                                                        \
    void operator delete(void* p, std::align_val_t align) noexcept {                                                                                                                         \
        return operator delete(p, 0, align);                                                                                                                                    \
    }                                                                                                                                                                                        \
    void operator delete(void* p, std::size_t size, std::align_val_t align) noexcept {                                                                                                       \
        if (p) { V::AllocatorInstance< _Allocator >::Get().DeAllocate(p, size, static_cast<std::size_t>(align)); }                                                                          \
    }                                                                                                                                                                                        \
    void operator delete(void* p, std::align_val_t align, const std::nothrow_t&) noexcept {                                                                                                  \
        return operator delete(p, 0, align);                                                                                                                                    \
    }                                                                                                                                                                                        \
    void operator delete(void* p, std::align_val_t align, [[maybe_unused]] const char* fileName, [[maybe_unused]] int lineNum, [[maybe_unused]] const char* name) noexcept {                 \
        return operator delete(p, 0, align);                                                                                                                                    \
    }                                                                                                                                                                                        \
    void operator delete[]([[maybe_unused]] void* p, [[maybe_unused]] std::align_val_t align) noexcept {                                                                                     \
        V_Assert(false, "We DO NOT support array operators, because it's really hard/impossible to handle alignment without proper tracking!\n"                                             \
                          "new[] inserts a header (platform dependent) to keep track of the array size!\n"                                                                                   \
                          "Use VStd::vector,VStd::array,VStd::fixed_vector or placement new and it's your responsibility!");                                                              \
    }                                                                                                                                                                                        \
    void operator delete[](void* p, [[maybe_unused]] std::size_t size, std::align_val_t align) noexcept {                                                                                    \
        return operator delete[](p, align);                                                                                                                                                  \
    }                                                                                                                                                                                        \
    void operator delete[](void* p, std::align_val_t align, const std::nothrow_t&) noexcept {                                                                                                \
        return operator delete[](p, align);                                                                                                                                                  \
    }                                                                                                                                                                                        \
    void operator delete[](void* p, std::align_val_t align, [[maybe_unused]] const char* fileName, [[maybe_unused]] int lineNum, [[maybe_unused]] const char* name) noexcept {               \
        return operator delete[](p, align);                                                                                                                                                  \
    }
#else
#define _V_CLASS_ALLOCATOR_ALIGNED_NEW(_Class, _Allocator, _Flags)
#endif

#define V_CLASS_ALLOCATOR(_Class, _Allocator, _Flags)                                                                                                                              \
    /* ========== placement operators (default) ========== */                                                                                                                       \
    V_FORCE_INLINE void* operator new(std::size_t, void* p)    { return p; }   /* placement new */                                                                                 \
    V_FORCE_INLINE void* operator new[](std::size_t, void* p)  { return p; }   /* placement array new */                                                                           \
    V_FORCE_INLINE void  operator delete(void*, void*)         { }             /* placement delete, called when placement new asserts */                                           \
    V_FORCE_INLINE void  operator delete[](void*, void*)       { }             /* placement array delete */                                                                        \
    /* ========== standard operator new/delete ========== */                                                                                                                        \
    V_FORCE_INLINE void* operator new(std::size_t size) {                      /* default operator new (called with "new _Class()") */                                             \
        V_Assert(size >= sizeof(_Class), "Size mismatch! Did you forget to declare the macro in derived class? Size: %d sizeof(%s): %d", size, #_Class, sizeof(_Class));           \
        V_Warning(0, true/*false*/, "Make sure you use vnew, offers better tracking!" /*Warning temporarily disabled until engine is using V allocators.*/);                     \
        return V::AllocatorInstance< _Allocator >::Get().Allocate(size, VStd::alignment_of< _Class >::value, _Flags,#_Class);                                                     \
    }                                                                                                                                                                               \
    V_FORCE_INLINE void  operator delete(void* p, std::size_t size) {    /* default operator delete */                                                                             \
        if (p) { V::AllocatorInstance< _Allocator >::Get().DeAllocate(p, size, VStd::alignment_of< _Class >::value); }                                                            \
    }                                                                                                                                                                               \
    /* ========== vnew (called "vnew _Class()") ========== */                                                                                                                     \
    V_FORCE_INLINE void* operator new(std::size_t size, const char* fileName, int lineNum, const char* name) { /* with tracking */                                                 \
        V_Assert(size >= sizeof(_Class), "Size mismatch! Did you forget to declare the macro in derived class? Size: %d sizeof(_type): %d", size, sizeof(_Class));                 \
        return V::AllocatorInstance< _Allocator >::Get().Allocate(size, VStd::alignment_of< _Class >::value, _Flags, (name == 0) ?#_Class: name, fileName, lineNum);              \
    }                                                                                                                                                                               \
    V_FORCE_INLINE void* operator new(std::size_t size, const V::Internal::AllocatorDummy*) {                 /* without tracking */                                              \
        V_Assert(size >= sizeof(_Class), "Size mismatch! Did you forget to declare the macro in derived class? Size: %d sizeof(_type): %d", size, sizeof(_Class));                 \
        return V::AllocatorInstance< _Allocator >::Get().Allocate(size, VStd::alignment_of< _Class >::value);                                                                     \
    }                                                                                                                                                                               \
    /* ========== Symetrical delete operators (required incase vnew throws) ========== */                                                                                          \
    V_FORCE_INLINE void  operator delete(void* p, const char*, int, const char*) {                                                                                                 \
        if (p) { V::AllocatorInstance< _Allocator >::Get().DeAllocate(p); }                                                                                                        \
    }                                                                                                                                                                               \
    V_FORCE_INLINE void  operator delete(void* p, const V::Internal::AllocatorDummy*) {                                                                                           \
        if (p) { V::AllocatorInstance< _Allocator >::Get().DeAllocate(p); }                                                                                                        \
    }                                                                                                                                                                               \
    /* ========== Unsupported operators ========== */                                                                                                                               \
    V_FORCE_INLINE void* operator new[](std::size_t) {                                         /* default array operator new (called with "new _Class[x]") */                      \
        V_Assert(false, "We DO NOT support array operators, because it's really hard/impossible to handle alignment without proper tracking!\n"                                    \
                         "new[] inserts a header (platform dependent) to keep track of the array size!\n"                                                                           \
                         "Use VStd::vector,VStd::array,VStd::fixed_vector or placement new and it's your responsibility!");                                                      \
        return V_INVALID_POINTER;                                                                                                                                                  \
    }                                                                                                                                                                               \
    V_FORCE_INLINE void  operator delete[](void*) {                                            /* default array operator delete */                                                 \
        V_Assert(false, "We DO NOT support array operators, because it's really hard/impossible to handle alignment without proper tracking!\n"                                    \
                         "new[] inserts a header (platform dependent) to keep track of the array size!\n"                                                                           \
                         "Use VStd::vector,VStd::array,VStd::fixed_vector or placement new and it's your responsibility!");                                                      \
    }                                                                                                                                                                               \
    V_FORCE_INLINE void* operator new[](std::size_t, const char*, int, const char*) {          /* array operator vnew with tracking (called with "vnew _Class[x]") */            \
        V_Assert(false, "We DO NOT support array operators, because it's really hard/impossible to handle alignment without proper tracking!\n"                                    \
                         "new[] inserts a header (platform dependent) to keep track of the array size!\n"                                                                           \
                         "Use VStd::vector,VStd::array,VStd::fixed_vector or placement new and it's your responsibility!");                                                      \
        return V_INVALID_POINTER;                                                                                                                                                  \
    }                                                                                                                                                                               \
    V_FORCE_INLINE void* operator new[](std::size_t, const V::Internal::AllocatorDummy*) {    /* array operator vnew without tracking (called with "vnew _Class[x]") */         \
        V_Assert(false, "We DO NOT support array operators, because it's really hard/impossible to handle alignment without proper tracking!\n"                                    \
                         "new[] inserts a header (platform dependent) to keep track of the array size!\n"                                                                           \
                         "Use VStd::vector,VStd::array,VStd::fixed_vector or placement new and it's your responsibility!");                                                      \
        return V_INVALID_POINTER;                                                                                                                                                  \
    }                                                                                                                                                                               \
    /* ========== V_CLASS_ALLOCATOR API ========== */                                                                                                                              \
    V_FORCE_INLINE static void* V_CLASS_ALLOCATOR_Allocate() {                                                                                                                    \
        return V::AllocatorInstance< _Allocator >::Get().Allocate(sizeof(_Class), VStd::alignment_of< _Class >::value, _Flags, #_Class);                                          \
    }                                                                                                                                                                               \
    V_FORCE_INLINE static void V_CLASS_ALLOCATOR_DeAllocate(void* object) {                                                                                                       \
        V::AllocatorInstance< _Allocator >::Get().DeAllocate(object, sizeof(_Class), VStd::alignment_of< _Class >::value);                                                        \
    }                                                                                                                                                                               \
    template<bool Placeholder = true> void V_CLASS_ALLOCATOR_DECLARED();                                                                                                           \
    _V_CLASS_ALLOCATOR_ALIGNED_NEW(_Class, _Allocator, _Flags)

// 如果您想避免在头文件中包含内存管理器类, 请使用 _DECL（声明）和 _IMPL（实现/定义）宏
#if __cpp_aligned_new
// 声明 C++17aligned_new 运算符 new/operatordelete 重载
#define _V_CLASS_ALLOCATOR_DECL_ALIGNED_NEW                                                                                                                 \
    /* class-specific allocation functions */                                                                                                                \
    [[nodiscard]] void* operator new(std::size_t size, std::align_val_t align);                                                                              \
    [[nodiscard]] void* operator new(std::size_t size, std::align_val_t align, const std::nothrow_t&) noexcept;                                              \
    [[nodiscard]] void* operator new(std::size_t size, std::align_val_t align, const char* fileName, int lineNum, const char* name);                         \
    [[nodiscard]] void* operator new[](std::size_t, std::align_val_t);                                                                                       \
    [[nodiscard]] void* operator new[](std::size_t size, std::align_val_t align, const std::nothrow_t&) noexcept;                                            \
    [[nodiscard]] void* operator new[](std::size_t size, std::align_val_t align, const char*, int, const char*);                                             \
    void operator delete(void* p, std::align_val_t align) noexcept;                                                                                          \
    void operator delete(void* p, std::size_t size, std::align_val_t align) noexcept;                                                                        \
    void operator delete(void* p, std::align_val_t align, const std::nothrow_t&) noexcept;                                                                   \
    void operator delete(void* p, std::align_val_t align, [[maybe_unused]] const char* fileName, [[maybe_unused]] int lineNum, const char* name) noexcept;   \
    void operator delete[](void* p, std::align_val_t align) noexcept;                                                                                        \
    void operator delete[](void* p, std::size_t size, std::align_val_t align) noexcept;                                                                      \
    void operator delete[](void* p, std::align_val_t align, const std::nothrow_t&) noexcept;                                                                 \
    void operator delete[](void* p, std::align_val_t align, [[maybe_unused]] const char* fileName, [[maybe_unused]] int lineNum, const char* name) noexcept;
#else
#define _V_CLASS_ALLOCATOR_DECL_ALIGNED_NEW
#endif

#define V_CLASS_ALLOCATOR_DECL                                                                                                                           \
    /* ========== placement operators (default) ========== */                                                                                             \
    V_FORCE_INLINE void* operator new(std::size_t, void* p)    { return p; }                                                                             \
    V_FORCE_INLINE void* operator new[](std::size_t, void* p)  { return p; }                                                                             \
    V_FORCE_INLINE void operator delete(void*, void*)          { }                                                                                       \
    V_FORCE_INLINE void operator delete[](void*, void*)        { }                                                                                       \
    /* ========== standard operator new/delete ========== */                                                                                              \
    void* operator new(std::size_t size);                                                                                                                 \
    void  operator delete(void* p, std::size_t size);                                                                                                     \
    /* ========== vnew (called "vnew _Class()")========== */                                                                                            \
    void* operator new(std::size_t size, const char* fileName, int lineNum, const char* name);                                                            \
    void* operator new(std::size_t size, const V::Internal::AllocatorDummy*);                                                                            \
    /* ========== Symetrical delete operators (required incase vnew throws) ========== */                                                                \
    void  operator delete(void* p, const char*, int, const char*);                                                                                        \
    void  operator delete(void* p, const V::Internal::AllocatorDummy*);                                                                                  \
    /* ========== Unsupported operators ========== */                                                                                                     \
    V_FORCE_INLINE void* operator new[](std::size_t) {                                                                                                   \
        V_Assert(false, "We DO NOT support array operators, because it's really hard/impossible to handle alignment without proper tracking!\n"          \
                        "new[] inserts a header (platform dependent) to keep track of the array size!\n"                                                  \
                        "Use VStd::vector,VStd::array,VStd::fixed_vector or placement new and it's your responsibility!");                             \
        return V_INVALID_POINTER;                                                                                                                        \
    }                                                                                                                                                     \
    V_FORCE_INLINE void  operator delete[](void*) {                                                                                                      \
        V_Assert(false, "We DO NOT support array operators, because it's really hard/impossible to handle alignment without proper tracking!\n"          \
                        "new[] inserts a header (platform dependent) to keep track of the array size!\n"                                                  \
                        "Use VStd::vector,VStd::array,VStd::fixed_vector or placement new and it's your responsibility!");                             \
    }                                                                                                                                                     \
    V_FORCE_INLINE void* operator new[](std::size_t, const char*, int, const char*, const V::Internal::AllocatorDummy*) {                               \
        V_Assert(false, "We DO NOT support array operators, because it's really hard/impossible to handle alignment without proper tracking!\n"          \
            "new[] inserts a header (platform dependent) to keep track of the array size!\n"                                                              \
            "Use VStd::vector,VStd::array,VStd::fixed_vector or placement new and it's your responsibility!");                                         \
        return V_INVALID_POINTER;                                                                                                                        \
    }                                                                                                                                                     \
    V_FORCE_INLINE void* operator new[](std::size_t, const V::Internal::AllocatorDummy*) {                                                              \
        V_Assert(false, "We DO NOT support array operators, because it's really hard/impossible to handle alignment without proper tracking!\n"          \
            "new[] inserts a header (platform dependent) to keep track of the array size!\n"                                                              \
            "Use VStd::vector,VStd::array,VStd::fixed_vector or placement new and it's your responsibility!");                                         \
        return V_INVALID_POINTER;                                                                                                                        \
    }                                                                                                                                                     \
    /* ========== V_CLASS_ALLOCATOR API ========== */                                                                                                    \
    static void* V_CLASS_ALLOCATOR_Allocate();                                                                                                           \
    static void  V_CLASS_ALLOCATOR_DeAllocate(void* object);                                                                                             \
    template<bool Placeholder = true> void V_CLASS_ALLOCATOR_DECLARED();                                                                                 \
    _V_CLASS_ALLOCATOR_DECL_ALIGNED_NEW


#if __cpp_aligned_new
// Defines the C++17 aligned_new operator new/operator delete overloads
#define _V_CLASS_ALLOCATOR_IMPL_ALIGNED_NEW(_Class, _Allocator, _Flags, _Template)                                                                                                                     \
    _Template [[nodiscard]] void* _Class::operator new(std::size_t size, std::align_val_t align, const char* fileName, int lineNum, const char* name) {                                                 \
        V_Assert(size >= sizeof(_Class), "Size mismatch! Did you forget to declare the macro in derived class? Size: %d sizeof(%s): %d", size, #_Class, sizeof(_Class));                               \
        return V::AllocatorInstance< _Allocator >::Get().Allocate(size, static_cast<std::size_t>(align), _Flags, name ? name : #_Class, fileName, lineNum);                                            \
    }                                                                                                                                                                                                   \
    _Template [[nodiscard]] void* _Class::operator new(std::size_t size, std::align_val_t align) {                                                                                                      \
        return operator new(size, align, nullptr, 0, #_Class);                                                                                                                                          \
    }                                                                                                                                                                                                   \
    _Template [[nodiscard]] void* _Class::operator new(std::size_t size, std::align_val_t align, const std::nothrow_t&) noexcept {                                                                      \
        return operator new(size, align, nullptr, 0, #_Class);                                                                                                                                          \
    }                                                                                                                                                                                                   \
    _Template [[nodiscard]] void* _Class::operator new[]([[maybe_unused]] std::size_t, [[maybe_unused]] std::align_val_t) {                                                                             \
        V_Assert(false, "We DO NOT support array operators, because it's really hard/impossible to handle alignment without proper tracking!\n"                                                        \
                          "new[] inserts a header (platform dependent) to keep track of the array size!\n"                                                                                              \
                          "Use VStd::vector,VStd::array,VStd::fixed_vector or placement new and it's your responsibility!");                                                                         \
        return V_INVALID_POINTER;                                                                                                                                                                      \
    }                                                                                                                                                                                                   \
    _Template [[nodiscard]] void* _Class::operator new[](std::size_t size, std::align_val_t align, const std::nothrow_t&) noexcept {                                                                    \
        return operator new[](size, align);                                                                                                                                                             \
    }                                                                                                                                                                                                   \
    _Template [[nodiscard]] void* _Class::operator new[](std::size_t size, std::align_val_t align, const char*, int, const char*) {                                                                     \
        return operator new[](size, align);                                                                                                                                                             \
    }                                                                                                                                                                                                   \
    _Template void _Class::operator delete(void* p, std::size_t size, std::align_val_t align) noexcept {                                                                                                \
        if (p) { V::AllocatorInstance< _Allocator >::Get().DeAllocate(p, size, static_cast<std::size_t>(align)); }                                                                                     \
    }                                                                                                                                                                                                   \
    _Template void _Class::operator delete(void* p, std::align_val_t align) noexcept {                                                                                                                  \
        return operator delete(p, 0, align);                                                                                                                                               \
    }                                                                                                                                                                                                   \
    _Template void _Class::operator delete(void* p, std::align_val_t align, const std::nothrow_t&) noexcept {                                                                                           \
        return operator delete(p, 0, align);                                                                                                                                               \
    }                                                                                                                                                                                                   \
    _Template void _Class::operator delete(void* p, std::align_val_t align, [[maybe_unused]] const char* fileName, [[maybe_unused]] int lineNum, [[maybe_unused]] const char* name) noexcept {          \
        return operator delete(p, 0, align);                                                                                                                                               \
    }                                                                                                                                                                                                   \
    _Template void _Class::operator delete[]([[maybe_unused]] void* p, [[maybe_unused]] std::align_val_t align) noexcept {                                                                              \
        V_Assert(false, "We DO NOT support array operators, because it's really hard/impossible to handle alignment without proper tracking!\n"                                                        \
                          "new[] inserts a header (platform dependent) to keep track of the array size!\n"                                                                                              \
                          "Use VStd::vector,VStd::array,VStd::fixed_vector or placement new and it's your responsibility!");                                                                         \
    }                                                                                                                                                                                                   \
    _Template void _Class::operator delete[](void* p, [[maybe_unused]] std::size_t size, std::align_val_t align) noexcept {                                                                             \
        return _Class::operator delete[](p, align);                                                                                                                                                             \
    }                                                                                                                                                                                                   \
    _Template void _Class::operator delete[](void* p, std::align_val_t align, const std::nothrow_t&) noexcept {                                                                                         \
        return _Class::operator delete[](p, align);                                                                                                                                                             \
    }                                                                                                                                                                                                   \
    _Template void _Class::operator delete[](void* p, std::align_val_t align, [[maybe_unused]] const char* fileName, [[maybe_unused]] int lineNum, [[maybe_unused]] const char* name) noexcept {        \
        return _Class::operator delete[](p, align);                                                                                                                                                             \
    }
#else
#define _V_CLASS_ALLOCATOR_IMPL_ALIGNED_NEW(_Class, _Allocator, _Flags, _Template)
#endif
#define V_CLASS_ALLOCATOR_IMPL_INTERNAL(_Class, _Allocator, _Flags, _Template)                                                                                                                                                                     \
    /* ========== standard operator new/delete ========== */                                                                                                                                                                                        \
    _Template                                                                                                                                                                                                                                       \
    [[nodiscard]] void* _Class::operator new(std::size_t size)                                                                                                                                                                                      \
    {                                                                                                                                                                                                                                               \
        V_Assert(size >= sizeof(_Class), "Size mismatch! Did you forget to declare the macro in derived class? Size: %d sizeof(_Class): %d", size, sizeof(_Class));                                                                                \
        V_Warning(0, false, "Make sure you use vnew, offers better tracking!");                                                                                                                                                                   \
        return V::AllocatorInstance< _Allocator >::Get().Allocate(size, VStd::alignment_of< _Class >::value, _Flags,#_Class);                                                                                                                     \
    }                                                                                                                                                                                                                                               \
    _Template                                                                                                                                                                                                                                       \
    void _Class::operator delete(void* p, std::size_t size)  {                                                                                                                                                                                      \
        if (p) { V::AllocatorInstance< _Allocator >::Get().DeAllocate(p, size, VStd::alignment_of< _Class >::value); }                                                                                                                            \
    }                                                                                                                                                                                                                                               \
    /* ========== vnew (called "vnew _Class()")========== */                                                                                                                                                                                      \
    _Template                                                                                                                                                                                                                                       \
    [[nodiscard]] void* _Class::operator new(std::size_t size, const char* fileName, int lineNum, const char* name) {                                                                                                                               \
        V_Assert(size >= sizeof(_Class), "Size mismatch! Did you forget to declare the macro in derived class? Size: %d sizeof(_type): %d", size, sizeof(_Class));                                                                                 \
        return V::AllocatorInstance< _Allocator >::Get().Allocate(size, VStd::alignment_of< _Class >::value, _Flags, (name == 0) ?#_Class: name, fileName, lineNum);                                                                              \
    }                                                                                                                                                                                                                                               \
    _Template                                                                                                                                                                                                                                       \
    [[nodiscard]] void* _Class::operator new(std::size_t size, const V::Internal::AllocatorDummy*) {                                                                                                                                               \
        V_Assert(size >= sizeof(_Class), "Size mismatch! Did you forget to declare the macro in derived class? Size: %d sizeof(_type): %d", size, sizeof(_Class));                                                                                 \
        return V::AllocatorInstance< _Allocator >::Get().Allocate(size, VStd::alignment_of< _Class >::value);                                                                                                                                     \
    }                                                                                                                                                                                                                                               \
    /* ========== Symetrical delete operators (required incase vnew throws) ========== */                                                                                                                                                          \
    _Template                                                                                                                                                                                                                                       \
    void  _Class::operator delete(void* p, const char*, int, const char*) {                                                                                                                                                                         \
        if (p) { V::AllocatorInstance< _Allocator >::Get().DeAllocate(p); }                                                                                                                                                                        \
    }                                                                                                                                                                                                                                               \
    _Template                                                                                                                                                                                                                                       \
    void  _Class::operator delete(void* p, const V::Internal::AllocatorDummy*) {                                                                                                                                                                   \
        if (p) { V::AllocatorInstance< _Allocator >::Get().DeAllocate(p); }                                                                                                                                                                        \
    }                                                                                                                                                                                                                                               \
    /* ========== V_CLASS_ALLOCATOR API ========== */                                                                                                                                                                                              \
    _Template                                                                                                                                                                                                                                       \
    [[nodiscard]] void* _Class::V_CLASS_ALLOCATOR_Allocate() {                                                                                                                                                                                     \
        return V::AllocatorInstance< _Allocator >::Get().Allocate(sizeof(_Class), VStd::alignment_of< _Class >::value, _Flags, #_Class);                                                                                                          \
    }                                                                                                                                                                                                                                               \
    _Template                                                                                                                                                                                                                                       \
    void _Class::V_CLASS_ALLOCATOR_DeAllocate(void* object) {                                                                                                                                                                                      \
        V::AllocatorInstance< _Allocator >::Get().DeAllocate(object, sizeof(_Class), VStd::alignment_of< _Class >::value);                                                                                                                        \
    }                                                                                                                                                                                                                                               \
    _V_CLASS_ALLOCATOR_IMPL_ALIGNED_NEW(_Class, _Allocator, _Flags, _Template)

#define V_CLASS_ALLOCATOR_IMPL(_Class, _Allocator, _Flags)                                                                                                                                                                                         \
    V_CLASS_ALLOCATOR_IMPL_INTERNAL(_Class, _Allocator, _Flags,)

#define V_CLASS_ALLOCATOR_IMPL_TEMPLATE(_Class, _Allocator, _Flags)                                                                                                                                                                                \
    V_CLASS_ALLOCATOR_IMPL_INTERNAL(_Class, _Allocator, _Flags, template<>)

//////////////////////////////////////////////////////////////////////////
// new operator overloads

// you can redefine this macro to whatever suits you.
#ifndef VCORE_GLOBAL_NEW_ALIGNMENT
#   define VCORE_GLOBAL_NEW_ALIGNMENT 16
#endif

/**
 * By default VCore doesn't overload operator new and delete. This is a no-no for middle-ware.
 * You are encouraged to do that in your executable. What you need to do is to pipe all allocation trough VCore memory manager.
 * VCore relies on \ref V_CLASS_ALLOCATOR to specify the class default allocator or on explicit
 * azcreate/azdestroy calls which provide the allocator. If you class doesn't not implement the
 * \ref V_CLASS_ALLOCATOR when you call a new/delete they will use the global operator new/delete. In addition
 * if you call aznew on a class without V_CLASS_ALLOCATOR you will need to implement new operator specific to
 * aznew call signature.
 * So in an exception free environment (VLibs don't have exception support) you need to implement the following functions:
 *
 * void* operator new(std::size_t size, const char* fileName, int lineNum, const char* name, const V::Internal::AllocatorDummy*);
 * void* operator new[](std::size_t size, const char* fileName, int lineNum, const char* name, const V::Internal::AllocatorDummy*);
 * void* operator new(std::size_t);
 * void* operator new[](std::size_t);
 * void operator delete(void*);
 * void operator delete[](void*);
 *
 * You can implement those functions anyway you like, or you can use the provided implementations for you! \ref Global New/Delete Operators
 * All allocations will happen using the V::SystemAllocator. Make sure you create it properly before any new calls.
 * If you use our default new implementation you should map the global functions like that:
 *
 * void* operator new(std::size_t size, const char* fileName, int lineNum, const char* name, const V::Internal::AllocatorDummy*)       { return V::OperatorNew(size,fileName,lineNum,name); }
 * void* operator new[](std::size_t size, const char* fileName, int lineNum, const char* name, const V::Internal::AllocatorDummy*)     { return V::OperatorNewArray(size,fileName,lineNum,name); }
 * void* operator new(std::size_t size)         { return V::OperatorNew(size); }
 * void* operator new[](std::size_t size)       { return V::OperatorNewArray(size); }
 * void operator delete(void* ptr)              { V::OperatorDelete(ptr); }
 * void operator delete[](void* ptr)            { V::OperatorDeleteArray(ptr); }
 */
namespace V
{
    namespace AllocatorStorage
    {
        /// A private structure to create heap-storage for an allocator that won't expire until other static module members are destructed.
        struct LazyAllocatorRef
        {
            using CreationFn = IAllocator*(*)(void*);
            using DestructionFn = void(*)(IAllocator&);

            ~LazyAllocatorRef();
            void Init(size_t size, size_t alignment, CreationFn creationFn, DestructionFn destructionFn);

            IAllocator* m_allocator = nullptr;
            DestructionFn m_destructor = nullptr;
        };

        /**
        * A base class for all storage policies. This exists to provide access to private IAllocator methods via template friends.
        */
        template<class Allocator>
        class StoragePolicyBase
        {
        protected:
            static void Create(Allocator& allocator, const typename Allocator::Descriptor& desc, bool lazilyCreated)
            {
                allocator.Create(desc);
                allocator.PostCreate();
                allocator.SetLazilyCreated(lazilyCreated);
            }

            static void SetLazilyCreated(Allocator& allocator, bool lazilyCreated)
            {
                allocator.SetLazilyCreated(lazilyCreated);
            }

            static void Destroy(IAllocator& allocator)
            {
                allocator.PreDestroy();
                allocator.Destroy();
            }
        };

        /**
        * EnvironmentStoragePolicy stores the allocator singleton in the shared Environment.
        * This is the default, preferred method of storing allocators.
        */
        template<class Allocator>
        class EnvironmentStoragePolicy : public StoragePolicyBase<Allocator>
        {
        public:
            static IAllocator& GetAllocator()
            {
                if (!s_allocator)
                {
                    // Assert here before attempting to resolve. Otherwise a module-local
                    // environment will be created which will result in a much more difficult to
                    // locate problem
                    V_Assert(V::Environment::IsReady(), "Environment has not been attached yet, allocator cannot be created/resolved");
                    if (V::Environment::IsReady())
                    {
                        s_allocator = Environment::FindVariable<Allocator>(AzTypeInfo<Allocator>::Name());
                        V_Assert(s_allocator, "Allocator '%s' NOT ready for use! Call Create first!", AzTypeInfo<Allocator>::Name());
                    }
                }
                return *s_allocator;
            }

            static void Create(const typename Allocator::Descriptor& desc)
            {
                if (!s_allocator)
                {
                    s_allocator = Environment::CreateVariable<Allocator>(AzTypeInfo<Allocator>::Name());
                    if (s_allocator->IsReady()) // already created in a different module
                    {
                        return;
                    }
                }
                else
                {
                    V_Assert(s_allocator->IsReady(), "Allocator '%s' already created!", AzTypeInfo<Allocator>::Name());
                }

                StoragePolicyBase<Allocator>::Create(*s_allocator, desc, false);
            }

            static void Destroy()
            {
                if (s_allocator)
                {
                    if (s_allocator.IsOwner())
                    {
                        StoragePolicyBase<Allocator>::Destroy(*s_allocator);
                    }
                    s_allocator = nullptr;
                }
                else
                {
                    V_Assert(false, "Allocator '%s' NOT ready for use! Call Create first!", AzTypeInfo<Allocator>::Name());
                }
            }

            V_FORCE_INLINE static bool IsReady()
            {
                if (Environment::IsReady())
                {
                    if (!s_allocator) // if not there check the environment (if available)
                    {
                        s_allocator = Environment::FindVariable<Allocator>(AzTypeInfo<Allocator>::Name());
                    }
                    return s_allocator && s_allocator->IsReady();
                }
                else
                {
                    return false;
                }
            }

            static EnvironmentVariable<Allocator> s_allocator;
        };

        template<class Allocator>
        EnvironmentVariable<Allocator> EnvironmentStoragePolicy<Allocator>::s_allocator;

        /**
        * ModuleStoragePolicy stores the allocator in a static variable that is local to the module using it.
        * This forces separate instances of the allocator to exist in each module, and permits lazy instantiation.
        * We only tolerate this for some special allocators, primarily to maintain backwards compatibility with CryEngine,
        * since it still allocates outside of code in the data section.
        *
        * It has two ways of storing its allocator: either on the heap, which is the preferred way, since it guarantees
        * the memory for the allocator won't be deallocated (such as in a DLL) before anyone that's using it. If disabled
        * the allocator is stored in a static variable, which should only be used where this isn't a problem a shut-down
        * time, such as on a console.
        */
        template<class Allocator, bool StoreAllocatorOnHeap>
        struct ModuleStoragePolicyBase;
        
        template<class Allocator>
        struct ModuleStoragePolicyBase<Allocator, false>: public StoragePolicyBase<Allocator>
        {
        protected:
            // Use a static instance to store the allocator. This is not recommended when the order of shut-down with the module matters, as the allocator could have its memory destroyed 
            // before the users of it are destroyed. The primary use case for this is allocators that need to support the CRT, as they cannot allocate from the heap.
            static Allocator& GetModuleAllocatorInstance()
            {
                static Allocator* s_allocator = nullptr;
                static typename VStd::aligned_storage<sizeof(Allocator), VStd::alignment_of<Allocator>::value>::type s_storage;

                if (!s_allocator)
                {
                    s_allocator = new (&s_storage) Allocator;
                    StoragePolicyBase<Allocator>::Create(*s_allocator, typename Allocator::Descriptor(), true);
                }

                return *s_allocator;
            }
        };

        template<class Allocator>
        struct ModuleStoragePolicyBase<Allocator, true> : public StoragePolicyBase<Allocator>
        {
        protected:
            // Store-on-heap implementation uses the LazyAllocatorRef to create and destroy an allocator using heap-space so there isn't a problem with destruction order within the module.
            static Allocator& GetModuleAllocatorInstance()
            {
                static LazyAllocatorRef s_allocator;

                if (!s_allocator.m_allocator)
                {
                    s_allocator.Init(sizeof(Allocator), VStd::alignment_of<Allocator>::value, [](void* mem) -> IAllocator* { return new (mem) Allocator; }, &StoragePolicyBase<Allocator>::Destroy);
                    StoragePolicyBase<Allocator>::Create(*static_cast<Allocator*>(s_allocator.m_allocator), typename Allocator::Descriptor(), true);
                }

                return *static_cast<Allocator*>(s_allocator.m_allocator);
            }
        };

        template<class Allocator, bool StoreAllocatorOnHeap = true>
        class ModuleStoragePolicy : public ModuleStoragePolicyBase<Allocator, StoreAllocatorOnHeap>
        {
        public:
            using Base = ModuleStoragePolicyBase<Allocator, StoreAllocatorOnHeap>;

            static IAllocator& GetAllocator()
            {
                return Base::GetModuleAllocatorInstance();
            }

            static void Create(const typename Allocator::Descriptor& desc = typename Allocator::Descriptor())
            {
                StoragePolicyBase<Allocator>::Create(Base::GetModuleAllocatorInstance(), desc, true);
            }

            static void Destroy()
            {
                StoragePolicyBase<Allocator>::Destroy(Base::GetModuleAllocatorInstance());
            }

            static bool IsReady()
            {
                return Base::GetModuleAllocatorInstance().IsReady();
            }
        };
    }

    namespace Internal
    {
        /**
        * The main class that provides access to the allocator singleton, with a customizable storage policy for that allocator.
        */
        template<class Allocator, typename StoragePolicy = AllocatorStorage::EnvironmentStoragePolicy<Allocator>>
        class AllocatorInstanceBase
        {
        public:
            typedef typename Allocator::Descriptor Descriptor;

            V_FORCE_INLINE static IAllocatorAllocate& Get()
            {
                return *GetAllocator().GetAllocationSource();
            }

            V_FORCE_INLINE static IAllocator& GetAllocator()
            {
                return StoragePolicy::GetAllocator();
            }

            static void Create(const Descriptor& desc = Descriptor())
            {
                StoragePolicy::Create(desc);
            }

            static void Destroy()
            {
                StoragePolicy::Destroy();
            }

            V_FORCE_INLINE static bool IsReady()
            {
                return StoragePolicy::IsReady();
            }
        };
    }

    /**
     * Standard allocator singleton, using Environment storage. Specialize this for your
     * allocator if you need to control storage or lifetime, by changing the policy class
     * used in AllocatorInstanceBase.
     *
     * It is preferred that you don't do a complete specialization of AllocatorInstance,
     * as the logic governing creation and destruction of allocators is complicated and
     * susceptible to edge cases across all platforms and build types, and it is best to
     * keep the allocator code flowing through a consistent codepath.
     */
    template<class Allocator>
    class AllocatorInstance : public Internal::AllocatorInstanceBase<Allocator, AllocatorStorage::EnvironmentStoragePolicy<Allocator>>
    {
    };

    // Schema which acts as a pass through to another allocator. This allows for allocators
    // which exist purely to categorize/track memory separately, piggy backing on the
    // structure of another allocator
    template <class ParentAllocator>
    class ChildAllocatorSchema
        : public IAllocatorAllocate
    {
    public:
        // No descriptor is necessary, as the parent allocator is expected to already
        // be created and configured
        struct Descriptor {};
        using Parent = ParentAllocator;

        ChildAllocatorSchema(const Descriptor&) {}

        //---------------------------------------------------------------------
        // IAllocatorAllocate
        //---------------------------------------------------------------------
        pointer_type Allocate(size_type byteSize, size_type alignment, int flags = 0, const char* name = 0, const char* fileName = 0, int lineNum = 0, unsigned int suppressStackRecord = 0) override
        {
            return V::AllocatorInstance<Parent>::Get().Allocate(byteSize, alignment, flags, name, fileName, lineNum, suppressStackRecord);
        }

        void DeAllocate(pointer_type ptr, size_type byteSize = 0, size_type alignment = 0) override
        {
            V::AllocatorInstance<Parent>::Get().DeAllocate(ptr, byteSize, alignment);
        }

        size_type Resize(pointer_type ptr, size_type newSize) override
        {
            return V::AllocatorInstance<Parent>::Get().Resize(ptr, newSize);
        }

        pointer_type ReAllocate(pointer_type ptr, size_type newSize, size_type newAlignment) override
        {
            return V::AllocatorInstance<Parent>::Get().ReAllocate(ptr, newSize, newAlignment);
        }

        size_type AllocationSize(pointer_type ptr) override
        {
            return V::AllocatorInstance<Parent>::Get().AllocationSize(ptr);
        }

        void GarbageCollect() override
        {
            V::AllocatorInstance<Parent>::Get().GarbageCollect();
        }

        size_type NumAllocatedBytes() const override
        {
            return V::AllocatorInstance<Parent>::Get().NumAllocatedBytes();
        }

        size_type Capacity() const override
        {
            return V::AllocatorInstance<Parent>::Get().Capacity();
        }

        size_type GetMaxAllocationSize() const override
        {
            return V::AllocatorInstance<Parent>::Get().GetMaxAllocationSize();
        }

        size_type GetMaxContiguousAllocationSize() const override
        {
            return V::AllocatorInstance<Parent>::Get().GetMaxContiguousAllocationSize();
        }

        size_type               GetUnAllocatedMemory(bool isPrint = false) const override
        {
            return V::AllocatorInstance<Parent>::Get().GetUnAllocatedMemory(isPrint);
        }

        IAllocatorAllocate*     GetSubAllocator() override
        {
            return V::AllocatorInstance<Parent>::Get().GetSubAllocator();
        }
    };

    /**
    * Generic wrapper for binding allocator to an VStd one.
    * \note VStd allocators are one of the few differences from STD/STL.
    * It's very tedious to write a wrapper for STD too.
    */
    template <class Allocator>
    class VStdAlloc
    {
    public:
        typedef void*               pointer_type;
        typedef VStd::size_t       size_type;
        typedef VStd::ptrdiff_t    difference_type;
        typedef VStd::false_type   allow_memory_leaks;         ///< Regular allocators should not leak.

        V_FORCE_INLINE VStdAlloc()
        {
            if (AllocatorInstance<Allocator>::IsReady())
            {
                m_name = AllocatorInstance<Allocator>::GetAllocator().GetName();
            }
            else
            {
                m_name = AzTypeInfo<Allocator>::Name();
            }
        }
        V_FORCE_INLINE VStdAlloc(const char* name)
            : m_name(name)     {}
        V_FORCE_INLINE VStdAlloc(const VStdAlloc& rhs)
            : m_name(rhs.m_name)  {}
        V_FORCE_INLINE VStdAlloc(const VStdAlloc& rhs, const char* name)
            : m_name(name) { (void)rhs; }
        V_FORCE_INLINE VStdAlloc& operator=(const VStdAlloc& rhs) { m_name = rhs.m_name; return *this; }
        V_FORCE_INLINE pointer_type allocate(size_t byteSize, size_t alignment, int flags = 0)
        {
            return AllocatorInstance<Allocator>::Get().Allocate(byteSize, alignment, flags, m_name, __FILE__, __LINE__, 1);
        }
        V_FORCE_INLINE size_type resize(pointer_type ptr, size_type newSize)
        {
            return AllocatorInstance<Allocator>::Get().Resize(ptr, newSize);
        }
        V_FORCE_INLINE void deallocate(pointer_type ptr, size_type byteSize, size_type alignment)
        {
            AllocatorInstance<Allocator>::Get().DeAllocate(ptr, byteSize, alignment);
        }
        V_FORCE_INLINE const char* get_name() const            { return m_name; }
        V_FORCE_INLINE void        set_name(const char* name)  { m_name = name; }
        size_type                   max_size() const            { return AllocatorInstance<Allocator>::Get().GetMaxContiguousAllocationSize(); }
        size_type                   get_allocated_size() const  { return AllocatorInstance<Allocator>::Get().NumAllocatedBytes(); }

        V_FORCE_INLINE bool is_lock_free()                     { return AllocatorInstance<Allocator>::Get().is_lock_free(); }
        V_FORCE_INLINE bool is_stale_read_allowed()            { return AllocatorInstance<Allocator>::Get().is_stale_read_allowed(); }
        V_FORCE_INLINE bool is_delayed_recycling()             { return AllocatorInstance<Allocator>::Get().is_delayed_recycling(); }
    private:
        const char* m_name;
    };

    template<class Allocator>
    V_FORCE_INLINE bool operator==(const VStdAlloc<Allocator>& a, const VStdAlloc<Allocator>& b) { (void)a; (void)b; return true; } // always true since they use the same instance of AllocatorInstance<Allocator>
    template<class Allocator>
    V_FORCE_INLINE bool operator!=(const VStdAlloc<Allocator>& a, const VStdAlloc<Allocator>& b) { (void)a; (void)b; return false; } // always false since they use the same instance of AllocatorInstance<Allocator>

    /**
     * Generic wrapper for binding IAllocator interface allocator.
     * This is basically the same as \ref VStdAlloc but it allows
     * you to remove the template parameter and set you interface on demand.
     * of course at a cost of a pointer.
     */
    class VStdIAllocator
    {
    public:
        typedef void*               pointer_type;
        typedef VStd::size_t       size_type;
        typedef VStd::ptrdiff_t    difference_type;
        typedef VStd::false_type   allow_memory_leaks;         ///< Regular allocators should not leak.

        V_FORCE_INLINE VStdIAllocator(IAllocatorAllocate* allocator, const char* name = "V::VStdIAllocator")
            : m_allocator(allocator)
            , m_name(name)
        {
            V_Assert(m_allocator != NULL, "You must provide a valid allocator!");
        }
        V_FORCE_INLINE VStdIAllocator(const VStdIAllocator& rhs)
            : m_allocator(rhs.m_allocator)
            , m_name(rhs.m_name)  {}
        V_FORCE_INLINE VStdIAllocator(const VStdIAllocator& rhs, const char* name)
            : m_allocator(rhs.m_allocator)
            , m_name(name) { (void)rhs; }
        V_FORCE_INLINE VStdIAllocator& operator=(const VStdIAllocator& rhs) { m_allocator = rhs.m_allocator; m_name = rhs.m_name; return *this; }
        V_FORCE_INLINE pointer_type allocate(size_t byteSize, size_t alignment, int flags = 0)
        {
            return m_allocator->Allocate(byteSize, alignment, flags, m_name, __FILE__, __LINE__, 1);
        }
        V_FORCE_INLINE size_type resize(pointer_type ptr, size_t newSize)
        {
            return m_allocator->Resize(ptr, newSize);
        }
        V_FORCE_INLINE void deallocate(pointer_type ptr, size_t byteSize, size_t alignment)
        {
            m_allocator->DeAllocate(ptr, byteSize, alignment);
        }
        V_FORCE_INLINE const char* get_name() const { return m_name; }
        V_FORCE_INLINE void        set_name(const char* name) { m_name = name; }
        size_type                   max_size() const { return m_allocator->GetMaxContiguousAllocationSize(); }
        size_type                   get_allocated_size() const { return m_allocator->NumAllocatedBytes(); }

        V_FORCE_INLINE bool operator==(const VStdIAllocator& rhs) const { return m_allocator == rhs.m_allocator; }
        V_FORCE_INLINE bool operator!=(const VStdIAllocator& rhs) const { return m_allocator != rhs.m_allocator; }
    private:
        IAllocatorAllocate* m_allocator;
        const char* m_name;
    };

    /**
    * Generic wrapper for binding IAllocator interface allocator.
    * This is basically the same as \ref VStdAlloc but it allows
    * you to remove the template parameter and retrieve the allocator from a supplied function
    * pointer
    */
    class VStdFunctorAllocator
    {
    public:
        using pointer_type = void*;
        using size_type = VStd::size_t;
        using difference_type = VStd::ptrdiff_t;
        using allow_memory_leaks = VStd::false_type; ///< Regular allocators should not leak.
        using functor_type = IAllocatorAllocate&(*)(); ///< Function Pointer must return IAllocatorAllocate&.
                                                       ///< function pointers do not support covariant return types

        constexpr VStdFunctorAllocator(functor_type allocatorFunctor, const char* name = "V::VStdFunctorAllocator")
            : m_allocatorFunctor(allocatorFunctor)
            , m_name(name)
        {
        }
        constexpr VStdFunctorAllocator(const VStdFunctorAllocator& rhs, const char* name)
            : m_allocatorFunctor(rhs.m_allocatorFunctor)
            , m_name(name)
        {
        }
        constexpr VStdFunctorAllocator(const VStdFunctorAllocator& rhs) = default;
        constexpr VStdFunctorAllocator& operator=(const VStdFunctorAllocator& rhs) = default;
        pointer_type allocate(size_t byteSize, size_t alignment, int flags = 0)
        {
            return m_allocatorFunctor().Allocate(byteSize, alignment, flags, m_name, __FILE__, __LINE__, 1);
        }
        size_type resize(pointer_type ptr, size_t newSize)
        {
            return m_allocatorFunctor().Resize(ptr, newSize);
        }
        void deallocate(pointer_type ptr, size_t byteSize, size_t alignment)
        {
            m_allocatorFunctor().DeAllocate(ptr, byteSize, alignment);
        }
        constexpr const char* get_name() const { return m_name; }
        void set_name(const char* name) { m_name = name; }
        size_type max_size() const { return m_allocatorFunctor().GetMaxContiguousAllocationSize(); }
        size_type get_allocated_size() const { return m_allocatorFunctor().NumAllocatedBytes(); }

        constexpr bool operator==(const VStdFunctorAllocator& rhs) const { return m_allocatorFunctor == rhs.m_allocatorFunctor; }
        constexpr bool operator!=(const VStdFunctorAllocator& rhs) const { return m_allocatorFunctor != rhs.m_allocatorFunctor; }
    private:
        functor_type m_allocatorFunctor;
        const char* m_name;
    };

    /**
    * Helper class to determine if type T has a V_CLASS_ALLOCATOR defined,
    * so we can safely call vnew on it. -  VClassAllocator<ClassType>....
    */
    V_HAS_MEMBER(VClassAllocator, V_CLASS_ALLOCATOR_DECLARED, void, ());

   


    // {@ Global New/Delete Operators
    [[nodiscard]] void* OperatorNew(std::size_t size, const char* fileName, int lineNum, const char* name = nullptr);
    [[nodiscard]] void* OperatorNew(std::size_t size);
    void OperatorDelete(void* ptr);
    void OperatorDelete(void* ptr, std::size_t size);

    [[nodiscard]] void* OperatorNewArray(std::size_t size, const char* fileName, int lineNum, const char* name = nullptr);
    [[nodiscard]] void* OperatorNewArray(std::size_t size);
    void OperatorDeleteArray(void* ptr);
    void OperatorDeleteArray(void* ptr, std::size_t size);

#if __cpp_aligned_new
    [[nodiscard]] void* OperatorNew(std::size_t size, std::align_val_t align, const char* fileName, int lineNum, const char* name = nullptr);
    [[nodiscard]] void* OperatorNew(std::size_t size, std::align_val_t align);
    [[nodiscard]] void* OperatorNewArray(std::size_t size, std::align_val_t align, const char* fileName, int lineNum, const char* name = nullptr);
    [[nodiscard]] void* OperatorNewArray(std::size_t size, std::align_val_t align);
    void OperatorDelete(void* ptr, std::size_t size, std::align_val_t align);
    void OperatorDeleteArray(void* ptr, std::size_t size, std::align_val_t align);
#endif
    // @}
}

#define V_PAGE_SIZE V_TRAIT_OS_DEFAULT_PAGE_SIZE
#define V_DEFAULT_ALIGNMENT (sizeof(void*))

// define unlimited allocator limits (scaled to real number when we check if there is enough memory to allocate)
#define V_CORE_MAX_ALLOCATOR_SIZE V_TRAIT_OS_MEMORY_MAX_ALLOCATOR_SIZE


#endif // V_FRAMEWORK_CORE_MEMORY_MEMMORY_H