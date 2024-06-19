#include <core/platform_incl.h>

#include <core/IO/Path/Path.h>
#include <core/std/functional.h>
#include <core/io/system_file.h>
#include <vtest/platform.h>

namespace V
{
    namespace Test
    {
        ScopedAutoTempDirectory::ScopedAutoTempDirectory()
        {
            constexpr const DWORD bufferSize = static_cast<DWORD>(V::IO::MaxPathLength);

            char tempDir[bufferSize] = {0};
            GetTempPathA(bufferSize, tempDir);

            char workingTempPathBuffer[bufferSize] = {'\0'};

            int maxAttempts = 2000; // Prevent an infinite loop by setting an arbitrary maximum attempts at finding an available temp folder name
            while (maxAttempts > 0)
            {
                // Use the system's tick count to base the folder name
                ULONGLONG currentTick = GetTickCount64();
                azsnprintf(workingTempPathBuffer, bufferSize, "%sUnitTest-%X", tempDir, aznumeric_cast<unsigned int>(currentTick));

                // Check if the requested directory name is available and re-generate if it already exists
                bool exists = V::IO::SystemFile::Exists(workingTempPathBuffer);
                if (exists) {
                    Sleep(1);
                    maxAttempts--;
                    continue;
                }
                break;
            }

            V_Error("VelcroTest", maxAttempts > 0, "Unable to determine a temp directory");

            if (maxAttempts > 0)
            {
                // Create the temp directory and track it for deletion
                bool tempDirectoryCreated = V::IO::SystemFile::CreateDir(workingTempPathBuffer);
                if (tempDirectoryCreated)
                {
                    azstrncpy(m_tempDirectory, V::IO::MaxPathLength, workingTempPathBuffer, V::IO::MaxPathLength);
                }
                else
                {
                    V_Error("VelcroTest", false, "Unable to create temp directory %s", workingTempPathBuffer);
                }
            }
        }
    } // Test
} // V
