/*
 * Copyright (c) Contributors to the VelcroFramework.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 * 
 */
#ifndef V_FRAMEWORK_CORE_STD_ALLOCATOR_H
#define V_FRAMEWORK_CORE_STD_ALLOCATOR_H

#include <vcore/std/base.h>
#include <vcore/std/typetraits/integral_constant.h>

namespace VStd {
    class allocator {
        public:
            typedef void*              pointer_type;
            typedef VStd::size_t       size_type;
            typedef VStd::ptrdiff_t    difference_type;
            typedef VStd::false_type   allow_memory_leaks;

            V_FORCE_INLINE allocator(const char* name = "VStd::allocator")
                : m_name(name) {}
            V_FORCE_INLINE allocator(const allocator& rhs)
                : m_name(rhs.m_name)    {}
            V_FORCE_INLINE allocator(const allocator& rhs, const char* name)
                : m_name(name) { (void)rhs; }

            V_FORCE_INLINE allocator& operator=(const allocator& rhs)      { m_name = rhs.m_name; return *this; }

            V_FORCE_INLINE const char*  get_name() const                   { return m_name; }
            V_FORCE_INLINE void         set_name(const char* name)         { m_name = name; }

            pointer_type    allocate(size_type byteSize, size_type alignment, int flags = 0);
            void            deallocate(pointer_type ptr, size_type byteSize, size_type alignment);
            size_type       resize(pointer_type ptr, size_type newSize);
            // max_size actually returns the true maximum size of a single allocation
            size_type       max_size() const;
            size_type       get_allocated_size() const;

            V_FORCE_INLINE bool is_lock_free()                             { return false; }
            V_FORCE_INLINE bool is_stale_read_allowed()                    { return false; }
            V_FORCE_INLINE bool is_delayed_recycling()                     { return false; }
        private:
            const char* m_name;
    };

    V_FORCE_INLINE bool operator==(const VStd::allocator& a, const VStd::allocator& b) {
        (void)a;
        (void)b;
        return true;
    }

    V_FORCE_INLINE bool operator!=(const VStd::allocator& a, const VStd::allocator& b) {
        (void)a;
        (void)b;
        return false;
    }

    /**
     * No Default allocator implementation (invalid allocator).
     * - If you want to make sure we don't use default allocator, define \ref VStd::allocator to VStd::no_default_allocator (this allocator)
     *  the code which try to use it, will not compile.
     * 
     * 
     * - If you have a compile error here, this means that
     * you did not provide allocator to your container. This
     * is intentional so you make sure where do you allocate from. If this is not
     * the VStd integration this means that you might have defined in your
     * code a default allocator or even have predefined container types. Use them.
     */
    class no_default_allocator {
    public:
        typedef void*              pointer_type;
        typedef VStd::size_t       size_type;
        typedef VStd::ptrdiff_t    difference_type;
        typedef VStd::false_type   allow_memory_leaks;

        V_FORCE_INLINE no_default_allocator(const char* name = "Invalid allocator") { (void)name; }
        V_FORCE_INLINE no_default_allocator(const allocator&) {}
        V_FORCE_INLINE no_default_allocator(const allocator&, const char*) {}

        // none of this functions are implemented we should get a link error if we use them!
        V_FORCE_INLINE allocator& operator=(const allocator& rhs);
        V_FORCE_INLINE pointer_type allocate(size_type byteSize, size_type alignment, int flags = 0);
        V_FORCE_INLINE void  deallocate(pointer_type ptr, size_type byteSize, size_type alignment);
        V_FORCE_INLINE size_type    resize(pointer_type ptr, size_type newSize);

        V_FORCE_INLINE const char*  get_name() const;
        V_FORCE_INLINE void         set_name(const char* name);

        V_FORCE_INLINE size_type   max_size() const;

        V_FORCE_INLINE bool is_lock_free();
        V_FORCE_INLINE bool is_stale_read_allowed();
        V_FORCE_INLINE bool is_delayed_recycling();
    };
}

#endif // V_FRAMEWORK_CORE_STD_ALLOCATOR_H