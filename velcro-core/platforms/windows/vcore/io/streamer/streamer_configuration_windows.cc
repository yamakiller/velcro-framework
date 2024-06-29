#include <vcore/platform_incl.h>
#include <vcore/io/istreamer_types.h>
#include <vcore/io/streamer/storage_drive_config_windows.h>
#include <vcore/io/streamer/streamer_configuration_windows.h>
//#include <vcore/Settings/SettingsRegistry.h>
//#include <vcore/Settings/SettingsRegistryMergeUtils.h>
#include <vcore/std/containers/unordered_map.h>
#include <vcore/string_func/string_func.h>

// https://developercommunity.visualstudio.com/t/windows-sdk-100177630-pragma-push-pop-mismatch-in/386142
#define _NTDDSCM_H_
#include <winioctl.h>

namespace V::IO
{
    static DWORD NextPowerOfTwo(DWORD value)
    {
        --value;
        value |= value >> 1;
        value |= value >> 2;
        value |= value >> 4;
        value |= value >> 8;
        value |= value >> 16;
        ++value;
        return value;
    }

    static void CollectIoAdaptor(HANDLE deviceHandle, DriveInformation& info, const char* driveName, bool reportHardware)
    {
        STORAGE_ADAPTER_DESCRIPTOR adapterDescriptor{};
        STORAGE_PROPERTY_QUERY query{};
        query.QueryType = PropertyStandardQuery;
        query.PropertyId = StorageAdapterProperty;

        DWORD bytesReturned = 0;
        if (::DeviceIoControl(deviceHandle, IOCTL_STORAGE_QUERY_PROPERTY, &query, sizeof(query),
            &adapterDescriptor, sizeof(adapterDescriptor), &bytesReturned, nullptr))
        {
            switch (adapterDescriptor.BusType)
            {
            case BusTypeScsi:               info.Profile = "Scsi";                break;
            case BusTypeAtapi:              info.Profile = "Atapi";               break;
            case BusTypeAta:                info.Profile = "Ata";                 break;
            case BusType1394:               info.Profile = "1394";                break;
            case BusTypeSsa:                info.Profile = "Ssa";                 break;
            case BusTypeFibre:              info.Profile = "Fibre";               break;
            case BusTypeUsb:                info.Profile = "Usb";                 break;
            case BusTypeRAID:               info.Profile = "Raid";                break;
            case BusTypeiScsi:              info.Profile = "iScsi";               break;
            case BusTypeSas:                info.Profile = "Sas";                 break;
            case BusTypeSata:               info.Profile = "Sata";                break;
            case BusTypeSd:                 info.Profile = "Sd";                  break;
            case BusTypeMmc:                info.Profile = "Mmc";                 break;
            case BusTypeVirtual:            info.Profile = "Virtual";             break;
            case BusTypeFileBackedVirtual:  info.Profile = "File backed virtual"; break;
            case BusTypeSpaces:             info.Profile = "Spaces";              break;
            case BusTypeNvme:               info.Profile = "Nvme";                break;
            case BusTypeSCM:                info.Profile = "SCM";                 break;
            case BusTypeUfs:                info.Profile = "Ufs";                 break;
            default:                        info.Profile = "Generic";
            }

            DWORD pageSize = NextPowerOfTwo(adapterDescriptor.MaximumTransferLength / adapterDescriptor.MaximumPhysicalPages);

            if (reportHardware)
            {
                V_Printf(
                    "Streamer",
                    "Adapter for drive '%s':\n"
                    "    Bus: %s %i.%i\n"
                    "    Max transfer: %.3f kb\n"
                    "    Page size: %i kb for %i pages\n"
                    "    Supports queuing: %s\n",
                    driveName,
                    info.Profile.c_str(), adapterDescriptor.BusMajorVersion, adapterDescriptor.BusMinorVersion,
                    (1.0f / 1024.0f) * adapterDescriptor.MaximumTransferLength,
                    pageSize, adapterDescriptor.MaximumPhysicalPages,
                    adapterDescriptor.CommandQueueing ? "Yes" : "No");
            }

            info.MaxTransfer = adapterDescriptor.MaximumTransferLength;
            info.PageSize = pageSize;
            info.SupportsQueuing = adapterDescriptor.CommandQueueing;
        }
    }

