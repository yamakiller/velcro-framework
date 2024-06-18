#ifndef V_FRAMEWORK_CORE_THREDING_THREAD_SAFE_DEQUE_H
#define V_FRAMEWORK_CORE_THREDING_THREAD_SAFE_DEQUE_H


#include <core/base.h>
#include <core/std/containers/deque.h>
#include <core/std/parallel/lock.h>

namespace V
{
    //! @class ThreadSafeQueue
    //! @brief thread safe double-ended queue data structure.
    template <typename TYPE>
    class ThreadSafeDeque final
    {
    public:

        using DequeType = VStd::deque<TYPE>;

        ThreadSafeDeque() = default;
        ~ThreadSafeDeque() = default;

        //! Returns the size of the dequeue in numbers of elements.
        //! @return the number of elements contained in the deque
        VStd::size_t Size() const;

        //! Clears the contents of the deque.
        void Clear();

        //! Pushes a new item to the front of the dequeue.
        //! @param item element to push to the front of the dequeue
        template <typename TYPE_DEDUCED>
        void PushFrontItem(TYPE_DEDUCED&& item);

        //! Pushes a new item to the back of the dequeue.
        //! @param item element to push to the back of the dequeue
        template <typename TYPE_DEDUCED>
        void PushBackItem(TYPE_DEDUCED&& item);

        //! Pops the front item from the dequeue.
        //! @param outItem output parameter containing the element at the front of the dequeue
        //! @return boolean true on success, false if the dequeue was empty
        bool PopFrontItem(TYPE& outItem);

        //! Pops the last item from the dequeue.
        //! @param outItem output parameter containing the element at the back of the dequeue
        //! @return boolean true on success, false if the dequeue was empty
        bool PopBackItem(TYPE& outItem);

        //! Swaps the underlying dequeue data with the input dequeue.
        //! @param swapDeque the non-thread safe deque to swap the internal deque with
        void Swap(DequeType& swapDeque);

        //! Visits all the elements of the deque under lock using the provided functor.
        //! @param visitor the functor to visit all deque elements with
        void Visit(const VStd::function<void(TYPE&)>& visitor);

        //! Visits the whole internal deque under lock using the provided functor.
        //! @param visitor the functor to operate on the deque with
        void VisitDeque(const VStd::function<void(DequeType&)>& visitor);

    private:

        mutable VStd::recursive_mutex m_mutex;
        DequeType m_deque;
    };
}

#include <core/threading/thread_safe_deque.inl>

#endif // V_FRAMEWORK_CORE_THREDING_THREAD_SAFE_DEQUE_H