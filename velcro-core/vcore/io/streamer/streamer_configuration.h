#ifndef V_FRAMEWORK_CORE_IO_STREAMER_STREAMER_CONFIGURATION_H
#define V_FRAMEWORK_CORE_IO_STREAMER_STREAMER_CONFIGURATION_H

#include <vcore/base.h>
#include <vcore/std/any.h>
//#include <vcore/memory/system_allocator.h>
#include <vcore/std/containers/map.h>
#include <vcore/std/containers/vector.h>
#include <vcore/std/smart_ptr/shared_ptr.h>
#include <vcore/std/string/string.h>


#if !defined(V_STREAMER_ADD_EXTRA_PROFILING_INFO)
#   if defined(_RELEASE)
#       define V_STREAMER_ADD_EXTRA_PROFILING_INFO 0
#   else
#       define V_STREAMER_ADD_EXTRA_PROFILING_INFO 1
#   endif
#endif

namespace V::IO
{
    class StreamStackEntry;

    struct HardwareInformation
    {
        VStd::any PlatformData;
        VStd::string Profile{"Default"};
        size_t MaxPhysicalSectorSize{ VCORE_GLOBAL_NEW_ALIGNMENT };
        size_t MaxLogicalSectorSize{ VCORE_GLOBAL_NEW_ALIGNMENT };
        size_t MaxPageSize{ VCORE_GLOBAL_NEW_ALIGNMENT };
        size_t MaxTransfer{ VCORE_GLOBAL_NEW_ALIGNMENT };
    };

    class IStreamerStackConfig
    {
    public:
        VOBJECT_RTTI(V::IO::IStreamerStackConfig, "{97266736-E55E-4BF4-9E4A-9D5A9FF4D230}");
        V_CLASS_ALLOCATOR(IStreamerStackConfig, SystemAllocator, 0);

        virtual ~IStreamerStackConfig() = default;
        virtual VStd::shared_ptr<StreamStackEntry> AddStreamStackEntry(
            const HardwareInformation& hardware, VStd::shared_ptr<StreamStackEntry> parent) = 0;
    };

    class StreamerConfig final
    {
    public:
        VOBJECT_RTTI(V::IO::StreamerConfig, "{B20540CB-A75C-45F9-A891-7ABFD9192E9F}");
        V_CLASS_ALLOCATOR(StreamerConfig, SystemAllocator, 0);

        VStd::vector<VStd::shared_ptr<IStreamerStackConfig>> StackConfig;
       
    };

    //! Collects hardware information for all hardware that can be used for file IO.
    //! @param info The retrieved hardware information.
    //! @param includeAllHardware Includes all available hardware that can be used by AZ::IO::Streamer. If set to false
    //!         only hardware is listed that is known to be used. This may be more performant, but can result is file
    //!         requests failing if they use an previously unknown path.
    //! @param reportHardware If true, hardware information will be printed to the log if available.
    extern bool CollectIoHardwareInformation(HardwareInformation& info, bool includeAllHardware, bool reportHardware);
    

    //! Constant used to denote "file not found" in StreamStackEntry processing.
    inline static constexpr size_t _fileNotFound = std::numeric_limits<size_t>::max();

    //! The number of entries in the statistics window. Larger number will measure over a longer time and will be more
    //! accurate, but also less responsive to sudden changes.
    static const size_t _statisticsWindowSize = 128;
} // namespace V::IO

#endif // V_FRAMEWORK_CORE_IO_STREAMER_STREAMER_CONFIGURATION_H