    static void AppendDriveTypeToProfile(HANDLE deviceHandle, DriveInformation& information, bool reportHardware)
    {
        // Check for support for the TRIM command. This command is exclusively used by SSD drives so can be used to
        // tell the difference between SSD and HDD. Since these are the only 2 types supported right now, no further
        // checks need to happen.

        DEVICE_TRIM_DESCRIPTOR trimDescriptor{};
        STORAGE_PROPERTY_QUERY query{};
        query.QueryType = PropertyStandardQuery;
        query.PropertyId = StorageDeviceTrimProperty;

        DWORD bytesReturned = 0;
        if (::DeviceIoControl(deviceHandle, IOCTL_STORAGE_QUERY_PROPERTY, &query, sizeof(query),
            &trimDescriptor, sizeof(trimDescriptor), &bytesReturned, nullptr))
        {
            information.Profile += trimDescriptor.TrimEnabled ? "_SSD" : "_HDD";
            if (reportHardware)
            {

                V_Printf("Streamer",
                    "    Drive type: %s\n",
                    trimDescriptor.TrimEnabled ? "SSD" : "HDD");
            }
        }
        else
        {
            if (reportHardware)
            {
                V_Printf("Streamer", "    Drive type couldn't be determined.");
            }
        }
    }

    static void CollectAlignmentRequirements(HANDLE deviceHandle, DriveInformation& information, bool reportHardware)
    {
        STORAGE_ACCESS_ALIGNMENT_DESCRIPTOR alignmentDescriptor{};
        STORAGE_PROPERTY_QUERY query{};
        query.QueryType = PropertyStandardQuery;
        query.PropertyId = StorageAccessAlignmentProperty;

        DWORD bytesReturned = 0;
        if (::DeviceIoControl(deviceHandle, IOCTL_STORAGE_QUERY_PROPERTY, &query, sizeof(query),
            &alignmentDescriptor, sizeof(alignmentDescriptor), &bytesReturned, nullptr))
        {
            information.PhysicalSectorSize = v_numeric_caster(alignmentDescriptor.BytesPerPhysicalSector);
            information.LogicalSectorSize  = v_numeric_caster(alignmentDescriptor.BytesPerLogicalSector);
            if (reportHardware)
            {
                V_Printf(
                    "Streamer",
                    "    Physical sector size: %i bytes\n"
                    "    Logical sector size: %i bytes\n",
                    alignmentDescriptor.BytesPerPhysicalSector,
                    alignmentDescriptor.BytesPerLogicalSector);
            }
        }
    }

    static void CollectDriveInfo(HANDLE deviceHandle, const char* driveName, bool reportHardware)
    {
        STORAGE_DEVICE_DESCRIPTOR sizeRequest{};
        STORAGE_PROPERTY_QUERY query{};
        query.QueryType = PropertyStandardQuery;
        query.PropertyId = StorageDeviceProperty;

        DWORD bytesReturned = 0;
        if (::DeviceIoControl(deviceHandle, IOCTL_STORAGE_QUERY_PROPERTY, &query, sizeof(query),
            &sizeRequest, sizeof(sizeRequest), &bytesReturned, nullptr))
        {
            auto header = reinterpret_cast<STORAGE_DESCRIPTOR_HEADER*>(&sizeRequest);
            auto buffer = VStd::unique_ptr<char[]>(new char[header->Size]);

            if (::DeviceIoControl(deviceHandle, IOCTL_STORAGE_QUERY_PROPERTY, &query, sizeof(query),
                buffer.get(), header->Size, &bytesReturned, nullptr))
            {
                if (reportHardware)
                {
                    auto deviceDescriptor = reinterpret_cast<STORAGE_DEVICE_DESCRIPTOR*>(buffer.get());
                    V_Printf("Streamer",
                        "Drive info for '%s':\n"
                        "    Id: %s%s%s%s%s\n",
                        driveName,
                        deviceDescriptor->VendorIdOffset != 0 ? buffer.get() + deviceDescriptor->VendorIdOffset : "",
                        deviceDescriptor->VendorIdOffset != 0 ? " " : "",
                        deviceDescriptor->ProductIdOffset != 0 ? buffer.get() + deviceDescriptor->ProductIdOffset : "",
                        deviceDescriptor->ProductIdOffset != 0 ? " " : "",
                        deviceDescriptor->ProductRevisionOffset != 0 ? buffer.get() + deviceDescriptor->ProductRevisionOffset : "");
                }
            }
        }
    }

