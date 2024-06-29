#ifndef V_FRAMEWORK_PLATFORMS_WINDOWS_CORE__IO_STREAMER_STREAMER_CONFIGURATION_WINDOWS_H
#define V_FRAMEWORK_PLATFORMS_WINDOWS_CORE__IO_STREAMER_STREAMER_CONFIGURATION_WINDOWS_H


#include <vcore/base.h>
#include <vcore/memory/memory.h>
#include <vcore/std/containers/vector.h>
#include <vcore/std/string/string.h>
#include <vcore/vobject/vobject.h>


namespace V::IO
{
    struct DriveInformation
    {
        VOBJECT(V::IO::DriveInformation, "{4e77964a-e7c3-4fdb-ac72-b534678f3814}");

        VStd::vector<VStd::string> Paths;
        VStd::string Profile;
        size_t PhysicalSectorSize{ VCORE_GLOBAL_NEW_ALIGNMENT };
        size_t LogicalSectorSize{ VCORE_GLOBAL_NEW_ALIGNMENT };
        size_t PageSize{ 0 };
        size_t MaxTransfer{ 0 };
        u32 IoChannelCount{ 0 };
        bool SupportsQueuing{ false };
        bool HasSeekPenalty{ true };
    };

    using DriveList = VStd::vector<DriveInformation>;
} // namespace V::IO


#endif // V_FRAMEWORK_PLATFORMS_WINDOWS_CORE__IO_STREAMER_STREAMER_CONFIGURATION_WINDOWS_H