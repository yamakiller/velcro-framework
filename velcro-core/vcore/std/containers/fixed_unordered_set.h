#ifndef V_FRAMEWORK_CORE_STD_CONTAINERS_FIXED_UNORDERED_SET_H
#define V_FRAMEWORK_CORE_STD_CONTAINERS_FIXED_UNORDERED_SET_H

#include <vcore/std/hash_table.h>

namespace VStd
{
    namespace Internal
    {
        template<class Key, class Hasher, class EqualKey, bool isMultiSet, VStd::size_t FixedNumBuckets, VStd::size_t FixedNumElements>
        struct UnorderedFixedSetTableTraits
        {
            typedef Key         key_type;
            typedef EqualKey    key_eq;
            typedef Hasher      hasher;
            typedef Key         value_type;
            typedef VStd::no_default_allocator allocator_type;
            enum
            {
                max_load_factor = 4,
                min_buckets = 7,
                has_multi_elements = isMultiSet,
                is_dynamic         = false,
                fixed_num_buckets  = FixedNumBuckets, // a prime number between 1/3 - 1/4 of the number of elements.
                fixed_num_elements = FixedNumElements
            };
            static V_FORCE_INLINE const key_type& key_from_value(const value_type& value)  { return value; }
        };
    }

    /**
    * Fixed unordered set container is based on unordered set and complaint with \ref CTR1 (6.2.4.3).
    * This is an associative container, all Keys are unique.
    * insert function will return false, if you try to add key that is in the set.
    * It has the extensions in the \ref hash_table class.
    * No allocations will occur using this container. For optimal performance the provided FixedNumBuckets should be
    * a prime number and about 1/3-1/4 of the FixedNumElements.
    *
    * Check the unordered_set \ref VStdExamples.
    *
    * \note By default we don't have reverse iterators for unordered_xxx containers. This saves us 4 bytes per node and it's not used
    * in the majority of cases. Reverse iterators are supported via the last template parameter.
    */
    template<class Key, VStd::size_t FixedNumBuckets, VStd::size_t FixedNumElements, class Hasher = VStd::hash<Key>, class EqualKey = VStd::equal_to<Key> >
    class fixed_unordered_set
        : public hash_table< Internal::UnorderedFixedSetTableTraits<Key, Hasher, EqualKey, false, FixedNumBuckets, FixedNumElements> >
    {
        enum
        {
            CONTAINER_VERSION = 1
        };

        typedef fixed_unordered_set<Key, FixedNumBuckets, FixedNumElements, Hasher, EqualKey> this_type;
        typedef hash_table< Internal::UnorderedFixedSetTableTraits<Key, Hasher, EqualKey, false, FixedNumBuckets, FixedNumElements> > base_type;
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

        typedef typename base_type::iterator                    iterator;
        typedef typename base_type::const_iterator              const_iterator;

        //typedef typename base_type::reverse_iterator          reverse_iterator;
        //typedef typename base_type::const_reverse_iterator    const_reverse_iterator;

        typedef typename base_type::value_type                  value_type;

        typedef typename base_type::local_iterator              local_iterator;
        typedef typename base_type::const_local_iterator        const_local_iterator;

        V_FORCE_INLINE fixed_unordered_set()
            : base_type(hasher(), key_eq()) {}
        V_FORCE_INLINE fixed_unordered_set(const hasher& hash, const key_eq& keyEqual)
            : base_type(hash, keyEqual)
        {}
        template<class Iterator>
        fixed_unordered_set(Iterator first, Iterator last)
            : base_type(hasher(), key_eq())
        {
            for (; first != last; ++first)
            {
                base_type::insert(*first);
            }
        }
        template<class Iterator>
        fixed_unordered_set(Iterator first, Iterator last, const hasher& hash, const key_eq& keyEqual)
            : base_type(hash, keyEqual)
        {
            for (; first != last; ++first)
            {
                base_type::insert(*first);
            }
        }
    };

    template<class Key, VStd::size_t FixedNumBuckets, VStd::size_t FixedNumElements, class Hasher, class EqualKey >
    V_FORCE_INLINE void swap(fixed_unordered_set<Key, FixedNumBuckets, FixedNumElements, Hasher, EqualKey>& left, fixed_unordered_set<Key, FixedNumBuckets, FixedNumElements, Hasher, EqualKey>& right)
    {
        left.swap(right);
    }

    /**
    * Fixed unordered multi set container is based on unordered multiset and complaint with \ref CTR1 (6.2.4.5)
    * The only difference from the unordered_set is that we allow multiple entries with
    * the same key. You can iterate over them, by checking if the key stays the same.
    * No allocations will occur using this container. For optimal performance the provided FixedNumBuckets should be
    * a prime number and about 1/3-1/4 of the FixedNumElements.
    *
    * Check the unordered_multiset \ref VStdExamples.
    *
    * \note By default we don't have reverse iterators for unordered_xxx containers. This saves us 4 bytes per node and it's not used
    * in the majority of cases. Reverse iterators are supported via the last template parameter.
    */
    template<class Key, VStd::size_t FixedNumBuckets, VStd::size_t FixedNumElements, class Hasher = VStd::hash<Key>, class EqualKey = VStd::equal_to<Key> >
    class fixed_unordered_multiset
        : public hash_table< Internal::UnorderedFixedSetTableTraits<Key, Hasher, EqualKey, true, FixedNumBuckets, FixedNumElements> >
    {
        enum
        {
            CONTAINER_VERSION = 1
        };

        typedef fixed_unordered_multiset<Key, FixedNumBuckets, FixedNumElements, Hasher, EqualKey> this_type;
        typedef hash_table< Internal::UnorderedFixedSetTableTraits<Key, Hasher, EqualKey, true, FixedNumBuckets, FixedNumElements> > base_type;
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

        typedef typename base_type::iterator                    iterator;
        typedef typename base_type::const_iterator              const_iterator;

        //typedef typename base_type::reverse_iterator          reverse_iterator;
        //typedef typename base_type::const_reverse_iterator        const_reverse_iterator;

        typedef typename base_type::value_type                  value_type;

        typedef typename base_type::local_iterator              local_iterator;
        typedef typename base_type::const_local_iterator        const_local_iterator;

        V_FORCE_INLINE fixed_unordered_multiset()
            : base_type(hasher(), key_eq()) {}
        V_FORCE_INLINE fixed_unordered_multiset(const hasher& hash, const key_eq& keyEqual)
            : base_type(hash, keyEqual)
        {}
        template<class Iterator>
        fixed_unordered_multiset(Iterator first, Iterator last)
            : base_type(hasher(), key_eq())
        {
            for (; first != last; ++first)
            {
                base_type::insert(*first);
            }
        }
        template<class Iterator>
        fixed_unordered_multiset(Iterator first, Iterator last, const hasher& hash, const key_eq& keyEqual)
            : base_type(hash, keyEqual)
        {
            for (; first != last; ++first)
            {
                base_type::insert(*first);
            }
        }
    };

    template<class Key, VStd::size_t FixedNumBuckets, VStd::size_t FixedNumElements, class Hasher, class EqualKey >
    V_FORCE_INLINE void swap(fixed_unordered_multiset<Key, FixedNumBuckets, FixedNumElements, Hasher, EqualKey>& left, fixed_unordered_multiset<Key, FixedNumBuckets, FixedNumElements, Hasher, EqualKey>& right)
    {
        left.swap(right);
    }
}

#endif // V_FRAMEWORK_CORE_STD_CONTAINERS_FIXED_UNORDERED_SET_H