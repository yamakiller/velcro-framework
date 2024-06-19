#ifndef V_FRAMEWORK_CORE_EVENT_BUS_INTERNAL_STORAGE_POLICIES_H
#define V_FRAMEWORK_CORE_EVENT_BUS_INTERNAL_STORAGE_POLICIES_H

#include <vcore/event_bus/policies.h>
#include <vcore/event_bus/internal/debug.h>

#include <vcore/std/hash_table.h>
#include <vcore/std/containers/rbtree.h>
#include <vcore/std/containers/intrusive_list.h>
#include <vcore/std/containers/intrusive_set.h>

namespace V
{
    namespace Internal
    {
        /**
         * AddressStoragePolicy is used to determine how to store a mapping of address -> HandlerStorage.
         *
         * \tparam Traits       The traits for the Bus. Used to determin: id type, allocator type, whether to order container, and how to compare ids.
         * \tparam HandlerStorage   The type to use as the value of the map. This will often be a structure containing a list or ordered list of handlers.
         */
        template <typename Traits, typename HandlerStorage, EventBusAddressPolicy = Traits::AddressPolicy>
        struct AddressStoragePolicy;
        // Unordered
        template <typename Traits, typename HandlerStorage>
        struct AddressStoragePolicy<Traits, HandlerStorage, EventBusAddressPolicy::ById> {
        private:
            using IdType = typename Traits::BusIdType;

            struct hash_table_traits
            {
                using key_type = IdType;
                using key_equal = VStd::equal_to<key_type>;
                using value_type = HandlerStorage;
                using allocator_type = typename Traits::AllocatorType;
                enum
                {
                    max_load_factor = 4,
                    min_buckets = 7,
                    has_multi_elements = false,

                    is_dynamic = true,
                    fixed_num_buckets = 1,
                    fixed_num_elements = 4
                };
                static const key_type& key_from_value(const value_type& value) { return value.m_busId; }
                struct hasher
                {
                    VStd::size_t operator()(const IdType& id) const
                    {
                        return VStd::hash<IdType>()(id);
                    }
                };
            };

        public:
            struct StorageType : public VStd::hash_table<hash_table_traits> {
                using base_type = VStd::hash_table<hash_table_traits>;

                StorageType()
                    : base_type(typename base_type::hasher(), typename base_type::key_equal(), typename base_type::allocator_type())
                { }

                // EventBus extension: Assert on insert
                template <typename... InputArgs>
                typename base_type::iterator emplace(InputArgs&&... args) {
                    auto pair = base_type::emplace(VStd::forward<InputArgs>(args)...);
                    EVENTBUS_ASSERT(pair.second, "Internal error: Failed to insert");
                    return pair.first;
                }

                // EventBus extension: rehash on final erase
                void erase(const IdType& id) {
                    base_type::erase(id);
                    if (base_type::empty())
                    {
                        base_type::rehash(0);
                    }
                }
            };
        };
        // Ordered
        template <typename Traits, typename HandlerStorage>
        struct AddressStoragePolicy<Traits, HandlerStorage, EventBusAddressPolicy::ByIdAndOrdered>
        {
        private:
            using IdType = typename Traits::BusIdType;

            struct rbtree_traits
            {
                using key_type = IdType;
                using key_equal = typename Traits::BusIdOrderCompare;
                using value_type = HandlerStorage;
                using allocator_type = typename Traits::AllocatorType;
                static const key_type& key_from_value(const value_type& value) { return value.m_busId; }
            };

        public:
            struct StorageType
                : public VStd::rbtree<rbtree_traits>
            {
                using base_type = VStd::rbtree<rbtree_traits>;

                // EventBus extension: Assert on insert
                template <typename... InputArgs>
                typename StorageType::iterator emplace(InputArgs&&... args)
                {
                    auto pair = base_type::emplace_unique(VStd::forward<InputArgs>(args)...);
                    EVENTBUS_ASSERT(pair.second, "Internal error: Failed to insert");
                    return pair.first;
                }
            };
        };

        /**
         * Helpers for substituting default compare implementation when BusHandlerCompareDefault is specified.
         */
        template <typename /*Interface*/, typename Traits, typename Comparer = typename Traits::BusHandlerOrderCompare>
        struct HandlerCompare
            : public Comparer
        { };
        template <typename Interface, typename Traits>
        struct HandlerCompare<Interface, Traits, V::BusHandlerCompareDefault>
        {
            bool operator()(const Interface* left, const Interface* right) const { return left->Compare(right); }
        };

        /**
         * HandlerStoragePolicy is used to determine how to store a list of handlers. This collection will always be intrusive.
         *
         * \tparam Interface    The interface for the bus. Used as a parameter to the compare function.
         * \tparam Traits       The traits for the Bus. Used to determin: whether to order container, and how to compare handlers.
         * \tparam Handler      The handler type. This will be used as the value of the container. This type is expected to inherit from HandlerStorageNode.
         *                      MUST NOT BE A POINTER TYPE.
         */
        template <typename Interface, typename Traits, typename Handler, EventBusHandlerPolicy = Traits::HandlerPolicy>
        struct HandlerStoragePolicy;
        // Unordered
        template <typename Interface, typename Traits, typename Handler>
        struct HandlerStoragePolicy<Interface, Traits, Handler, EventBusHandlerPolicy::Multiple>
        {
        public:
            struct StorageType
                : public VStd::intrusive_list<Handler, VStd::list_base_hook<Handler>>
            {
                using base_type = VStd::intrusive_list<Handler, VStd::list_base_hook<Handler>>;

                void insert(Handler& elem)
                {
                    base_type::push_front(elem);
                }
            };
        };
        // Ordered
        template <typename Interface, typename Traits, typename Handler>
        struct HandlerStoragePolicy<Interface, Traits, Handler, EventBusHandlerPolicy::MultipleAndOrdered>
        {
        private:
            using Compare = HandlerCompare<Interface, Traits>;

        public:
            using StorageType = VStd::intrusive_multiset<Handler, VStd::intrusive_multiset_base_hook<Handler>, Compare>;
        };

        // Param Handler to HandlerStoragePolicy is expected to inherit from this type.
        template <typename Handler, EventBusHandlerPolicy>
        struct HandlerStorageNode
        {
        };
        template <typename Handler>
        struct HandlerStorageNode<Handler, EventBusHandlerPolicy::Multiple>
            : public VStd::intrusive_list_node<Handler>
        {
        };
        template <typename Handler>
        struct HandlerStorageNode<Handler, EventBusHandlerPolicy::MultipleAndOrdered>
            : public VStd::intrusive_multiset_node<Handler>
        {
        };
    } // namespace Internal
} // namespace V


#endif // V_FRAMEWORK_CORE_EVENT_BUS_INTERNAL_STORAGE_POLICIES_H