namespace V
{
    template <typename TYPE>
    inline VStd::size_t ThreadSafeDeque<TYPE>::Size() const
    {
        VStd::scoped_lock<VStd::recursive_mutex> lock(m_mutex);
        return m_deque.size();
    }

    template <typename TYPE>
    void ThreadSafeDeque<TYPE>::Clear()
    {
        VStd::scoped_lock<VStd::recursive_mutex> lock(m_mutex);
        m_deque.clear();
    }

    template <typename TYPE>
    template <typename TYPE_DEDUCED>
    inline void ThreadSafeDeque<TYPE>::PushFrontItem(TYPE_DEDUCED&& item)
    {
        VStd::scoped_lock<VStd::recursive_mutex> lock(m_mutex);
        m_deque.push_front(VStd::forward<TYPE_DEDUCED>(item));
    }
    
    template <typename TYPE>
    template <typename TYPE_DEDUCED>
    inline void ThreadSafeDeque<TYPE>::PushBackItem(TYPE_DEDUCED&& item)
    {
        VStd::scoped_lock<VStd::recursive_mutex> lock(m_mutex);
        m_deque.push_back(VStd::forward<TYPE_DEDUCED>(item));
    }

    template <typename TYPE>
    inline bool ThreadSafeDeque<TYPE>::PopFrontItem(TYPE& item)
    {
        VStd::scoped_lock<VStd::recursive_mutex> lock(m_mutex);
        if (m_deque.size() <= 0)
        {
            return false;
        }
        item = m_deque.front();
        m_deque.pop_front();
        return true;
    }

    template <typename TYPE>
    inline bool ThreadSafeDeque<TYPE>::PopBackItem(TYPE& item)
    {
        VStd::scoped_lock<VStd::recursive_mutex> lock(m_mutex);
        if (m_deque.size() <= 0)
        {
            return false;
        }
        item = m_deque.back();
        m_deque.pop_back();
        return true;
    }

    template <typename TYPE>
    inline void ThreadSafeDeque<TYPE>::Swap(DequeType& swapDeque)
    {
        VStd::scoped_lock<VStd::recursive_mutex> lock(m_mutex);
        swapDeque.swap(m_deque);
    }

    template <typename TYPE>
    inline void ThreadSafeDeque<TYPE>::Visit(const VStd::function<void(TYPE&)>& visitor)
    {
        VStd::scoped_lock<VStd::recursive_mutex> lock(m_mutex);
        for (TYPE& element : m_deque)
        {
            visitor(element);
        }
    }

    template <typename TYPE>
    inline void ThreadSafeDeque<TYPE>::VisitDeque(const VStd::function<void(DequeType&)>& visitor)
    {
        VStd::scoped_lock<VStd::recursive_mutex> lock(m_mutex);
        visitor(m_deque);
    }
}