    static void CollectDriveIoCapability(HANDLE deviceHandle, DriveInformation& information, bool reportHardware)
    {
        STORAGE_DEVICE_IO_CAPABILITY_DESCRIPTOR capabilityDescriptor{};
        STORAGE_PROPERTY_QUERY query{};
        query.QueryType = PropertyStandardQuery;
        query.PropertyId = StorageDeviceIoCapabilityProperty;

        DWORD bytesReturned = 0;
        if (::DeviceIoControl(deviceHandle, IOCTL_STORAGE_QUERY_PROPERTY, &query, sizeof(query),
            &capabilityDescriptor, sizeof(capabilityDescriptor), &bytesReturned, nullptr))
        {
            information.IoChannelCount = v_numeric_caster(capabilityDescriptor.LunMaxIoCount);
            if (reportHardware)
            {
                V_Printf(
                    "Streamer",
                    "    Max IO count (LUN): %i\n"
                    "    Max IO count (Adapter): %i\n",
                    capabilityDescriptor.LunMaxIoCount,
                    capabilityDescriptor.AdapterMaxIoCount);
            }
        }
    }

    static void CollectDriveSeekPenalty(HANDLE deviceHandle, DriveInformation& information, bool reportHardware)
    {
        DWORD bytesReturned = 0;

        // Get seek penalty information.
        DEVICE_SEEK_PENALTY_DESCRIPTOR seekPenaltyDescriptor{};
        STORAGE_PROPERTY_QUERY query{};
        query.QueryType = PropertyStandardQuery;
        query.PropertyId = StorageDeviceSeekPenaltyProperty;
        if (::DeviceIoControl(deviceHandle, IOCTL_STORAGE_QUERY_PROPERTY, &query, sizeof(query),
            &seekPenaltyDescriptor, sizeof(seekPenaltyDescriptor), &bytesReturned, nullptr))
        {
            information.HasSeekPenalty = seekPenaltyDescriptor.IncursSeekPenalty ? true : false;
            if (reportHardware)
            {
                V_Printf("Streamer",
                    "    Has seek penalty: %s\n",
                    information.HasSeekPenalty ? "Yes" : "No");
            }
        }

    }

    static bool IsDriveUsed(VStd::string_view driveId)
    {
        return false;
    }

