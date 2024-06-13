//#ifndef V_FRAMEWORK_CORE_STD_PARALLEL_WINDOWS_INTERNAL_CONDITION_VARIABLE_WINAPI_H
//#define V_FRAMEWORK_CORE_STD_PARALLEL_WINDOWS_INTERNAL_CONDITION_VARIABLE_WINAPI_H

namespace VStd {
#define V_COND_VAR_CAST(m) reinterpret_cast<PCONDITION_VARIABLE>(&m)
#define V_STD_MUTEX_CAST(m) reinterpret_cast<LPCRITICAL_SECTION>(&m)

   //////////////////////////////////////////////////////////////////////////
    // Condition variable
    inline condition_variable::condition_variable() {
        InitializeConditionVariable(V_COND_VAR_CAST(m_cond_var));
    }

    inline condition_variable::condition_variable(const char* name) {
        (void)name;
        InitializeConditionVariable(V_COND_VAR_CAST(m_cond_var));
    }

    inline condition_variable::~condition_variable() {
    }

    inline void condition_variable::notify_one() {
        WakeConditionVariable(V_COND_VAR_CAST(m_cond_var));
    }
    inline void condition_variable::notify_all() {
        WakeAllConditionVariable(V_COND_VAR_CAST(m_cond_var));
    }
    inline void condition_variable::wait(unique_lock<mutex>& lock) {
        SleepConditionVariableCS(V_COND_VAR_CAST(m_cond_var), lock.mutex()->native_handle(), V_INFINITE);
    }
    template <class Predicate>
    V_FORCE_INLINE void condition_variable::wait(unique_lock<mutex>& lock, Predicate pred) {
        while (!pred()) {
            wait(lock);
        }
    }

    template <class Clock, class Duration>
    V_FORCE_INLINE cv_status condition_variable::wait_until(unique_lock<mutex>& lock, const chrono::time_point<Clock, Duration>& abs_time) {
        chrono::system_clock::time_point now = chrono::system_clock::now();
        if (now < abs_time) {
            return wait_for(lock, abs_time - now);
        }

        return cv_status::timeout;
    }
    template <class Clock, class Duration, class Predicate>
    V_FORCE_INLINE bool condition_variable::wait_until(unique_lock<mutex>& lock, const chrono::time_point<Clock, Duration>& abs_time, Predicate pred) {
        while (!pred()) {
            if (wait_until(lock, abs_time) == cv_status::timeout) {
                return pred();
            }
        }
        return pred();
    }

    template <class Rep, class Period>
    V_FORCE_INLINE cv_status condition_variable::wait_for(unique_lock<mutex>& lock, const chrono::duration<Rep, Period>& rel_time) {
        chrono::milliseconds toWait = rel_time;
        // We need to make sure we use CriticalSection based mutex.
        // https://docs.microsoft.com/en-us/windows/win32/api/synchapi/nf-synchapi-sleepconditionvariablecs
        // indicates that this returns nonzero on success, zero on failure
        // we need to return cv_status::timeout IFF a timeout occurred and no_timeout in all other situations
        // including error.
        int ret = SleepConditionVariableCS(V_COND_VAR_CAST(m_cond_var), lock.mutex()->native_handle(), static_cast<DWORD>(toWait.count()));
        if (ret == 0) {
            int lastError = GetLastError();
            // detect a sitaution where the mutex or the cond var are invalid, or the duration
            V_Assert(lastError == V_ERROR_TIMEOUT, "Error from SleepConditionVariableCS: 0x%08x\n", lastError);
            // asserts are continuable so we still check.
            if (lastError == V_ERROR_TIMEOUT) {
                return cv_status::timeout;
            }
        } 
        return cv_status::no_timeout;
    }
    template <class Rep, class Period, class Predicate>
    V_FORCE_INLINE bool condition_variable::wait_for(unique_lock<mutex>& lock, const chrono::duration<Rep, Period>& rel_time, Predicate pred) {
        while (!pred()) {
            if (wait_for(lock, rel_time) == cv_status::timeout) {
                return pred();
            }
        }
        return pred();
    }
    condition_variable::native_handle_type
    inline condition_variable::native_handle() {
        return V_COND_VAR_CAST(m_cond_var);
    }

    //////////////////////////////////////////////////////////////////////////
    // Condition variable any
    inline condition_variable_any::condition_variable_any() {
        InitializeCriticalSection(V_STD_MUTEX_CAST(m_mutex));
        InitializeConditionVariable(V_COND_VAR_CAST(m_cond_var));
    }

