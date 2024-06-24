#include <vcore/io/system_file.h>
#include <vcore/io/file_io.h>

#include <vcore/platform_incl.h>
#include <vcore/std/functional.h>
#include <vcore/std/typetraits/typetraits.h>

#include <stdio.h>

namespace V::IO
{

namespace Platform
{
    // Forward declaration of platform specific implementations

    using FileHandleType = SystemFile::FileHandleType;

    void Seek(FileHandleType handle, const SystemFile* systemFile, SystemFile::SeekSizeType offset, SystemFile::SeekMode mode);
    SystemFile::SizeType Tell(FileHandleType handle, const SystemFile* systemFile);
    bool Eof(FileHandleType handle, const SystemFile* systemFile);
    V::u64 ModificationTime(FileHandleType handle, const SystemFile* systemFile);
    SystemFile::SizeType Read(FileHandleType handle, const SystemFile* systemFile, SizeType byteSize, void* buffer);
    SystemFile::SizeType Write(FileHandleType handle, const SystemFile* systemFile, const void* buffer, SizeType byteSize);
    void Flush(FileHandleType handle, const SystemFile* systemFile );
    SystemFile::SizeType Length(FileHandleType handle, const SystemFile* systemFile);

    bool Exists(const char* fileName);
    bool IsDirectory(const char* filePath);
    void FindFiles(const char* filter, SystemFile::FindFileCB cb);
    V::u64 ModificationTime(const char* fileName);
    SystemFile::SizeType Length(const char* fileName);
    bool Delete(const char* fileName);
    bool Rename(const char* sourceFileName, const char* targetFileName, bool overwrite);
    bool IsWritable(const char* sourceFileName);
    bool SetWritable(const char* sourceFileName, bool writable);
    bool CreateDir(const char* dirName);
    bool DeleteDir(const char* dirName);
}

void SystemFile::CreatePath(const char* fileName)
{
    char folderPath[V_MAX_PATH_LEN] = { 0 };
    const char* delimiter1 = strrchr(fileName, '\\');
    const char* delimiter2 = strrchr(fileName, '/');
    const char* delimiter = delimiter2 > delimiter1 ? delimiter2 : delimiter1;
    if (delimiter > fileName)
    {
        v_strncpy(folderPath, V_ARRAY_SIZE(folderPath), fileName, delimiter - fileName);
        if (!Exists(folderPath))
        {
            CreateDir(folderPath);
        }
    }
}

SystemFile::SystemFile()
    : m_handle{ V_TRAIT_SYSTEMFILE_INVALID_HANDLE }
{
}

SystemFile::~SystemFile()
{
    if (IsOpen())
    {
        Close();
    }
}

SystemFile::SystemFile(SystemFile&& other)
    : SystemFile{}
{
    VStd::swap(m_fileName, other.m_fileName);
    VStd::swap(m_handle, other.m_handle);
}

SystemFile& SystemFile::operator=(SystemFile&& other)
{
    // Close the current file and take over the SystemFile handle and filename
    Close();
    m_fileName = VStd::move(other.m_fileName);
    m_handle = VStd::move(other.m_handle);
    other.m_fileName = {};
    other.m_handle = V_TRAIT_SYSTEMFILE_INVALID_HANDLE;

    return *this;
}

bool SystemFile::Open(const char* fileName, int mode, int platformFlags)
{
    if (fileName)       // If we reopen the file we are allowed to have NULL file name
    {
        if (strlen(fileName) > m_fileName.max_size())
        {
            //EBUS_EVENT(FileIOEventBus, OnError, this, nullptr, 0);
            return false;
        }

        // store the filename
        m_fileName = fileName;
    }

    /*if (FileIOBus::HasHandlers())
    {
        bool isOpen = false;
        bool isHandled = false;
        EBUS_EVENT_RESULT(isHandled, FileIOBus, OnOpen, *this, m_fileName.c_str(), mode, platformFlags, isOpen);
        if (isHandled)
        {
            return isOpen;
        }
    }*/

    V_Assert(!IsOpen(), "This file (%s) is already open!", m_fileName.c_str());

    return PlatformOpen(mode, platformFlags);
}

bool SystemFile::ReOpen(int mode, int platformFlags)
{
    V_Assert(!m_fileName.empty(), "Missing filename. You must call open first!");
    return Open(nullptr, mode, platformFlags);
}

void SystemFile::Close()
{
    /*if (FileIOBus::HasHandlers())
    {
        bool isHandled = false;
        EBUS_EVENT_RESULT(isHandled, FileIOBus, OnClose, *this);
        if (isHandled)
        {
            return;
        }
    }*/

    PlatformClose();
}

void SystemFile::Seek(SeekSizeType offset, SeekMode mode)
{
    /*if (FileIOBus::HasHandlers())
    {
        bool isHandled = false;
        EBUS_EVENT_RESULT(isHandled, FileIOBus, OnSeek, *this, offset, mode);
        if (isHandled)
        {
            return;
        }
    }*/

    Platform::Seek(m_handle, this, offset, mode);
}

SystemFile::SizeType SystemFile::Tell() const
{
    return Platform::Tell(m_handle, this);
}

bool SystemFile::Eof() const
{
    return Platform::Eof(m_handle, this);
}

V::u64 SystemFile::ModificationTime()
{
    return Platform::ModificationTime(m_handle, this);
}

SystemFile::SizeType SystemFile::Read(SizeType byteSize, void* buffer)
{
    /*if (FileIOBus::HasHandlers())
    {
        SizeType numRead = 0;
        bool isHandled = false;
        EBUS_EVENT_RESULT(isHandled, FileIOBus, OnRead, *this, byteSize, buffer, numRead);
        if (isHandled)
        {
            return numRead;
        }
    }*/

    return Platform::Read(m_handle, this, byteSize, buffer);
}

SystemFile::SizeType SystemFile::Write(const void* buffer, SizeType byteSize)
{
    /*if (FileIOBus::HasHandlers())
    {
        SizeType numWritten = 0;
        bool isHandled = false;
        EBUS_EVENT_RESULT(isHandled, FileIOBus, OnWrite, *this, buffer, byteSize, numWritten);
        if (isHandled)
        {
            return numWritten;
        }
    }*/

    return Platform::Write(m_handle, this, buffer, byteSize);
}

void SystemFile::Flush()
{
    Platform::Flush(m_handle, this);
}

SystemFile::SizeType SystemFile::Length() const
{
    return Platform::Length(m_handle, this);
}

bool SystemFile::IsOpen() const
{
    return m_handle != V_TRAIT_SYSTEMFILE_INVALID_HANDLE;
}

SystemFile::SizeType SystemFile::DiskOffset() const
{
#if V_TRAIT_DOES_NOT_SUPPORT_FILE_DISK_OFFSET
    #pragma message("--- File Disk Offset is not available ---")
#endif
    return 0;
}

bool SystemFile::Exists(const char* fileName)
{
    return Platform::Exists(fileName);
}

bool SystemFile::IsDirectory(const char* filePath)
{
    return Platform::IsDirectory(filePath);
}

void SystemFile::FindFiles(const char* filter, FindFileCB cb)
{
    Platform::FindFiles(filter, cb);
}

V::u64 SystemFile::ModificationTime(const char* fileName)
{
    return Platform::ModificationTime(fileName);
}

SystemFile::SizeType SystemFile::Length(const char* fileName)
{
    return Platform::Length(fileName);
}

SystemFile::SizeType SystemFile::Read(const char* fileName, void* buffer, SizeType byteSize, SizeType byteOffset)
{
    SizeType numBytesRead = 0;
    SystemFile f;
    if (f.Open(fileName, SF_OPEN_READ_ONLY))
    {
        if (byteSize == 0)
        {
            byteSize = f.Length(); // read the entire file
        }
        if (byteOffset != 0)
        {
            f.Seek(byteOffset, SF_SEEK_BEGIN);
        }

        numBytesRead = f.Read(byteSize, buffer);
        f.Close();
    }

    return numBytesRead;
}

bool SystemFile::Delete(const char* fileName)
{
    if (!Exists(fileName))
    {
        return false;
    }

    return Platform::Delete(fileName);
}

bool SystemFile::Rename(const char* sourceFileName, const char* targetFileName, bool overwrite)
{
    if (!Exists(sourceFileName))
    {
        return false;
    }

    return Platform::Rename(sourceFileName, targetFileName, overwrite);
}

bool SystemFile::IsWritable(const char* sourceFileName)
{
    return Platform::IsWritable(sourceFileName);
}

bool SystemFile::SetWritable(const char* sourceFileName, bool writable)
{
    return Platform::SetWritable(sourceFileName, writable);
}

bool SystemFile::CreateDir(const char* dirName)
{
    return Platform::CreateDir(dirName);
}

bool SystemFile::DeleteDir(const char* dirName)
{
    return Platform::DeleteDir(dirName);
}


namespace
{
#define HasPosixEnumOption(EnumValue) \
    static_assert(std::is_enum_v<decltype(EnumValue)>, "PosixInternal enum option " #EnumValue " not defined for platform " V_TRAIT_OS_PLATFORM_NAME);

    using namespace V::IO::PosixInternal;
    // Verify all posix internal flags have been defined
    // If you happen to add any OpenFlag or PermissionModeFlag, please update this list

    HasPosixEnumOption(OpenFlags::Append);
    HasPosixEnumOption(OpenFlags::Create);
    HasPosixEnumOption(OpenFlags::Temporary);
    HasPosixEnumOption(OpenFlags::Exclusive);
    HasPosixEnumOption(OpenFlags::Truncate);
    HasPosixEnumOption(OpenFlags::ReadOnly);
    HasPosixEnumOption(OpenFlags::WriteOnly);
    HasPosixEnumOption(OpenFlags::ReadWrite);
    HasPosixEnumOption(OpenFlags::NoInherit);
    HasPosixEnumOption(OpenFlags::Random);
    HasPosixEnumOption(OpenFlags::Sequential);
    HasPosixEnumOption(OpenFlags::Binary);
    HasPosixEnumOption(OpenFlags::Text);
    HasPosixEnumOption(OpenFlags::U16Text);
    HasPosixEnumOption(OpenFlags::U8Text);
    HasPosixEnumOption(OpenFlags::WText);
    HasPosixEnumOption(OpenFlags::Async);
    HasPosixEnumOption(OpenFlags::Direct);
    HasPosixEnumOption(OpenFlags::Directory);
    HasPosixEnumOption(OpenFlags::NoFollow);
    HasPosixEnumOption(OpenFlags::NoAccessTime);
    HasPosixEnumOption(OpenFlags::Path);

    HasPosixEnumOption(PermissionModeFlags::None);
    HasPosixEnumOption(PermissionModeFlags::Read);
    HasPosixEnumOption(PermissionModeFlags::Write);

#undef HasPosixEnumOption
}


FileDescriptorRedirector::FileDescriptorRedirector(int sourceFileDescriptor)
    : m_sourceFileDescriptor(sourceFileDescriptor)
{
}

FileDescriptorRedirector::~FileDescriptorRedirector()
{
    Reset();
}

void FileDescriptorRedirector::RedirectTo(VStd::string_view toFileName, Mode mode)
{
    if (m_sourceFileDescriptor == -1)
    {
        return;
    }

    using OpenFlags = PosixInternal::OpenFlags;
    using PermissionModeFlags = PosixInternal::PermissionModeFlags;

    V::IO::FixedMaxPathString redirectPath = V::IO::FixedMaxPathString(toFileName.data(), toFileName.size());
    const OpenFlags createOrAppend = mode == Mode::Create ? (OpenFlags::Create | OpenFlags::Truncate) : OpenFlags::Append;

    int toFileDescriptor = PosixInternal::Open(redirectPath.c_str(),
        createOrAppend | OpenFlags::WriteOnly,
        PermissionModeFlags::Read | PermissionModeFlags::Write);

    if (toFileDescriptor == -1)
    {
        return;
    }

    m_redirectionFileDescriptor = toFileDescriptor;
    // Duplicate the source file descriptor and redirect the opened file descriptor
    // to the source file descriptor
    m_dupSourceFileDescriptor = PosixInternal::Dup(m_sourceFileDescriptor);
    if (PosixInternal::Dup2(m_redirectionFileDescriptor, m_sourceFileDescriptor) == -1)
    {
        // Failed to redirect the newly opened file descriptor to the source file descriptor
        PosixInternal::Close(m_redirectionFileDescriptor);
        PosixInternal::Close(m_dupSourceFileDescriptor);
        m_redirectionFileDescriptor = -1;
        m_dupSourceFileDescriptor = -1;
        return;
    }
}

void FileDescriptorRedirector::Reset()
{
    if (m_redirectionFileDescriptor != -1)
    {
        // Close the redirect file
        PosixInternal::Close(m_redirectionFileDescriptor);
        // Restore the redirected file descriptor back to the original
        PosixInternal::Dup2(m_dupSourceFileDescriptor, m_sourceFileDescriptor);
        PosixInternal::Close(m_dupSourceFileDescriptor);
        m_redirectionFileDescriptor = -1;
        m_dupSourceFileDescriptor = -1;
    }
}

void FileDescriptorRedirector::WriteBypassingRedirect(const void* data, unsigned int size)
{
    int targetFileDescriptor = m_sourceFileDescriptor;
    if (m_redirectionFileDescriptor != -1)
    {
        targetFileDescriptor = m_dupSourceFileDescriptor;
    }
    V::IO::PosixInternal::Write(targetFileDescriptor, data, size);
}

} // namespace V::IO