    static bool CollectHardwareInfo(HardwareInformation& hardwareInfo, bool addAllDrives, bool reportHardware)
    {
        char drives[512];
        if (::GetLogicalDriveStringsA(sizeof(drives) - 1, drives))
        {
            VStd::unordered_map<DWORD, DriveInformation> driveMappings;
            char* driveIt = drives;
            do
            {
                UINT driveType = ::GetDriveTypeA(driveIt);
                // Only a selective set of devices that share similar behavior are supported, in particular
                // drives that have magnetic or solid state storage. All types of buses (usb, sata, etc)
                // are supported except network drives. If network support is needed it's better to use the
                // virtual file system. Optical drives are not supported as they are currently not in use
                // on any platform for games, as games are expected to be downloaded or installed to storage.
                if (driveType == DRIVE_FIXED || driveType == DRIVE_REMOVABLE || driveType == DRIVE_RAMDISK)
                {
                    if (!addAllDrives && !IsDriveUsed(driveIt))
                    {
                        if (reportHardware)
                        {
                            V_Printf("Streamer", "Skipping drive '%s' because no paths make use of it.\n", driveIt);
                        }
                        while (*driveIt++);
                        continue;
                    }

                    VStd::string deviceName = R"(\\.\)";
                    deviceName += driveIt;
                    deviceName.erase(deviceName.length() - 1); // Erase the slash.

                    HANDLE deviceHandle = ::CreateFileA(
                        deviceName.c_str(), 0, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING, 0, nullptr);
                    if (deviceHandle != INVALID_HANDLE_VALUE)
                    {
                        DWORD bytesReturned = 0;
                        // Get the device number so multiple partitions and mappings are handled by the same drive.
                        STORAGE_DEVICE_NUMBER storageDeviceNumber{};
                        if (::DeviceIoControl(deviceHandle, IOCTL_STORAGE_GET_DEVICE_NUMBER, nullptr, 0,
                            &storageDeviceNumber, sizeof(storageDeviceNumber), &bytesReturned, nullptr))
                        {
                            auto driveInformationEntry = driveMappings.find(storageDeviceNumber.DeviceNumber);
                            if (driveInformationEntry == driveMappings.end())
                            {
                                DriveInformation driveInformation;
                                driveInformation.Paths.emplace_back(driveIt);

                                CollectIoAdaptor(deviceHandle, driveInformation, driveIt, reportHardware);
                                if (driveInformation.SupportsQueuing)
                                {
                                    CollectDriveInfo(deviceHandle, driveIt, reportHardware);
                                    AppendDriveTypeToProfile(deviceHandle, driveInformation, reportHardware);
                                    CollectDriveIoCapability(deviceHandle, driveInformation, reportHardware);
                                    CollectDriveSeekPenalty(deviceHandle, driveInformation, reportHardware);
                                    CollectAlignmentRequirements(deviceHandle, driveInformation, reportHardware);

                                    hardwareInfo.MaxPhysicalSectorSize =
                                        VStd::max(hardwareInfo.MaxPhysicalSectorSize, driveInformation.PhysicalSectorSize);
                                    hardwareInfo.MaxLogicalSectorSize =
                                        VStd::max(hardwareInfo.MaxLogicalSectorSize, driveInformation.LogicalSectorSize);
                                    hardwareInfo.MaxPageSize = VStd::max(hardwareInfo.MaxPageSize, driveInformation.PageSize);
                                    hardwareInfo.MaxTransfer = VStd::max(hardwareInfo.MaxTransfer, driveInformation.MaxTransfer);

                                    driveMappings.insert({ storageDeviceNumber.DeviceNumber, VStd::move(driveInformation) });

                                    if (reportHardware)
                                    {
                                        V_Printf("Streamer", "\n");
                                    }
                                }
                                else
                                {
                                    if (reportHardware)
                                    {
                                        V_Printf(
                                            "Streamer", "Skipping drive '%s' because device does not support queuing requests.\n", driveIt);
                                    }
                                }
                            }
                            else
                            {
                                if (reportHardware)
                                {
                                    V_Printf(
                                        "Streamer", "Drive '%s' is on the same storage drive as '%s'.\n",
                                        driveIt, driveInformationEntry->second.Paths[0].c_str());
                                }
                                driveInformationEntry->second.Paths.emplace_back(driveIt);
                            }
                        }
                        else
                        {
                            if (reportHardware)
                            {
                                V_Printf(
                                    "Streamer", "Skipping drive '%s' because device is not registered with OS as a storage device.\n",
                                    driveIt);
                            }
                        }
                        ::CloseHandle(deviceHandle);
                    }
                    else
                    {
                        V_Warning("Streamer", false,
                            "Skipping drive '%s' because device information can't be retrieved.\n", driveIt);
                    }
                }
                else
                {
                    if (reportHardware)
                    {
                        V_Printf("Streamer", "Skipping drive '%s', as it the type of drive is not supported.\n", driveIt);
                    }
                }

                // Move to next drive string. GetLogicalDriveStrings fills the target buffer with null-terminated strings, for instance
                // "E:\\0F:\\0G:\\0\0". The code below reads forward till the next \0 and the following while will continue iterating until
                // the entire buffer has been used.
                while (*driveIt++);
            } while (*driveIt);

            DriveList driveList;
            driveList.reserve(driveMappings.size());
            for (auto& drive : driveMappings)
            {
                driveList.push_back(VStd::move(drive.second));
            }
            hardwareInfo.Profile = driveList.size() == 1 ? driveList.front().Profile : "Generic";
            hardwareInfo.PlatformData = VStd::make_any<DriveList>(VStd::move(driveList));

            return !driveList.empty();
        }
        else
        {
            return false;
        }
    }

    bool CollectIoHardwareInformation(HardwareInformation& info, bool includeAllHardware, bool reportHardware)
    {
        if (!CollectHardwareInfo(info, includeAllHardware, reportHardware))
        {
            // The numbers below are based on common defaults from a local hardware survey.
            info.MaxPageSize = 4096;
            info.MaxTransfer = 512_kib;
            info.MaxPhysicalSectorSize = 4096;
            info.MaxLogicalSectorSize = 512;
            info.Profile = "Generic";
        }
        return true;
    }
} // namespace V::IO
