#include <core/utilitys/utility.h>
#include <core/io/path/path.h>
#include <core/platform_incl.h>
#include <core/std/optional.h>
#include <core/std/string/fixed_string.h>
#include <core/std/string/conversions.h>

#include <stdlib.h>

namespace V {
    namespace Utilitys {
        void RequestAbnormalTermination() {
            abort();
        }

        GetExecutablePathReturnType GetExecutablePath(char* exeStorageBuffer, size_t exeStorageSize) {
            GetExecutablePathReturnType result;
            result.m_pathIncludesFilename = true;
            // Platform specific get exe path: http://stackoverflow.com/a/1024937
            // https://docs.microsoft.com/en-us/windows/win32/api/libloaderapi/nf-libloaderapi-getmodulefilenamea
            wchar_t pathBufferW[V::IO::MaxPathLength] = { 0 };
            const DWORD pathLen = GetModuleFileNameW(nullptr, pathBufferW, static_cast<DWORD>(VStd::size(pathBufferW)));
            const DWORD errorCode = GetLastError();
            if (pathLen == exeStorageSize && errorCode == ERROR_INSUFFICIENT_BUFFER)
            {
                result.m_pathStored = ExecutablePathResult::BufferSizeNotLargeEnough;
            }
            else if (pathLen == 0 && errorCode != ERROR_SUCCESS)
            {
                result.m_pathStored = ExecutablePathResult::GeneralError;
            }
            else
            {
                size_t utf8PathSize = VStd::to_string_length({ pathBufferW, pathLen });
                if (utf8PathSize >= exeStorageSize)
                {
                    result.m_pathStored = ExecutablePathResult::BufferSizeNotLargeEnough;
                }
                else
                {
                    VStd::to_string(exeStorageBuffer, exeStorageSize, pathBufferW);
                }
            }

            return result;
        }

        VStd::optional<V::IO::FixedMaxPathString> GetDefaultAppRootPath()
        {
            return VStd::nullopt;
        }

        VStd::optional<V::IO::FixedMaxPathString> GetDevWriteStoragePath()
        {
            return VStd::nullopt;
        }

        bool ConvertToAbsolutePath(const char* path, char* absolutePath, V::u64 maxLength)
        {
            char* result = _fullpath(absolutePath, path, maxLength);
            return result != nullptr;
        }
    } // namespace Utilitys
} // namespace V