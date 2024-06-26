#ifndef V_FRAMEWORK_CORE_NAME_INTERNAL_NAME_DATA_H
#define V_FRAMEWORK_CORE_NAME_INTERNAL_NAME_DATA_H

#include <vcore/memory/system_allocator.h>
#include <vcore/std/parallel/atomic.h>
#include <vcore/std/string/string.h>
#include <vcore/std/string/string_view.h>
#include <vcore/std/smart_ptr/intrusive_ptr.h>

namespace V {
    class NameDictionary;

    namespace Internal {
        class NameData final
        {
            friend class NameDictionary;
            template <typename T>
            friend struct VStd::IntrusivePtrCountPolicy;
        public:
            V_CLASS_ALLOCATOR(NameData, V::SystemAllocator, 0);

            using Hash = uint32_t; // We use a 32 bit hash especially for efficient transport over a network.

            //! Returns the string part of the name data.
            VStd::string_view GetName() const;

            //! Returns the hash part of the name data.
            Hash GetHash() const;

        private:
            NameData(VStd::string&& name, Hash hash);

            void add_ref();
            void release();

        private:
            VStd::atomic_int m_useCount = {0};
            VStd::string m_name;
            Hash m_hash;

            VStd::atomic<bool> m_hashCollision = false; // Tracks whether the hash has been involved in a collision
        };
    }
}

#endif // V_FRAMEWORK_CORE_NAME_INTERNAL_NAME_DATA_H