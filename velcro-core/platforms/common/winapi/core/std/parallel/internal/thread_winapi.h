#ifndef V_FRAMEWORK_CORE_STD_PARALLEL_WINDOWS_INTERNAL_THREAD_WINAPI_H
#define V_FRAMEWORK_CORE_STD_PARALLEL_WINDOWS_INTERNAL_THREAD_WINAPI_H

#ifndef YieldProcessor
#    define YieldProcessor _mm_pause
#endif

extern "C" {
    // forward declare to avoid windows.h
    V_DLL_IMPORT unsigned long __stdcall GetCurrentThreadId(void);
}

#include <vcore/std/tuple.h>

namespace VStd {
    namespace Internal {
        /**
         * Create and run thread
         */
        HANDLE create_thread(const thread_desc* desc, thread_info* ti, unsigned int* id);
    }

    //////////////////////////////////////////////////////////////////////////
    // thread
    template<class F, class... Args, typename>
    thread::thread(F&& f, Args&&... args)
        : thread(thread_desc{}, VStd::forward<F>(f), VStd::forward<Args>(args)...)
    {}

    template<class F, class... Args>
    thread::thread(const thread_desc& desc, F&& f, Args&&... args) {
        auto threadfunc = [fn = VStd::forward<F>(f), argsTuple = VStd::make_tuple(VStd::forward<Args>(args)...)]() mutable -> void
        {
            VStd::apply(VStd::move(fn), VStd::move(argsTuple));
        };
        Internal::thread_info* ti = Internal::create_thread_info(VStd::move(threadfunc));
        m_thread.m_handle = Internal::create_thread(&desc, ti, &m_thread.m_id);
    }

    inline bool thread::joinable() const {
        if (m_thread.m_id == native_thread_invalid_id) {
            return false;
        }
        return m_thread.m_id != this_thread::get_id().m_id;
    }
    inline thread::id thread::get_id() const {
        return id(m_thread.m_id);
    }
    thread::native_handle_type
    inline thread::native_handle() {
        return m_thread.m_handle;
    }
    //////////////////////////////////////////////////////////////////////////

    namespace this_thread {
        V_FORCE_INLINE thread::id get_id() {
            DWORD threadId = GetCurrentThreadId();
            return thread::id(threadId);
        }

        V_FORCE_INLINE void yield() {
            ::Sleep(0);
        }

        V_FORCE_INLINE void pause(int numLoops) {
            for (int i = 0; i < numLoops; ++i) {
                YieldProcessor(); //pause instruction on x86, allows hyperthread to run
            }
        }

        template <class Rep, class Period>
        V_FORCE_INLINE void sleep_for(const chrono::duration<Rep, Period>& rel_time) {
            chrono::milliseconds toSleep = rel_time;
            ::Sleep((DWORD)toSleep.count());
        }
    }
}


#endif // V_FRAMEWORK_CORE_STD_PARALLEL_WINDOWS_INTERNAL_THREAD_WINAPI_H
