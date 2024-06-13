#ifndef V_FRAMEWORK_CORE_STD_PARALLEL_WINDOWS_H
#define V_FRAMEWORK_CORE_STD_PARALLEL_WINDOWS_H

#include <intrin.h>
#include <core/std/typetraits/aligned_storage.h>

#pragma intrinsic (_InterlockedIncrement)
#pragma intrinsic (_InterlockedDecrement)

#ifndef WAIT_OBJECT_0
    #define V_WAIT_OBJECT_0 0
#else 
    #define V_WAIT_OBJECT_0 WAIT_OBJECT_0
#endif

#ifndef ERROR_TIMEOUT
    #define V_ERROR_TIMEOUT 0x000005B4
#else
    #define V_ERROR_TIMEOUT ERROR_TIMEOUT
#endif

#ifndef INFINITE
    #define V_INFINITE            0xFFFFFFFF  // Infinite timeout
#else
    #define V_INFINITE            INFINITE 
#endif

extern "C"
{
    // forward declare to avoid windows.h
    using HANDLE = void*;
    using DWORD = unsigned long;
    using BOOL = int;
    using LONG = long;
    using LPLONG = long*;
    using LPCSTR = const char*;
    using LPCWSTR = const wchar_t*;
    // CRITICAL SECTION
    using RTL_CRITICAL_SECTION = struct _RTL_CRITICAL_SECTION;
    using CRITICAL_SECTION = RTL_CRITICAL_SECTION;
    using PCRITICAL_SECTION = RTL_CRITICAL_SECTION*;
    using LPCRITICAL_SECTION = RTL_CRITICAL_SECTION*;

    V_DLL_IMPORT void __stdcall InitializeCriticalSection(LPCRITICAL_SECTION);
    V_DLL_IMPORT void __stdcall DeleteCriticalSection(LPCRITICAL_SECTION);
    V_DLL_IMPORT void __stdcall EnterCriticalSection(LPCRITICAL_SECTION);
    V_DLL_IMPORT void __stdcall LeaveCriticalSection(LPCRITICAL_SECTION);
    V_DLL_IMPORT BOOL __stdcall TryEnterCriticalSection(LPCRITICAL_SECTION);

    // CONDITIONAL VARIABLE
    using RTL_CONDITION_VARIABLE = struct _RTL_CONDITION_VARIABLE;
    using CONDITION_VARIABLE = RTL_CONDITION_VARIABLE;
    using PCONDITION_VARIABLE = RTL_CONDITION_VARIABLE*;

    V_DLL_IMPORT void __stdcall InitializeConditionVariable(PCONDITION_VARIABLE);
    V_DLL_IMPORT void __stdcall WakeConditionVariable(PCONDITION_VARIABLE);
    V_DLL_IMPORT void __stdcall WakeAllConditionVariable(PCONDITION_VARIABLE);
    V_DLL_IMPORT BOOL __stdcall SleepConditionVariableCS(PCONDITION_VARIABLE, PCRITICAL_SECTION, DWORD);

    // Generic
    V_DLL_IMPORT DWORD _stdcall WaitForSingleObject(HANDLE hHandle, DWORD dwMilliseconds);
    V_DLL_IMPORT BOOL _stdcall CloseHandle(HANDLE hObject);
    V_DLL_IMPORT DWORD _stdcall GetLastError();
    V_DLL_IMPORT void _stdcall Sleep(DWORD);
    using SECURITY_ATTRIBUTES = struct _SECURITY_ATTRIBUTES;
    using LPSECURITY_ATTRIBUTES = SECURITY_ATTRIBUTES*;

    // Semaphore
    V_DLL_IMPORT HANDLE _stdcall CreateSemaphoreW(LPSECURITY_ATTRIBUTES lpSemaphoreAttributes, LONG lInitialCount, LONG lMaximumCount, LPCWSTR lpName);
    V_DLL_IMPORT BOOL _stdcall ReleaseSemaphore(HANDLE hSemaphore, LONG lReleaseCount, LPLONG lpPreviousCount);

    // Event
    V_DLL_IMPORT HANDLE _stdcall CreateEventW(LPSECURITY_ATTRIBUTES lpEventAttributes, BOOL bManualReset, BOOL bInitialState, LPCWSTR lpName);
    V_DLL_IMPORT BOOL _stdcall SetEvent(HANDLE);
}

namespace VStd
{
    // Mutex
    using native_mutex_data_type = VStd::aligned_storage_t<40, 8>; // declare storage for CRITICAL_SECTION (40 bytes on x64) to avoid windows.h include
    using native_mutex_handle_type = CRITICAL_SECTION*;
    using native_recursive_mutex_data_type = native_mutex_data_type;
    using native_recursive_mutex_handle_type = CRITICAL_SECTION*;

    using native_cond_var_data_type = VStd::aligned_storage_t<8, 8>; // declare storage for CONDITION_VARIABLE (8 bytes on x64) to avoid windows.h include
    using native_cond_var_handle_type = CONDITION_VARIABLE*;

    // Semaphore
    using native_semaphore_data_type = HANDLE;
    using native_semaphore_handle_type = HANDLE;

    // Thread
    using native_thread_id_type = unsigned;
    static constexpr native_thread_id_type native_thread_invalid_id = 0;
    struct native_thread_data_type
    {
        HANDLE m_handle;
        native_thread_id_type m_id;
    };
    using native_thread_handle_type = HANDLE;
}


#endif // V_FRAMEWORK_CORE_STD_PARALLEL_WINDOWS_H