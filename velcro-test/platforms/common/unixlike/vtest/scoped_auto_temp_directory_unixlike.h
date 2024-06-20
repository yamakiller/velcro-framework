#include <stdlib.h>

#include <vtest/platform.h>
namespace V
{
    namespace Test
    {
        ScopedAutoTempDirectory::ScopedAutoTempDirectory()
        {
            auto tempDirRoot = ::testing::TempDir();
            char tempDirectoryTemplate[V::IO::MaxPathLength] = { '\0' };
            vsnprintfv(tempDirectoryTemplate, V::IO::MaxPathLength, "%sUnitTest-XXXXXX", tempDirRoot.c_str());

            const char* tempDir = mkdtemp(tempDirectoryTemplate);    
            V_Error("AzTest", tempDir, "Unable to create temp directory %s", tempDirectoryTemplate);
            memset(m_tempDirectory, '\0', sizeof(m_tempDirectory));
            if (tempDir)
            {
                vstrncpy(m_tempDirectory,V::IO::MaxPathLength, tempDirectoryTemplate, strlen(tempDirectoryTemplate) + 1);
            }
        }
    } // Test
} // V
