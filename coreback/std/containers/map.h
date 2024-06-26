/*
 * Copyright (c) Contributors to the VelcroFramework Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#ifndef V_FRAMEWORK_CORE_STD_ORDERED_MAP_H
#define V_FRAMEWORK_CORE_STD_ORDERED_MAP_H 1

#include <vcore/std/containers/rbtree.h>
#include <vcore/std/containers/node_handle.h>
#include <vcore/std/functional_basic.h>
#include <vcore/std/algorithm.h>
#include <vcore/std/tuple.h>

namespace VStd
{
    namespace Internal
    {
        template<class Key, class MappedType, class KeyEq, class Allocator >
        struct OrderedMapRbTreeTraits
        {
            typedef Key                         key_type;
            typedef KeyEq                       key_eq;
            typedef VStd::pair<Key, MappedType> value_type;
            typedef Allocator                   allocator_type;
            static V_FORCE_INLINE const key_type& key_from_value(const value_type& value)  { return value.first;   }
        };
    }

    /**
    * Ordered map container is complaint with \ref C++0x (23.4.1)
    * This is an associative container, all keys are unique.
    * insert function will return false, if you try to add key that is in the set.
    *
    * Check the set \ref VStdExamples.
    */
    template<class Key, class MappedType, class Compare = VStd::less<Key>, class Allocator = VStd::allocator>
    class map
    {
        enum
        {
            CONTAINER_VERSION = 1
        };

        typedef map<Key, MappedType, Compare, Allocator> this_type;
        typedef rbtree< Internal::OrderedMapRbTreeTraits<Key, MappedType, Compare, Allocator> > tree_type;

    public:
        typedef typename tree_type::traits_type traits_type;

        typedef typename tree_type::key_type       key_type;
        typedef typename tree_type::value_type     value_type;
        typedef typename tree_type::key_eq         key_compare;
        typedef MappedType                         mapped_type;
        class value_compare
        {
        protected:
            Compare m_comp;
            V_FORCE_INLINE value_compare(Compare c)
                : m_comp(c) {}
        public:
            V_FORCE_INLINE bool operator()(const value_type& x, const value_type& y) const { return m_comp(x.first, y.first); }
        };

        typedef typename tree_type::allocator_type              allocator_type;
        typedef typename tree_type::size_type                   size_type;
        typedef typename tree_type::difference_type             difference_type;
        typedef typename tree_type::pointer                     pointer;
        typedef typename tree_type::const_pointer               const_pointer;
        typedef typename tree_type::reference                   reference;
        typedef typename tree_type::const_reference             const_reference;

        typedef typename tree_type::iterator                    iterator;
        typedef typename tree_type::const_iterator              const_iterator;

        typedef typename tree_type::reverse_iterator            reverse_iterator;
        typedef typename tree_type::const_reverse_iterator      const_reverse_iterator;
        
        using node_type = map_node_handle<map_node_traits<key_type, mapped_type, allocator_type, typename tree_type::node_type, typename tree_type::node_deleter>>;
        using insert_return_type = insert_return_type<iterator, node_type>;

        V_FORCE_INLINE explicit map(const Compare& comp = Compare(), const Allocator& alloc = Allocator())
            : m_tree(comp, alloc) {}
        explicit map(const Allocator& alloc)
            : m_tree(alloc) {}
        template <class InputIterator>
        V_FORCE_INLINE map(InputIterator first, InputIterator last, const Compare& comp = Compare(), const Allocator& alloc = Allocator())
            : m_tree(comp, alloc)
        {
            m_tree.insert_unique(first, last);
        }
        V_FORCE_INLINE map(const this_type& rhs)
            : m_tree(rhs.m_tree)  {}
        V_FORCE_INLINE map(const this_type& rhs, const Allocator& alloc)
            : m_tree(rhs.m_tree, alloc)   {}
        V_FORCE_INLINE map(const std::initializer_list<value_type>& list, const Compare& comp = Compare(), const Allocator& alloc = Allocator())
            : m_tree(comp, alloc)
        {
            m_tree.insert_unique(list.begin(), list.end());
        }

        // Add move semantics...
        V_FORCE_INLINE this_type& operator=(const this_type& rhs) { m_tree = rhs.m_tree; return *this; }
        V_FORCE_INLINE key_compare key_comp() const        { return m_tree.key_comp(); }
        V_FORCE_INLINE value_compare value_comp() const    { return value_comp(m_tree.key_comp()); }

        V_FORCE_INLINE iterator begin()                    { return m_tree.begin(); }
        V_FORCE_INLINE iterator end()                      { return m_tree.end(); }
        V_FORCE_INLINE const_iterator begin() const        { return m_tree.begin(); }
        V_FORCE_INLINE const_iterator end() const          { return m_tree.end(); }
        V_FORCE_INLINE reverse_iterator rbegin()           { return m_tree.rbegin(); }
        V_FORCE_INLINE reverse_iterator rend()             { return m_tree.rend(); }
        V_FORCE_INLINE const_reverse_iterator rbegin() const { return m_tree.rbegin(); }
        V_FORCE_INLINE const_reverse_iterator rend() const { return m_tree.rend(); }
        V_FORCE_INLINE bool empty() const                  { return m_tree.empty(); }
        V_FORCE_INLINE size_type size() const              { return m_tree.size(); }
        V_FORCE_INLINE size_type max_size() const          { return m_tree.max_size(); }
        MappedType& operator[](const key_type& key)
        {
            iterator iter = m_tree.lower_bound(key);
            if (iter == end() || key_comp()(key, (*iter).first))
            {
                iter = insert(iter, value_type(VStd::piecewise_construct, VStd::forward_as_tuple(VStd::move(key)), VStd::tuple<>{}));
            }
            return (*iter).second;
        }
        MappedType& operator[](key_type&& key)
        {
            iterator iter = m_tree.lower_bound(key);
            if (iter == end() || key_comp()(key, (*iter).first))
            {
                iter = insert(iter, value_type(VStd::piecewise_construct, VStd::forward_as_tuple(VStd::move(key)), VStd::tuple<>{}));
            }
            return (*iter).second;
        }
        V_FORCE_INLINE void swap(this_type& rhs)           { m_tree.swap(rhs.m_tree); }

        // insert/erase
        V_FORCE_INLINE pair<iterator, bool> insert(const value_type& value)             { return m_tree.insert_unique(value); }
        V_FORCE_INLINE iterator insert(const_iterator insertPos, const value_type& value)    { return m_tree.insert_unique(insertPos, value); }
        template <class InputIterator>
        V_FORCE_INLINE void insert(InputIterator first, InputIterator last)            { m_tree.insert_unique(first, last); }
        V_FORCE_INLINE iterator erase(const_iterator erasePos)                         { return m_tree.erase(erasePos); }
        V_FORCE_INLINE size_type erase(const key_type& key)                            { return m_tree.erase_unique(key); }
        V_FORCE_INLINE iterator erase(const_iterator first, const_iterator last)       { return m_tree.erase(first, last); }
        V_FORCE_INLINE void clear() { m_tree.clear(); }

        map(this_type&& rhs)
            : m_tree(VStd::move(rhs.m_tree)) {}
        /* map(this_type&& rhs, const Allocator& alloc)
             : m_tree(VStd::move(rhs.m_tree), alloc){}*/
        this_type& operator=(this_type&& rhs)
        {
            if (this != &rhs)
            {
                clear();
                swap(rhs);
            }
            return *this;
        }
        VStd::pair<iterator, bool> insert(value_type&& value)
        {
            return m_tree.insert_unique(VStd::forward<value_type>(value));
        }
        iterator insert(const_iterator insertPos, value_type&& value)
        {
            return m_tree.insert_unique(insertPos, VStd::forward<value_type>(value));
        }
        template<class ... InputArguments>
        VStd::pair<iterator, bool> emplace(InputArguments&& ... arguments)
        {
            return m_tree.emplace_unique(VStd::forward<InputArguments>(arguments) ...);
        }
        template<class ... InputArguments>
        iterator emplace_hint(const_iterator insertPos, InputArguments&& ... arguments)
        {
            return m_tree.emplace_unique(insertPos, VStd::forward<InputArguments>(arguments) ...);
        }

        insert_return_type insert(node_type&& nodeHandle)
        {
            VSTD_CONTAINER_ASSERT(nodeHandle.empty() || nodeHandle.get_allocator() == get_allocator(),
                "node_type with incompatible allocator passed to map::insert(node_type&& nodeHandle)");
            return m_tree.template node_handle_insert_unique<insert_return_type>(VStd::move(nodeHandle));
        }
        iterator insert(const_iterator hint, node_type&& nodeHandle)
        {
            VSTD_CONTAINER_ASSERT(nodeHandle.empty() || nodeHandle.get_allocator() == get_allocator(),
                "node_type with incompatible allocator passed to map::insert(const_iterator hint, node_type&& node_handle)");
            return m_tree.node_handle_insert_unique(hint, VStd::move(nodeHandle));
        }

        //! C++17 insert_or_assign function assigns the element to the mapped_type if the key exist in the container
        //! Otherwise a new value is inserted into the container
        template <typename M>
        pair<iterator, bool> insert_or_assign(const key_type& key, M&& value)
        {
            return m_tree.insert_or_assign_unique(key, VStd::forward<M>(value));
        }
        template <typename M>
        pair<iterator, bool> insert_or_assign(key_type&& key, M&& value)
        {
            return m_tree.insert_or_assign_unique(VStd::move(key), VStd::forward<M>(value));
        }
        template <typename M>
        iterator insert_or_assign(const_iterator hint, const key_type& key, M&& value)
        {
            return m_tree.insert_or_assign_unique(hint, key, VStd::forward<M>(value));
        }
        template <typename M>
        iterator insert_or_assign(const_iterator hint, key_type&& key, M&& value)
        {
            return m_tree.insert_or_assign_unique(hint, VStd::move(key), VStd::forward<M>(value));
        }

        //! C++17 try_emplace function that does nothing to the arguments if the key exist in the container,
        //! otherwise it constructs the value type as if invoking
        //! value_type(VStd::piecewise_construct, VStd::forward_as_tuple(VStd::forward<KeyType>(key)),
        //!  VStd::forward_as_tuple(VStd::forward<Args>(args)...))
        template <typename... Args>
        pair<iterator, bool> try_emplace(const key_type& key, Args&&... arguments)
        {
            return m_tree.try_emplace_unique(key, VStd::forward<Args>(arguments)...);
        }
        template <typename... Args>
        pair<iterator, bool> try_emplace(key_type&& key, Args&&... arguments)
        {
            return m_tree.try_emplace_unique(VStd::move(key), VStd::forward<Args>(arguments)...);
        }
        template <typename... Args>
        iterator try_emplace(const_iterator hint, const key_type& key, Args&&... arguments)
        {
            return m_tree.try_emplace_unique(hint, key, VStd::forward<Args>(arguments)...);
        }
        template <typename... Args>
        iterator try_emplace(const_iterator hint, key_type&& key, Args&&... arguments)
        {
            return m_tree.try_emplace_unique(hint, VStd::move(key), VStd::forward<Args>(arguments)...);
        }

        node_type extract(const key_type& key)
        {
            return m_tree.template node_handle_extract<node_type>(key);
        }
        node_type extract(const_iterator it)
        {
            return m_tree.template node_handle_extract<node_type>(it);
        }

        // set operations:
        const_iterator find(const key_type& key) const          { return m_tree.find(key); }
        iterator find(const key_type& key)                      { return m_tree.find(key); }
        bool contains(const key_type& key) const                { return m_tree.contains(key); }
        size_type count(const key_type& key) const              { return m_tree.find(key) == m_tree.end() ? 0 : 1; }
        iterator lower_bound(const key_type& key)               { return m_tree.lower_bound(key); }
        const_iterator lower_bound(const key_type& key) const   { return m_tree.lower_bound(key); }
        iterator upper_bound(const key_type& key)               { return m_tree.upper_bound(key); }
        const_iterator upper_bound(const key_type& key) const   { return m_tree.upper_bound(key); }
        pair<iterator, iterator> equal_range(const key_type& key)                   { return m_tree.equal_range_unique(key); }
        pair<const_iterator, const_iterator> equal_range(const key_type& key) const { return m_tree.equal_range_unique(key); }

        template<typename ComparableToKey>
        auto find(const ComparableToKey& key) const -> enable_if_t<Internal::is_transparent<key_compare, ComparableToKey>::value, const_iterator> { return m_tree.find(key); }
        template<typename ComparableToKey>
        auto find(const ComparableToKey& key) -> enable_if_t<Internal::is_transparent<key_compare, ComparableToKey>::value, iterator> { return m_tree.find(key); }

        template<typename ComparableToKey>
        auto contains(const ComparableToKey& key) const -> enable_if_t<Internal::is_transparent<key_compare, ComparableToKey>::value, bool> { return m_tree.contains(key); }

        template<typename ComparableToKey>
        auto count(const ComparableToKey& key) const -> enable_if_t<Internal::is_transparent<key_compare, ComparableToKey>::value, size_type> { return m_tree.find(key) == m_tree.end() ? 0 : 1; }

        template<typename ComparableToKey>
        auto lower_bound(const ComparableToKey& key) -> enable_if_t<Internal::is_transparent<key_compare, ComparableToKey>::value, iterator> { return m_tree.lower_bound(key); }
        template<typename ComparableToKey>
        auto lower_bound(const ComparableToKey& key) const -> enable_if_t<Internal::is_transparent<key_compare, ComparableToKey>::value, const_iterator> { return m_tree.lower_bound(key); }

        template<typename ComparableToKey>
        auto upper_bound(const ComparableToKey& key) -> enable_if_t<Internal::is_transparent<key_compare, ComparableToKey>::value, iterator>{ return m_tree.upper_bound(key); }
        template<typename ComparableToKey>
        auto upper_bound(const ComparableToKey& key) const -> enable_if_t<Internal::is_transparent<key_compare, ComparableToKey>::value, const_iterator> { return m_tree.upper_bound(key); }

        template<typename ComparableToKey>
        auto equal_range(const ComparableToKey& key) -> enable_if_t<Internal::is_transparent<key_compare, ComparableToKey>::value, pair<iterator, iterator>> { return m_tree.equal_range_unique(key); }
        template<typename ComparableToKey>
        auto equal_range(const ComparableToKey& key) const -> enable_if_t<Internal::is_transparent<key_compare, ComparableToKey>::value, pair<const_iterator, const_iterator>> { return m_tree.equal_range_unique(key); }

        /**
        * \anchor ListExtensions
        * \name Extensions
        * @{
        */
        /**
         * insert and default construct an element \todo for this to make sense we need to add it to the rbtree container. To avoid any value type copy, similar to the hash_table.
         * this is here only to make sure it's available to the end user.
         */
        V_FORCE_INLINE pair<iterator, bool> insert(const key_type& key)
        {
            if (m_tree.find(key) != m_tree.end())
            {
                return VStd::make_pair(m_tree.end(), false);
            }
            return m_tree.insert_unique(value_type(key, MappedType() /* post pone the ctor to avoid any copy */));
        }
        // The only difference from the standard is that we return the allocator instance, not a copy.
        V_FORCE_INLINE allocator_type&         get_allocator()         { return m_tree.get_allocator(); }
        V_FORCE_INLINE const allocator_type&   get_allocator() const   { return m_tree.get_allocator(); }
        /// Set the vector allocator. If different than then current all elements will be reallocated.
        V_FORCE_INLINE void                    set_allocator(const allocator_type& allocator) { m_tree.set_allocator(allocator); }
        // Validate container status.
        V_INLINE bool      validate() const    { return m_tree.validate(); }
        /// Validates an iter iterator. Returns a combination of \ref iterator_status_flag.
        V_INLINE int       validate_iterator(const const_iterator& iter) const { return m_tree.validate_iterator(iter); }
        V_INLINE int       validate_iterator(const iterator& iter) const       { return m_tree.validate_iterator(iter); }

        /**
        * Resets the container without deallocating any memory or calling any destructor.
        * This function should be used when we need very quick tear down. Generally it's used for temporary vectors and we can just nuke them that way.
        * In addition the provided \ref Allocators, has leak and reset flag which will enable automatically this behavior. So this function should be used in
        * special cases \ref VStdExamples.
        * \note This function is added to the vector for consistency. In the vector case we have only one allocation, and if the allocator allows memory leaks
        * it can just leave deallocate function empty, which performance wise will be the same. For more complex containers this will make big difference.
        */
        V_FORCE_INLINE void                        leak_and_reset()    { m_tree.leak_and_reset(); }
        /// @}
    private:
        tree_type   m_tree;
    };

    template<class Key, class MappedType, class Compare, class Allocator>
    V_FORCE_INLINE void swap(map<Key, MappedType, Compare, Allocator>& left, map<Key, MappedType, Compare, Allocator>& right)
    {
        left.swap(right);
    }

    template<class Key, class MappedType, class Compare, class Allocator>
    inline bool operator==(const map<Key, MappedType, Compare, Allocator>& left, const map<Key, MappedType, Compare, Allocator>& right)
    {
        return (left.size() == right.size()
                && equal(left.begin(), left.end(), right.begin()));
    }

    template<class Key, class MappedType, class Compare, class Allocator>
    inline bool operator!=(const map<Key, MappedType, Compare, Allocator>& left, const map<Key, MappedType, Compare, Allocator>& right)
    {
        return (!(left == right));
    }

    template<class Key, class MappedType, class Compare, class Allocator>
    inline bool operator<(const map<Key, MappedType, Compare, Allocator>& left, const map<Key, MappedType, Compare, Allocator>& right)
    {
        return (lexicographical_compare(left.begin(), left.end(), right.begin(), right.end()));
    }

    template<class Key, class MappedType, class Compare, class Allocator>
    inline bool operator>(const map<Key, MappedType, Compare, Allocator>& left, const map<Key, MappedType, Compare, Allocator>& right)
    {
        return (right < left);
    }

    template<class Key, class MappedType, class Compare, class Allocator>
    inline bool operator<=(const map<Key, MappedType, Compare, Allocator>& left, const map<Key, MappedType, Compare, Allocator>& right)
    {
        return (!(right < left));
    }

    template<class Key, class MappedType, class Compare, class Allocator>
    inline bool operator>=(const map<Key, MappedType, Compare, Allocator>& left, const map<Key, MappedType, Compare, Allocator>& right)
    {
        return (!(left < right));
    }

    template<class Key, class MappedType, class Compare, class Allocator, class Predicate>
    decltype(auto) erase_if(map<Key, MappedType, Compare, Allocator>& container, Predicate predicate)
    {
        auto originalSize = container.size();

        for (auto iter = container.begin(); iter != container.end(); )
        {
            if (predicate(*iter))
            {
                iter = container.erase(iter);
            }
            else
            {
                ++iter;
            }
        }

        return originalSize - container.size();
    }

    /**
    * Ordered multimap container is complaint with \ref C++0x (23.4.2)
    * This is an associative container, key can be equivalent (multiple copies of the same key value).
    * You can iterate over them, by checking if the key stays the same, after finding the first.
    *
    * Check the set \ref VStdExamples.
    */
    template<class Key, class MappedType, class Compare = VStd::less<Key>, class Allocator = VStd::allocator>
    class multimap
    {
        enum
        {
            CONTAINER_VERSION = 1
        };

        typedef multimap<Key, MappedType, Compare, Allocator> this_type;
        typedef rbtree< Internal::OrderedMapRbTreeTraits<Key, MappedType, Compare, Allocator> > tree_type;
    public:
        typedef typename tree_type::traits_type traits_type;

        typedef typename tree_type::key_type       key_type;
        typedef typename tree_type::value_type     value_type;
        typedef typename tree_type::key_eq         key_compare;
        typedef MappedType                         mapped_type;
        class value_compare
        {
        protected:
            Compare m_comp;
            V_FORCE_INLINE value_compare(Compare c)
                : m_comp(c) {}
        public:
            V_FORCE_INLINE bool operator()(const value_type& x, const value_type& y) const { return m_comp(x.first, y.first); }
        };

        typedef typename tree_type::allocator_type              allocator_type;
        typedef typename tree_type::size_type                   size_type;
        typedef typename tree_type::difference_type             difference_type;
        typedef typename tree_type::pointer                     pointer;
        typedef typename tree_type::const_pointer               const_pointer;
        typedef typename tree_type::reference                   reference;
        typedef typename tree_type::const_reference             const_reference;

        typedef typename tree_type::iterator                    iterator;
        typedef typename tree_type::const_iterator              const_iterator;

        typedef typename tree_type::reverse_iterator            reverse_iterator;
        typedef typename tree_type::const_reverse_iterator      const_reverse_iterator;
        
        using node_type = map_node_handle<map_node_traits<key_type, mapped_type, allocator_type, typename tree_type::node_type, typename tree_type::node_deleter>>;

        V_FORCE_INLINE explicit multimap(const Compare& comp = Compare(), const Allocator& alloc = Allocator())
            : m_tree(comp, alloc) {}
        explicit multimap(const Allocator& alloc)
            : m_tree(alloc) {}
        template <class InputIterator>
        V_FORCE_INLINE multimap(InputIterator first, InputIterator last, const Compare& comp = Compare(), const Allocator& alloc = Allocator())
            : m_tree(comp, alloc)
        {
            m_tree.insert_equal(first, last);
        }
        V_FORCE_INLINE multimap(const this_type& rhs)
            : m_tree(rhs.m_tree) {}
        V_FORCE_INLINE multimap(const this_type& rhs, const Allocator& alloc)
            : m_tree(rhs.m_tree, alloc)   {}
        V_FORCE_INLINE multimap(const std::initializer_list<value_type>& list, const Compare& comp = Compare(), const Allocator& alloc = Allocator())
            : m_tree(comp, alloc)
        {
            m_tree.insert_equal(list.begin(), list.end());
        }

        // Add move semantics...
        V_FORCE_INLINE this_type& operator=(const this_type& rhs) { m_tree = rhs.m_tree; return *this; }
        V_FORCE_INLINE key_compare key_comp() const        { return m_tree.key_comp(); }
        V_FORCE_INLINE value_compare value_comp() const    { return value_compare(m_tree.key_comp()); }

        V_FORCE_INLINE iterator begin()                    { return m_tree.begin(); }
        V_FORCE_INLINE iterator end()                      { return m_tree.end(); }
        V_FORCE_INLINE const_iterator begin() const        { return m_tree.begin(); }
        V_FORCE_INLINE const_iterator end() const          { return m_tree.end(); }
        V_FORCE_INLINE reverse_iterator rbegin()           { return m_tree.rbegin(); }
        V_FORCE_INLINE reverse_iterator rend()             { return m_tree.rend(); }
        V_FORCE_INLINE const_reverse_iterator rbegin() const { return m_tree.rbegin(); }
        V_FORCE_INLINE const_reverse_iterator rend() const { return m_tree.rend(); }
        V_FORCE_INLINE bool empty() const                  { return m_tree.empty(); }
        V_FORCE_INLINE size_type size() const              { return m_tree.size(); }
        V_FORCE_INLINE size_type max_size() const          { return m_tree.max_size(); }
        V_FORCE_INLINE void swap(this_type& rhs)           { m_tree.swap(rhs.m_tree); }

        // insert/erase
        V_FORCE_INLINE pair<iterator, bool> insert(const value_type& value)             { return m_tree.insert_equal(value); }
        V_FORCE_INLINE iterator insert(const_iterator insertPos, const value_type& value)    { return m_tree.insert_equal(insertPos, value); }
        template <class InputIterator>
        V_FORCE_INLINE void insert(InputIterator first, InputIterator last)            { m_tree.insert_equal(first, last); }
        V_FORCE_INLINE iterator erase(const_iterator erasePos)                         { return m_tree.erase(erasePos); }
        V_FORCE_INLINE size_type erase(const key_type& key)                            { return m_tree.erase(key); }
        V_FORCE_INLINE iterator erase(const_iterator first, const_iterator last)       { return m_tree.erase(first, last); }
        V_FORCE_INLINE void clear() { m_tree.clear(); }

        multimap(this_type&& rhs)
            : m_tree(VStd::move(rhs.m_tree)) {}
        multimap(this_type&& rhs, const Allocator& alloc)
            : m_tree(VStd::move(rhs.m_tree), alloc){}
        this_type& operator=(this_type&& rhs)
        {
            if (this != &rhs)
            {
                clear();
                swap(rhs);
            }
            return *this;
        }
        VStd::pair<iterator, bool> insert(value_type&& value)
        {
            return m_tree.insert_equal(VStd::forward<value_type>(value));
        }

        iterator insert(const_iterator insertPos, value_type&& value)
        {
            return m_tree.insert_equal(insertPos, VStd::forward<value_type>(value));
        }

        template<class ... InputArguments>
        iterator emplace(InputArguments&& ... arguments)
        {
            return m_tree.emplace_equal(VStd::forward<InputArguments>(arguments) ...);
        }
        template<class ... InputArguments>
        iterator emplace_hint(const_iterator insertPos, InputArguments&& ... arguments)
        {
            return m_tree.emplace_equal(insertPos, VStd::forward<InputArguments>(arguments) ...);
        }

        iterator insert(node_type&& nodeHandle)
        {
            VSTD_CONTAINER_ASSERT(nodeHandle.empty() || nodeHandle.get_allocator() == get_allocator(),
                "node_type with incompatible allocator passed to multimap::insert(node_type&& nodeHandle)");
            return m_tree.node_handle_insert_equal(VStd::move(nodeHandle));
        }
        iterator insert(const_iterator hint, node_type&& nodeHandle)
        {
            VSTD_CONTAINER_ASSERT(nodeHandle.empty() || nodeHandle.get_allocator() == get_allocator(),
                "node_type with incompatible allocator passed to multimap::insert(const_iterator hint, node_type&& nodeHandle)");
            return m_tree.node_handle_insert_equal(hint, VStd::move(nodeHandle));
        }

        node_type extract(const key_type& key)
        {
            return m_tree.template node_handle_extract<node_type>(key);
        }
        node_type extract(const_iterator it)
        {
            return m_tree.template node_handle_extract<node_type>(it);
        }

        // set operations:
        const_iterator find(const key_type& key) const          { return m_tree.find(key); }
        iterator find(const key_type& key)                      { return m_tree.find(key); }
        bool contains(const key_type& key) const                { return m_tree.contains(key); }
        size_type count(const key_type& key) const              { return m_tree.count(key); }
        iterator lower_bound(const key_type& key)               { return m_tree.lower_bound(key); }
        const_iterator lower_bound(const key_type& key) const   { return m_tree.lower_bound(key); }
        iterator upper_bound(const key_type& key)               { return m_tree.upper_bound(key); }
        const_iterator upper_bound(const key_type& key) const   { return m_tree.upper_bound(key); }
        pair<iterator, iterator> equal_range(const key_type& key)                   { return m_tree.equal_range(key); }
        pair<const_iterator, const_iterator> equal_range(const key_type& key) const { return m_tree.equal_range(key); }

        template<typename ComparableToKey>
        auto find(const ComparableToKey& key) const -> enable_if_t<Internal::is_transparent<key_compare, ComparableToKey>::value, const_iterator> { return m_tree.find(key); }
        template<typename ComparableToKey>
        auto find(const ComparableToKey& key) -> enable_if_t<Internal::is_transparent<key_compare, ComparableToKey>::value, iterator> { return m_tree.find(key); }

        template<typename ComparableToKey>
        auto contains(const ComparableToKey& key) const -> enable_if_t<Internal::is_transparent<key_compare, ComparableToKey>::value, bool> { return m_tree.contains(key); }

        template<typename ComparableToKey>
        auto count(const ComparableToKey& key) const -> enable_if_t<Internal::is_transparent<key_compare, ComparableToKey>::value, size_type> { return m_tree.count(key); }

        template<typename ComparableToKey>
        auto lower_bound(const ComparableToKey& key) -> enable_if_t<Internal::is_transparent<key_compare, ComparableToKey>::value, iterator> { return m_tree.lower_bound(key); }
        template<typename ComparableToKey>
        auto lower_bound(const ComparableToKey& key) const -> enable_if_t<Internal::is_transparent<key_compare, ComparableToKey>::value, const_iterator> { return m_tree.lower_bound(key); }

        template<typename ComparableToKey>
        auto upper_bound(const ComparableToKey& key) -> enable_if_t<Internal::is_transparent<key_compare, ComparableToKey>::value, iterator> { return m_tree.upper_bound(key); }
        template<typename ComparableToKey>
        auto upper_bound(const ComparableToKey& key) const -> enable_if_t<Internal::is_transparent<key_compare, ComparableToKey>::value, const_iterator> { return m_tree.upper_bound(key); }

        template<typename ComparableToKey>
        auto equal_range(const ComparableToKey& key) -> enable_if_t<Internal::is_transparent<key_compare, ComparableToKey>::value, pair<iterator, iterator>> { return m_tree.equal_range(key); }
        template<typename ComparableToKey>
        auto equal_range(const ComparableToKey& key) const -> enable_if_t<Internal::is_transparent<key_compare, ComparableToKey>::value, pair<const_iterator, const_iterator>> { return m_tree.equal_range(key); }

        /**
        * \anchor ListExtensions
        * \name Extensions
        * @{
        */
        /**
        * insert and default construct an element \todo for this to make sense we need to add it to the rbtree container. To avoid any value type copy, similar to the hash_table.
        * this is here only to make sure it's available to the end user.
        */
        V_FORCE_INLINE iterator  insert(const key_type& key)
        {
            return m_tree.insert_equal(value_type(key, MappedType() /* post pone the ctor to avoid any copy */));
        }
        // The only difference from the standard is that we return the allocator instance, not a copy.
        V_FORCE_INLINE allocator_type&         get_allocator()         { return m_tree.get_allocator(); }
        V_FORCE_INLINE const allocator_type&   get_allocator() const   { return m_tree.get_allocator(); }
        /// Set the vector allocator. If different than then current all elements will be reallocated.
        V_FORCE_INLINE void                    set_allocator(const allocator_type& allocator) { m_tree.set_allocator(allocator); }
        // Validate container status.
        V_INLINE bool      validate() const    { return m_tree.validate(); }
        /// Validates an iter iterator. Returns a combination of \ref iterator_status_flag.
        V_INLINE int       validate_iterator(const const_iterator& iter) const { return m_tree.validate_iterator(iter); }
        V_INLINE int       validate_iterator(const iterator& iter) const       { return m_tree.validate_iterator(iter); }

        /**
        * Resets the container without deallocating any memory or calling any destructor.
        * This function should be used when we need very quick tear down. Generally it's used for temporary vectors and we can just nuke them that way.
        * In addition the provided \ref Allocators, has leak and reset flag which will enable automatically this behavior. So this function should be used in
        * special cases \ref VStdExamples.
        * \note This function is added to the vector for consistency. In the vector case we have only one allocation, and if the allocator allows memory leaks
        * it can just leave deallocate function empty, which performance wise will be the same. For more complex containers this will make big difference.
        */
        V_FORCE_INLINE void                        leak_and_reset()    { m_tree.leak_and_reset(); }
        /// @}
    private:
        tree_type   m_tree;
    };

    template<class Key, class MappedType, class Compare, class Allocator>
    V_FORCE_INLINE void swap(multimap<Key, MappedType, Compare, Allocator>& left, multimap<Key, MappedType, Compare, Allocator>& right)
    {
        left.swap(right);
    }

    template<class Key, class MappedType, class Compare, class Allocator>
    inline bool operator==(const multimap<Key, MappedType, Compare, Allocator>& left, const multimap<Key, MappedType, Compare, Allocator>& right)
    {
        return (left.size() == right.size()
                && equal(left.begin(), left.end(), right.begin()));
    }

    template<class Key, class MappedType, class Compare, class Allocator>
    inline bool operator!=(const multimap<Key, MappedType, Compare, Allocator>& left, const multimap<Key, MappedType, Compare, Allocator>& right)
    {
        return (!(left == right));
    }

    template<class Key, class MappedType, class Compare, class Allocator>
    inline bool operator<(const multimap<Key, MappedType, Compare, Allocator>& left, const multimap<Key, MappedType, Compare, Allocator>& right)
    {
        return (lexicographical_compare(left.begin(), left.end(), right.begin(), right.end()));
    }

    template<class Key, class MappedType, class Compare, class Allocator>
    inline bool operator>(const multimap<Key, MappedType, Compare, Allocator>& left, const multimap<Key, MappedType, Compare, Allocator>& right)
    {
        return (right < left);
    }

    template<class Key, class MappedType, class Compare, class Allocator>
    inline bool operator<=(const multimap<Key, MappedType, Compare, Allocator>& left, const multimap<Key, MappedType, Compare, Allocator>& right)
    {
        return (!(right < left));
    }

    template<class Key, class MappedType, class Compare, class Allocator>
    inline bool operator>=(const multimap<Key, MappedType, Compare, Allocator>& left, const multimap<Key, MappedType, Compare, Allocator>& right)
    {
        return (!(left < right));
    }

    template<class Key, class MappedType, class Compare, class Allocator, class Predicate>
    decltype(auto) erase_if(multimap<Key, MappedType, Compare, Allocator>& container, Predicate predicate)
    {
        auto originalSize = container.size();

        for (auto iter = container.begin(); iter != container.end(); )
        {
            if (predicate(*iter))
            {
                iter = container.erase(iter);
            }
            else
            {
                ++iter;
            }
        }

        return originalSize - container.size();
    }
}

#endif // V_FRAMEWORK_CORE_STD_ORDERED_MAP_H
#pragma once