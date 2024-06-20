#ifndef V_FRAMEWORK_CORE_STD_CONTAINERS_FIXED_FORWARD_LIST_H
#define V_FRAMEWORK_CORE_STD_CONTAINERS_FIXED_FORWARD_LIST_H

#include <vcore/std/containers/forward_list.h>
#include <vcore/std/allocator_static.h>

namespace VStd
{
    /**
    * Fixed version of the \ref VStd::list. All of their functionality is the same
    * except we do not have allocator and never allocate memory.
    * \note At the moment fixed_list is based on a list with static pool allocator (which might change),
    * so don't rely on it.
    *
    * Check the fixed_list \ref VStdExamples.
    */
    template< class T, VStd::size_t NumberOfNodes>
    class fixed_forward_list
        : public forward_list<T, static_pool_allocator<typename Internal::forward_list_node<T>, NumberOfNodes > >
    {
        enum
        {
            CONTAINER_VERSION = 1
        };

        typedef fixed_forward_list<T, NumberOfNodes> this_type;
        typedef forward_list<T, static_pool_allocator<typename Internal::forward_list_node<T>, NumberOfNodes> > base_type;
    public:
        //////////////////////////////////////////////////////////////////////////
        // construct/copy/destroy
        V_FORCE_INLINE explicit fixed_forward_list() {}
        V_FORCE_INLINE explicit fixed_forward_list(typename base_type::size_type numElements, typename base_type::const_reference value = typename base_type::value_type())
            : base_type(numElements, value) {}
        template<class InputIterator>
        V_FORCE_INLINE fixed_forward_list(const InputIterator& first, const InputIterator& last)
            : base_type(first, last)   {}
        V_FORCE_INLINE fixed_forward_list(const this_type& rhs)
            : base_type(rhs)   {}
        V_FORCE_INLINE fixed_forward_list(std::initializer_list<T> list)
            : base_type(list) {}
        V_FORCE_INLINE this_type& operator=(const this_type& rhs)
        {
            if (this != &rhs)
            {
                base_type::assign(rhs.begin(), rhs.end());
            }
            return *this;
        }

        //V_FORCE_INLINE void * data() const ...
    private:
        // You are not allowed to change the allocator type.
        void set_allocator(const typename base_type::allocator_type& allocator) { (void)allocator; }
    };

    template< class T, VStd::size_t NumberOfNodes >
    V_FORCE_INLINE bool operator==(const fixed_forward_list<T, NumberOfNodes>& left, const fixed_forward_list<T, NumberOfNodes>& right)
    {
        return (left.size() == right.size() && equal(left.begin(), left.end(), right.begin()));
    }

    template< class T, VStd::size_t NumberOfNodes >
    V_FORCE_INLINE bool operator!=(const fixed_forward_list<T, NumberOfNodes>& left, const fixed_forward_list<T, NumberOfNodes>& right)
    {
        return !(left == right);
    }
}

#endif // V_FRAMEWORK_CORE_STD_CONTAINERS_FIXED_FORWARD_LIST_H