    inline condition_variable_any::condition_variable_any(const char* name) {
        InitializeCriticalSection(V_STD_MUTEX_CAST(m_mutex));
        (void)name;
        InitializeConditionVariable(V_COND_VAR_CAST(m_cond_var));
    }

    inline condition_variable_any::~condition_variable_any() {
        DeleteCriticalSection(V_STD_MUTEX_CAST(m_mutex));
    }

    inline void condition_variable_any::notify_one() {
        EnterCriticalSection(V_STD_MUTEX_CAST(m_mutex));
        WakeConditionVariable(V_COND_VAR_CAST(m_cond_var));
        LeaveCriticalSection(V_STD_MUTEX_CAST(m_mutex));
    }
    inline void condition_variable_any::notify_all() {
        EnterCriticalSection(V_STD_MUTEX_CAST(m_mutex));
        WakeAllConditionVariable(V_COND_VAR_CAST(m_cond_var));
        LeaveCriticalSection(V_STD_MUTEX_CAST(m_mutex));
    }

    template<class Lock>
    V_FORCE_INLINE void condition_variable_any::wait(Lock& lock) {
        EnterCriticalSection(V_STD_MUTEX_CAST(m_mutex));
        lock.unlock();
        // We need to make sure we use CriticalSection based mutex.
        SleepConditionVariableCS(V_COND_VAR_CAST(m_cond_var), V_STD_MUTEX_CAST(m_mutex), V_INFINITE);
        LeaveCriticalSection(V_STD_MUTEX_CAST(m_mutex));
        lock.lock();
    }
    template <class Lock, class Predicate>
    V_FORCE_INLINE void condition_variable_any::wait(Lock& lock, Predicate pred) {
        while (!pred()) {
            wait(lock);
        }
    }
    template <class Lock, class Clock, class Duration>
    V_FORCE_INLINE cv_status condition_variable_any::wait_until(Lock& lock, const chrono::time_point<Clock, Duration>& abs_time) {
        auto now = chrono::system_clock::now();
        if (now < abs_time) {
            return wait_for(lock, abs_time - now);
        }

        return cv_status::no_timeout;
    }
    template <class Lock, class Clock, class Duration, class Predicate>
    V_FORCE_INLINE bool condition_variable_any::wait_until(Lock& lock, const chrono::time_point<Clock, Duration>& abs_time, Predicate pred) {
        while (!pred()) {
            if (wait_until(lock, abs_time) == cv_status::timeout) {
                return pred();
            }
        }
        return true;
    }
    template <class Lock, class Rep, class Period>
    V_FORCE_INLINE cv_status condition_variable_any::wait_for(Lock& lock, const chrono::duration<Rep, Period>& rel_time) {
        chrono::milliseconds toWait = rel_time;
        EnterCriticalSection(V_STD_MUTEX_CAST(m_mutex));
        lock.unlock();

        // We need to make sure we use CriticalSection based mutex.
        cv_status returnCode = cv_status::no_timeout;
        int ret = SleepConditionVariableCS(V_COND_VAR_CAST(m_cond_var), V_STD_MUTEX_CAST(m_mutex), toWait.count());
        // according to MSDN docs, ret is 0 on FAILURE, and nonzero otherwise.
        if (ret == 0) {
            int lastError = GetLastError();
            // if we hit this assert, its likely a programmer error
            // and the values passed into SleepConditionVariableCS are invalid.
            V_Assert(lastError == V_ERROR_TIMEOUT, "Error from SleepConditionVariableCS: 0x%08x\n", lastError );

            if (lastError == V_ERROR_TIMEOUT) {
                returnCode = cv_status::timeout;
            }
        }
        LeaveCriticalSection(V_STD_MUTEX_CAST(m_mutex));
        lock.lock();
        return returnCode;
    }
    template <class Lock, class Rep, class Period, class Predicate>
    V_FORCE_INLINE bool condition_variable_any::wait_for(Lock& lock, const chrono::duration<Rep, Period>& rel_time, Predicate pred) {
        while (!pred()) {
            if (!wait_for(lock, rel_time)) {
                return pred();
            }
        }
        return pred();
    }
    condition_variable_any::native_handle_type
    inline condition_variable_any::native_handle() {
        return V_COND_VAR_CAST(m_cond_var);
    }
    //////////////////////////////////////////////////////////////////////////
#undef V_COND_VAR_CAST //
#undef V_STD_MUTEX_CAST
}

//#endif // V_FRAMEWORK_CORE_STD_PARALLEL_WINDOWS_INTERNAL_CONDITION_VARIABLE_WINAPI_H