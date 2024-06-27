//#ifndef V_FRAMEWORK_CORE_STD_PARALLEL_WINDOWS_INTERNAL_MUTEX_WINAPI_H
//#define V_FRAMEWORK_CORE_STD_PARALLEL_WINDOWS_INTERNAL_MUTEX_WINAPI_H


namespace VStd {
    #define V_STD_MUTEX_CAST(m) reinterpret_cast<LPCRITICAL_SECTION>(&m)
    
    // mutex
    inline mutex::mutex()
    {
        InitializeCriticalSection(V_STD_MUTEX_CAST(m_mutex));
    }
    inline mutex::mutex(const char* name)
    {
        (void)name;
        InitializeCriticalSection(V_STD_MUTEX_CAST(m_mutex));
    }

    inline mutex::~mutex()
    {
        DeleteCriticalSection(V_STD_MUTEX_CAST(m_mutex));
    }

    V_FORCE_INLINE void
    mutex::lock()
    {
        EnterCriticalSection(V_STD_MUTEX_CAST(m_mutex));
    }
    V_FORCE_INLINE bool
    mutex::try_lock()
    {
        return TryEnterCriticalSection(V_STD_MUTEX_CAST(m_mutex)) != 0;
    }
    V_FORCE_INLINE void
    mutex::unlock()
    {
        LeaveCriticalSection(V_STD_MUTEX_CAST(m_mutex));
    }
    V_FORCE_INLINE mutex::native_handle_type
    mutex::native_handle()
    {
        return V_STD_MUTEX_CAST(m_mutex);
    }
    //////////////////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////////////////
    // recursive_mutex
    inline recursive_mutex::recursive_mutex()
    {
        InitializeCriticalSection(V_STD_MUTEX_CAST(m_mutex));
    }

    inline recursive_mutex::recursive_mutex(const char* name)
    {
        (void)name;
        InitializeCriticalSection(V_STD_MUTEX_CAST(m_mutex));
    }

    inline recursive_mutex::~recursive_mutex()
    {
        DeleteCriticalSection(V_STD_MUTEX_CAST(m_mutex));
    }

    V_FORCE_INLINE void
    recursive_mutex::lock()
    {
        EnterCriticalSection(V_STD_MUTEX_CAST(m_mutex));
    }
    V_FORCE_INLINE bool
    recursive_mutex::try_lock()
    {
        return TryEnterCriticalSection(V_STD_MUTEX_CAST(m_mutex)) != 0;
    }
    V_FORCE_INLINE void
    recursive_mutex::unlock()
    {
        LeaveCriticalSection(V_STD_MUTEX_CAST(m_mutex));
    }
    V_FORCE_INLINE recursive_mutex::native_handle_type
    recursive_mutex::native_handle()
    {
        return V_STD_MUTEX_CAST(m_mutex);
    }
    //////////////////////////////////////////////////////////////////////////
#undef V_STD_MUTEX_CAST
}

//#endif // V_FRAMEWORK_CORE_STD_PARALLEL_WINDOWS_INTERNAL_MUTEX_WINAPI_H