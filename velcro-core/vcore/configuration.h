#ifndef V_FRAMEWORK_CORE_CONFIGURATION_H
#define V_FRAMEWORK_CORE_CONFIGURATION_H

#include <vcore/platform_default.h>

#if defined(V_MONOLITHIC_BUILD)
    #define VELCRO_CORE_API
#else
    #if defined(VELCROCORE_EXPORTS)
        #define VELCRO_CORE_API V_DLL_EXPORT
    #else
        #define VELCRO_CORE_API V_DLL_IMPORT
    #endif
#endif

#endif