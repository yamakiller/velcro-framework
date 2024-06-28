#ifndef V_FRAMEWORK_CORE_STD_PARALLEL_CONTAINERS_CONCURRENT_FIXED_UNORDERED_SET_H
#define V_FRAMEWORK_CORE_STD_PARALLEL_CONTAINERS_CONCURRENT_FIXED_UNORDERED_SET_H

#include <vcore/std/parallel/containers/internal/concurrent_hash_table.h>

namespace VStd
{
    namespace Internal
    {
        template<class Key, class Hasher, class EqualKey, bool isMultiSet, VStd::size_t FixedNumBuckets, VStd::size_t FixedNumElements, VStd::size_t NumLocks>
        struct ConcurrentUnorderedFixedSetTableTraits
        {
            typedef Key         key_type;
            typedef EqualKey    key_eq;
            typedef Hasher      hasher;
            typedef Key         value_type;
            typedef VStd::no_default_allocator allocator_type;
            enum
            {
                has_multi_elements = isMultiSet,
                is_dynamic = false,
                fixed_num_buckets = FixedNumBuckets, // a prime number between 1/3 - 1/4 of the number of elements.
                fixed_num_elements = FixedNumElements,
                num_locks = NumLocks
            };
            static V_FORCE_INLINE const key_type& key_from_value(const value_type& value)  { return value; }
        };
    } // namespace Internal

    /**
     * Concurrent fixed unordered set container, similar to fixed_unordered_set, but uses a striped array of locks to
     * allow concurrent access.
     */
    template<class Key, VStd::size_t FixedNumBuckets, VStd::size_t FixedNumElements, VStd::size_t NumLocks = 8, class Hasher = VStd::hash<Key>, class EqualKey = VStd::equal_to<Key> >
    class concurrent_fixed_unordered_set
        : public Internal::concurrent_hash_table< Internal::ConcurrentUnorderedFixedSetTableTraits<Key, Hasher, EqualKey, false, FixedNumBuckets, FixedNumElements, NumLocks> >
    {
        enum
        {
            CONTAINER_VERSION = 1
        };

        typedef concurrent_fixed_unordered_set<Key, FixedNumBuckets, FixedNumElements, NumLocks, Hasher, EqualKey> this_type;
        typedef Internal::concurrent_hash_table< Internal::ConcurrentUnorderedFixedSetTableTraits<Key, Hasher, EqualKey, false, FixedNumBuckets, FixedNumElements, NumLocks> > base_type;
    public:
        typedef typename base_type::key_type    key_type;
        typedef typename base_type::key_eq      key_eq;
        typedef typename base_type::hasher      hasher;

        typedef typename base_type::allocator_type              allocator_type;
        typedef typename base_type::size_type                   size_type;
        typedef typename base_type::difference_type             difference_type;
        typedef typename base_type::pointer                     pointer;
        typedef typename base_type::const_pointer               const_pointer;
        typedef typename base_type::reference                   reference;
        typedef typename base_type::const_reference             const_reference;

        typedef typename base_type::value_type                  value_type;

        V_FORCE_INLINE concurrent_fixed_unordered_set()
            : base_type(hasher(), key_eq()) {}
        V_FORCE_INLINE concurrent_fixed_unordered_set(const hasher& hash, const key_eq& keyEqual)
            : base_type(hash, keyEqual)
        {}
        template<class Iterator>
        V_FORCE_INLINE concurrent_fixed_unordered_set(Iterator first, Iterator last)
            : base_type(hasher(), key_eq())
        {
            for (; first != last; ++first)
            {
                insert(*first);
            }
        }
        template<class Iterator>
        V_FORCE_INLINE concurrent_fixed_unordered_set(Iterator first, Iterator last, const hasher& hash, const key_eq& keyEqual)
            : base_type(hash, keyEqual)
        {
            for (; first != last; ++first)
            {
                insert(*first);
            }
        }
    };

    template<class Key, VStd::size_t FixedNumBuckets, VStd::size_t FixedNumElements, VStd::size_t NumLocks, class Hasher, class EqualKey >
    V_FORCE_INLINE void swap(concurrent_fixed_unordered_set<Key, FixedNumBuckets, FixedNumElements, NumLocks, Hasher, EqualKey>& left, concurrent_fixed_unordered_set<Key, FixedNumBuckets, FixedNumElements, NumLocks, Hasher, EqualKey>& right)
    {
        left.swap(right);
    }

    /**
     * Concurrent fixed unordered multiset container, similar to fixed_unordered_multiset, but uses a striped array of
     * locks to allow concurrent access.
     */
    template<class Key, VStd::size_t FixedNumBuckets, VStd::size_t FixedNumElements, VStd::size_t NumLocks = 8, class Hasher = VStd::hash<Key>, class EqualKey = VStd::equal_to<Key> >
    class concurrent_fixed_unordered_multiset
        : public Internal::concurrent_hash_table< Internal::ConcurrentUnorderedFixedSetTableTraits<Key, Hasher, EqualKey, true, FixedNumBuckets, FixedNumElements, NumLocks> >
    {
        enum
        {
            CONTAINER_VERSION = 1
        };

        typedef concurrent_fixed_unordered_multiset<Key, FixedNumBuckets, FixedNumElements, NumLocks, Hasher, EqualKey> this_type;
        typedef Internal::concurrent_hash_table< Internal::ConcurrentUnorderedFixedSetTableTraits<Key, Hasher, EqualKey, true, FixedNumBuckets, FixedNumElements, NumLocks> > base_type;
    public:
        typedef typename base_type::key_type    key_type;
        typedef typename base_type::key_eq      key_eq;
        typedef typename base_type::hasher      hasher;

        typedef typename base_type::allocator_type              allocator_type;
        typedef typename base_type::size_type                   size_type;
        typedef typename base_type::difference_type             difference_type;
        typedef typename base_type::pointer                     pointer;
        typedef typename base_type::const_pointer               const_pointer;
        typedef typename base_type::reference                   reference;
        typedef typename base_type::const_reference             const_reference;

        typedef typename base_type::value_type                  value_type;

        V_FORCE_INLINE concurrent_fixed_unordered_multiset()
            : base_type(hasher(), key_eq()) {}
        V_FORCE_INLINE concurrent_fixed_unordered_multiset(const hasher& hash, const key_eq& keyEqual)
            : base_type(hash, keyEqual)
        {}
        template<class Iterator>
        V_FORCE_INLINE concurrent_fixed_unordered_multiset(Iterator first, Iterator last)
            : base_type(hasher(), key_eq())
        {
            for (; first != last; ++first)
            {
                insert(*first);
            }
        }
        template<class Iterator>
        V_FORCE_INLINE concurrent_fixed_unordered_multiset(Iterator first, Iterator last, const hasher& hash, const key_eq& keyEqual)
            : base_type(hash, keyEqual)
        {
            for (; first != last; ++first)
            {
                insert(*first);
            }
        }
    };

    template<class Key, VStd::size_t FixedNumBuckets, VStd::size_t FixedNumElements, VStd::size_t NumLocks, class Hasher, class EqualKey >
    V_FORCE_INLINE void swap(concurrent_fixed_unordered_multiset<Key, FixedNumBuckets, FixedNumElements, NumLocks, Hasher, EqualKey>& left, concurrent_fixed_unordered_multiset<Key, FixedNumBuckets, FixedNumElements, NumLocks, Hasher, EqualKey>& right)
    {
        left.swap(right);
    }
} // namespace VStd

#endif // V_FRAMEWORK_CORE_STD_PARALLEL_CONTAINERS_CONCURRENT_FIXED_UNORDERED_SET_H