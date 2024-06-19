#include <vcore/debug/stack_tracer.h>

#include <tchar.h>
#include <stdio.h>

#include <NTSecAPI.h> // for LdrDllNotification dll load hook (UNICODE_STRING)