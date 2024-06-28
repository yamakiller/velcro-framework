#ifndef V_FRAMEWORK_CORE_IO_FILE_READER_H
#define V_FRAMEWORK_CORE_IO_FILE_READER_H

#include <vcore/io/path/path_fwd.h>
#include <vcore/io/system_file.h>
#include <vcore/std/containers/variant.h>


namespace V::IO
{
    class FileIOBase;
    enum class SeekType : V::u32;

    //! Structure which encapsulates delegates File Read operations
    //! to either the FileIOBase or SystemFile classes based if a FileIOBase* instance has been supplied
    //! to the FileSystemReader class
    //! the SettingsRegistry option to use FileIO
    class FileReader
    {
        using HandleType = V::u32;
        using FileHandleType = VStd::variant<VStd::monostate, V::IO::SystemFile, HandleType>;
    public:
        using SizeType = V::u64;

        //! Creates FileReader instance in the default state with no file opend
        FileReader();
        ~FileReader();

        //! Creates a new FileReader instance and attempts to open the file at the supplied path
        //! Uses the FileIOBase instance if supplied
        //! @param fileIOBase pointer to fileIOBase instance
        //! @param null-terminated filePath to open
        FileReader(V::IO::FileIOBase* fileIoBase, const char* filePath);

        //! Takes ownership of the supplied FileReader handle
        FileReader(FileReader&& other);

        //! Moves ownership of FileReader handle to this instance
        FileReader& operator=(FileReader&& other);

        //! Opens a File using the FileIOBase instance if non-nullptr
        //! Otherwise fall back to use SystemFile
        //! @param fileIOBase pointer to fileIOBase instance
        //! @param null-terminated filePath to open
        //! @return true if the File is opened successfully
        bool Open(V::IO::FileIOBase* fileIoBase, const char* filePath);

        //! Returns true if a file is currently open
        //! @return true if the file is open
        bool IsOpen() const;

        //! Closes the File
        void Close();

        //! Retrieve the length of the OpenFile
        SizeType Length() const;

        //! Attempts to read up to byte size bytes into the supplied buffer
        //! @param byteSize - Maximum number of bytes to read
        //! @param buffer - Buffer to read bytes into
        //! @returns the number of bytes read if the file is open, otherwise 0
        SizeType Read(SizeType byteSize, void* buffer);

        //! Returns the current file offset
        //! @returns file offset if the file is open, otherwise 0
        SizeType Tell() const;

        //! Seeks within the open file to the offset supplied
        //! @param offset File offset to seek to
        //! @param type parameter to indicate the reference point to start the seek from
        //! @returns true if the file is open and the seek succeeded
        bool Seek(V::s64 offset, SeekType type);

        //! Returns true if the file is open and in the EOF state
        bool Eof() const;

        //! Store the file path of the open file into the output file path parameter
        //! The filePath reference is left unmodified, if the path was not stored
        //! @return true if the filePath was stored
        bool GetFilePath(V::IO::FixedMaxPath& filePath) const;

    private:

        FileHandleType m_file;
        V::IO::FileIOBase* m_fileIoBase{};
    };
}



#endif // V_FRAMEWORK_CORE_IO_FILE_READER_H