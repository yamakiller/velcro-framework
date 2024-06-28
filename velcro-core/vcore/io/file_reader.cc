#include <vcore/io/file_reader.h>
#include <vcore/io/file_io.h>
#include <vcore/io/path/path.h>


namespace V::IO
{
    FileReader::FileReader() = default;

    FileReader::FileReader(V::IO::FileIOBase* fileIoBase, const char* filePath)
    {
        Open(fileIoBase, filePath);
    }

    FileReader::~FileReader()
    {
        Close();
    }

    FileReader::FileReader(FileReader&& other)
    {
        VStd::swap(m_file, other.m_file);
        VStd::swap(m_fileIoBase, other.m_fileIoBase);
    }

    FileReader& FileReader::operator=(FileReader&& other)
    {
        // Close the current file and take over other file
        Close();
        m_file = VStd::move(other.m_file);
        m_fileIoBase = VStd::move(other.m_fileIoBase);
        other.m_file = VStd::monostate{};
        other.m_fileIoBase = {};

        return *this;
    }

    bool FileReader::Open(V::IO::FileIOBase* fileIoBase, const char* filePath)
    {
        // Close file if the FileReader has an instance open
        Close();

        if (fileIoBase != nullptr)
        {
            V::IO::HandleType fileHandle;
            if (fileIoBase->Open(filePath, IO::OpenMode::ModeRead, fileHandle))
            {
                m_file = fileHandle;
                m_fileIoBase = fileIoBase;
                return true;
            }
        }
        else
        {
            V::IO::SystemFile file;
            if (file.Open(filePath, IO::SystemFile::OpenMode::SF_OPEN_READ_ONLY))
            {
                m_file = VStd::move(file);
                return true;
            }
        }

        return false;
    }

    bool FileReader::IsOpen() const
    {
        if (auto fileHandle = VStd::get_if<V::IO::HandleType>(&m_file); fileHandle != nullptr)
        {
            return *fileHandle != V::IO::InvalidHandle;
        }
        else if (auto systemFile = VStd::get_if<V::IO::SystemFile>(&m_file); systemFile != nullptr)
        {
            return systemFile->IsOpen();
        }

        return false;
    }

    void FileReader::Close()
    {
        if (auto fileHandle = VStd::get_if<V::IO::HandleType>(&m_file); fileHandle != nullptr)
        {
            if (V::IO::FileIOBase* fileIo = m_fileIoBase; fileIo != nullptr)
            {
                fileIo->Close(*fileHandle);
            }
        }

        m_file = VStd::monostate{};
        m_fileIoBase = {};
    }

    auto FileReader::Length() const -> SizeType
    {
        if (auto fileHandle = VStd::get_if<V::IO::HandleType>(&m_file); fileHandle != nullptr)
        {
            if (SizeType fileSize{}; m_fileIoBase->Size(*fileHandle, fileSize))
            {
                return fileSize;
            }
        }
        else if (auto systemFile = VStd::get_if<V::IO::SystemFile>(&m_file); systemFile != nullptr)
        {
            return systemFile->Length();
        }

        return 0;
    }

    auto FileReader::Read(SizeType byteSize, void* buffer) -> SizeType
    {
        if (auto fileHandle = VStd::get_if<V::IO::HandleType>(&m_file); fileHandle != nullptr)
        {
            if (SizeType bytesRead{}; m_fileIoBase->Read(*fileHandle, buffer, byteSize, false, &bytesRead))
            {
                return bytesRead;
            }
        }
        else if (auto systemFile = VStd::get_if<V::IO::SystemFile>(&m_file); systemFile != nullptr)
        {
            return systemFile->Read(byteSize, buffer);
        }

        return 0;
    }

    auto FileReader::Tell() const -> SizeType
    {
        if (auto fileHandle = VStd::get_if<V::IO::HandleType>(&m_file); fileHandle != nullptr)
        {
            if (SizeType fileOffset{}; m_fileIoBase->Tell(*fileHandle, fileOffset))
            {
                return fileOffset;
            }
        }
        else if (auto systemFile = VStd::get_if<V::IO::SystemFile>(&m_file); systemFile != nullptr)
        {
            return systemFile->Tell();
        }

        return 0;
    }

    bool FileReader::Seek(V::s64 offset, SeekType type)
    {
        if (auto fileHandle = VStd::get_if<V::IO::HandleType>(&m_file); fileHandle != nullptr)
        {
            return m_fileIoBase->Seek(*fileHandle, offset, type);
        }
        else if (auto systemFile = VStd::get_if<V::IO::SystemFile>(&m_file); systemFile != nullptr)
        {
            systemFile->Seek(offset, static_cast<V::IO::SystemFile::SeekMode>(type));
            return true;
        }

        return false;
    }

    bool FileReader::Eof() const
    {
        if (auto fileHandle = VStd::get_if<V::IO::HandleType>(&m_file); fileHandle != nullptr)
        {
            return m_fileIoBase->Eof(*fileHandle);
        }
        else if (auto systemFile = VStd::get_if<V::IO::SystemFile>(&m_file); systemFile != nullptr)
        {
            return systemFile->Eof();
        }

        return false;
    }

    bool FileReader::GetFilePath(V::IO::FixedMaxPath& filePath) const
    {
        if (auto fileHandle = VStd::get_if<V::IO::HandleType>(&m_file); fileHandle != nullptr)
        {
            V::IO::FixedMaxPathString& pathStringRef = filePath.Native();
            if (m_fileIoBase->GetFilename(*fileHandle, pathStringRef.data(), pathStringRef.capacity()))
            {
                pathStringRef.resize_no_construct(VStd::char_traits<char>::length(pathStringRef.data()));
                return true;
            }
        }
        else if (auto systemFile = VStd::get_if<V::IO::SystemFile>(&m_file); systemFile != nullptr)
        {
            filePath = systemFile->Name();
            return true;
        }

        return false;
    }
}
