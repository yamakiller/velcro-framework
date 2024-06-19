#ifndef V_FRAMEWORK_CORE_PLATFORM_ID_H
#define V_FRAMEWORK_CORE_PLATFORM_ID_H

#include <stdint.h>

namespace V {
    enum PlatformID : int {
        PLATFORM_WINDOWS_64,
        PLATFORM_LINUX_64,
        PLATFORM_APPLE_IOS,
        PLATFORM_APPLE_MAC,
        PLATFORM_ANDROID_64, // ARMv8-64bit
// 外部定义的新平台
#if defined(V_EXPAND_FOR_RESTRICTED_PLATFORM) || defined(V_TOOLS_EXPAND_FOR_RESTRICTED_PLATFORMS)
#define V_RESTRICTED_PLATFORM_EXPANSION(CodeName, CODENAME, codename, PrivateName, PRIVATENAME, privatename, PublicName, PUBLICNAME, publicname, PublicAuxName1, PublicAuxName2, PublicAuxName3)\
        PLATFORM_##PUBLICNAME,
#if defined(V_EXPAND_FOR_RESTRICTED_PLATFORM)
        V_EXPAND_FOR_RESTRICTED_PLATFORM
#else
        V_TOOLS_EXPAND_FOR_RESTRICTED_PLATFORMS
#endif
#undef V_RESTRICTED_PLATFORM_EXPANSION
#endif
        PLATFORM_MAX,
    };

    inline bool IsBigEndian(PlatformID /*id*/) {
        return false;
    }

    const char* GetPlatformName(PlatformID platform);
}

#include <vcore/platform_id/platform_id_platform.h>

#endif // V_FRAMEWORK_CORE_PLATFORM_ID_H