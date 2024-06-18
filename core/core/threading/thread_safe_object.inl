namespace V
{
    template <typename _TYPE>
    inline ThreadSafeObject<_TYPE>::ThreadSafeObject(const _TYPE& rhs)
        : m_object(rhs)
    {
        ;
    }

    template <typename _TYPE>
    inline void ThreadSafeObject<_TYPE>::operator =(const _TYPE& value)
    {
        VStd::scoped_lock<VStd::mutex> lock(m_mutex);
        m_object = value;
    }

    template <typename _TYPE>
    inline ThreadSafeObject<_TYPE>::operator _TYPE() const
    {
        VStd::scoped_lock<VStd::mutex> lock(m_mutex);
        return m_object;
    }
}