#ifndef V_FRAMEWORK_PLATFORMS_WINDOWS_CORE__IO_STREAMER_STORAGE_DRIVE_CONFIG_WINDOWS_H
#define V_FRAMEWORK_PLATFORMS_WINDOWS_CORE__IO_STREAMER_STORAGE_DRIVE_CONFIG_WINDOWS_H

#include <vcore/io/streamer/streamer_configuration.h>

namespace V::IO
{
    class WindowsStorageDriveConfig final :
        public IStreamerStackConfig
    {
    public:
        VOBJECT_RTTI(V::IO::WindowsStorageDriveConfig, "{c9391bdf-8b5d-49d1-8d71-dae59369c439}", IStreamerStackConfig);
        V_CLASS_ALLOCATOR(WindowsStorageDriveConfig, SystemAllocator, 0);

        ~WindowsStorageDriveConfig() override = default;
        VStd::shared_ptr<StreamStackEntry> AddStreamStackEntry(
            const HardwareInformation& hardware, VStd::shared_ptr<StreamStackEntry> parent) override;

    private:
        V::u32 m_maxFileHandles{ 32 };
        V::u32 m_maxMetaDataCache{ 32 };
        V::u32 m_overcommit{ 8 };
        bool m_enableFileSharing{ false };
        bool m_enableUnbufferedReads{ true };
        bool m_minimalReporting{ false };
    };
} // namespace V::IO


#endif // V_FRAMEWORK_PLATFORMS_WINDOWS_CORE__IO_STREAMER_STORAGE_DRIVE_CONFIG_WINDOWS_H