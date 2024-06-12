#include <core/platform_id/platform_id.h>
#include <core/debug/trace.h>


namespace V {
        const char* GetPlatformName(PlatformID platform)
    {
        switch (platform)
        {
        case PlatformID::PLATFORM_WINDOWS_64:
            return "Win64";
        case PlatformID::PLATFORM_LINUX_64:
            return "Linux";
        case PlatformID::PLATFORM_ANDROID_64:
            return "Android64";
        case PlatformID::PLATFORM_APPLE_IOS:
            return "iOS";
        case PlatformID::PLATFORM_APPLE_MAC:
            return "Mac";
#if defined(V_EXPAND_FOR_RESTRICTED_PLATFORM) || defined(V_TOOLS_EXPAND_FOR_RESTRICTED_PLATFORMS)
#define V_RESTRICTED_PLATFORM_EXPANSION(CodeName, CODENAME, codename, PrivateName, PRIVATENAME, privatename, PublicName, PUBLICNAME, publicname, PublicAuxName1, PublicAuxName2, PublicAuxName3)\
        case PlatformID::PLATFORM_##PUBLICNAME:\
            return #PublicName;
#if defined(V_EXPAND_FOR_RESTRICTED_PLATFORM)
            V_EXPAND_FOR_RESTRICTED_PLATFORM
#else
            V_TOOLS_EXPAND_FOR_RESTRICTED_PLATFORMS
#endif
#undef V_RESTRICTED_PLATFORM_EXPANSION
#endif
        default:
            V_Assert(false, "Platform %u is unknown.", static_cast<uint32_t>(platform));
            return "";
        }
    }

}

#include <core/platform_id/platform_id_platform.h>