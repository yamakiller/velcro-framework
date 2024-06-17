#include <core/utilitys/utility.h>
//#include <core/io/ByteContainerStream.h>
#include <core/io/file_io.h>
#include <core/io/generic_streams.h>
#include <core/io/system_file.h>
#include <core/io/Path/Path.h>
#include <core/std/containers/vector.h>
#include <core/string_func/string_func.h>

namespace V::Utilitys
{
    ExecutablePathResult GetExecutableDirectory(char* exeStorageBuffer, size_t exeStorageSize)
    {
        GetExecutablePathReturnType result = GetExecutablePath(exeStorageBuffer, exeStorageSize);
        if (result.m_pathStored != ExecutablePathResult::Success)
        {
            *exeStorageBuffer = '\0';
            return result.m_pathStored;
        }

        // If it contains the filename, zero out the last path separator character...
        if (result.m_pathIncludesFilename)
        {
            V::IO::PathView exePathView(exeStorageBuffer);
            VStd::string_view exeDirectory = exePathView.ParentPath().Native();
            exeStorageBuffer[exeDirectory.size()] = '\0';
        }

        return result.m_pathStored;
    }

    V::IO::FixedMaxPathString GetExecutableDirectory()
    {
        V::IO::FixedMaxPathString executableDirectory;
        if(GetExecutableDirectory(executableDirectory.data(), executableDirectory.capacity())
            == ExecutablePathResult::Success)
        {
            // Updated the size field within the fixed string by using char_traits to calculate the string length
            executableDirectory.resize_no_construct(VStd::char_traits<char>::length(executableDirectory.data()));
        }

        return executableDirectory;
    }

    VStd::optional<V::IO::FixedMaxPathString> ConvertToAbsolutePath(VStd::string_view path)
    {
        V::IO::FixedMaxPathString absolutePath;
        V::IO::FixedMaxPathString srcPath{ path };
        if (ConvertToAbsolutePath(srcPath.c_str(), absolutePath.data(), absolutePath.capacity()))
        {
            // Fix the size value of the fixed string by calculating the c-string length using char traits
            absolutePath.resize_no_construct(VStd::char_traits<char>::length(absolutePath.data()));
            return srcPath;
        }

        return VStd::nullopt;
    }

    V::IO::FixedMaxPathString GetEngineManifestPath()
    {
        V::IO::FixedMaxPath velcroManifestPath = GetVelcroManifestDirectory();
        if (!velcroManifestPath.empty())
        {
            velcroManifestPath /= "velcro_manifest.json";
        }

        return velcroManifestPath.Native();
    }

    V::IO::FixedMaxPathString GetEnginePath()
    {
        /*if (auto registry = V::SettingsRegistry::Get(); registry != nullptr)
        {
            V::SettingsRegistryInterface::FixedValueString settingsValue;
            if (registry->Get(settingsValue, V::SettingsRegistryMergeUtils::FilePathKey_EngineRootFolder))
            {
                return V::IO::FixedMaxPathString{settingsValue};
            }
        }*/
        return {};
    }

    V::IO::FixedMaxPathString GetProjectPath()
    {
        /*if (auto registry = V::SettingsRegistry::Get(); registry != nullptr)
        {
            V::SettingsRegistryInterface::FixedValueString settingsValue;
            if (registry->Get(settingsValue, V::SettingsRegistryMergeUtils::FilePathKey_ProjectPath))
            {
                return V::IO::FixedMaxPathString{settingsValue};
            }
        }*/
        return {};
    }

    const char* GetProjectName()
    {
        /*if (auto registry = V::SettingsRegistry::Get(); registry != nullptr)
        {
            V::SettingsRegistryInterface::FixedValueString projectNameKey{ V::SettingsRegistryMergeUtils::ProjectSettingsRootKey };
            projectNameKey += "/project_name";
            V::SettingsRegistryInterface::FixedValueString settingsValue;
            if (registry->Get(settingsValue, projectNameKey))
            {
                return settingsValue;
            }
        }*/
        return "undefined";
    }

    V::Outcome<void, VStd::string> WriteFile(VStd::string_view content, VStd::string_view filePath)
    {
        V::IO::FixedMaxPath filePathFixed = filePath; // Because FileIOStream requires a null-terminated string
        V::IO::FileIOStream stream(filePathFixed.c_str(), V::IO::OpenMode::ModeWrite);

        bool success = false;

        if (stream.IsOpen())
        {
            V::IO::SizeType bytesWritten = stream.Write(content.size(), content.data());

            if (bytesWritten == content.size())
            {
                success = true;
            }
        }

        if (success)
        {
            return V::Success();
        }
        else
        {
            return V::Failure(VStd::string::format("Could not write to file '%s'", filePathFixed.c_str()));
        }
    }

    template<typename Container>
    V::Outcome<Container, VStd::string> ReadFile(VStd::string_view filePath, size_t maxFileSize)
    {
        IO::FileIOStream file;
        if (!file.Open(filePath.data(), IO::OpenMode::ModeRead))
        {
            return V::Failure(VStd::string::format("Failed to open '%.*s'.", AZ_STRING_ARG(filePath)));
        }

        V::IO::SizeType length = file.GetLength();

        if (length > maxFileSize)
        {
            return V::Failure(VStd::string{ "Data is too large." });
        }
        else if (length == 0)
        {
            return V::Failure(VStd::string::format("Failed to load '%.*s'. File is empty.", AZ_STRING_ARG(filePath)));
        }

        Container fileContent;
        fileContent.resize(length);
        V::IO::SizeType bytesRead = file.Read(length, fileContent.data());
        file.Close();

        // Resize again just in case bytesRead is less than length for some reason
        fileContent.resize(bytesRead);

        return V::Success(VStd::move(fileContent));
    }

    template V::Outcome<VStd::string, VStd::string> ReadFile(VStd::string_view filePath, size_t maxFileSize);
    template V::Outcome<VStd::vector<int8_t>, VStd::string> ReadFile(VStd::string_view filePath, size_t maxFileSize);
    template V::Outcome<VStd::vector<uint8_t>, VStd::string> ReadFile(VStd::string_view filePath, size_t maxFileSize);

    V::IO::FixedMaxPathString GetVelcroManifestDirectory()
    {
        V::IO::FixedMaxPath path = GetHomeDirectory();
        path /= ".velcro";
        return path.Native();
    }

    V::IO::FixedMaxPathString GetVelcroLogsDirectory()
    {
        V::IO::FixedMaxPath path = GetVelcroManifestDirectory();
        path /= "Logs";
        return path.Native();
    }
}