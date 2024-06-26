#ifndef V_FRAMEWORK_CORE_UNIT_TEST_MOCKS_MOCK_FILE_IO_BASE_H
#define V_FRAMEWORK_CORE_UNIT_TEST_MOCKS_MOCK_FILE_IO_BASE_H

#include <vcore/unit_test/unit_test.h>
#include <vcore/io/file_io.h>
#include <vcore/io/path/path.h>
#include <gmock/gmock.h>

// the following was generated using google's python script to autogenerate mocks.

namespace V
{
    namespace IO
    {
        class MockFileIOBase;

        using NiceFileIOBaseMock = ::testing::NiceMock<MockFileIOBase>;

        class MockFileIOBase : public FileIOBase {
        public:
            static const V::IO::HandleType FakeFileHandle = V::IO::HandleType(1234);

            MOCK_METHOD3(Open,  Result(const char* filePath, OpenMode mode, HandleType & fileHandle));
            MOCK_METHOD1(Close, Result(HandleType fileHandle));
            MOCK_METHOD2(Tell,  Result(HandleType fileHandle, V::u64& offset));
            MOCK_METHOD3(Seek,  Result(HandleType fileHandle, V::s64 offset, SeekType type));
            MOCK_METHOD5(Read,  Result(HandleType, void*, V::u64, bool, V::u64*));
            MOCK_METHOD4(Write, Result(HandleType, const void*, V::u64, V::u64*));
            MOCK_METHOD1(Flush, Result(HandleType fileHandle));
            MOCK_METHOD1(Eof,   bool(HandleType fileHandle));
            MOCK_METHOD1(ModificationTime, V::u64(HandleType fileHandle));
            MOCK_METHOD1(ModificationTime, V::u64(const char* filePath));
            MOCK_METHOD2(Size, Result(const char* filePath, V::u64& size));
            MOCK_METHOD2(Size, Result(HandleType fileHandle, V::u64& size));
            MOCK_METHOD1(Exists, bool(const char* filePath));
            MOCK_METHOD1(IsDirectory, bool(const char* filePath));
            MOCK_METHOD1(IsReadOnly,  bool(const char* filePath));
            MOCK_METHOD1(CreatePath,  Result(const char* filePath));
            MOCK_METHOD1(DestroyPath, Result(const char* filePath));
            MOCK_METHOD1(Remove,      Result(const char* filePath));
            MOCK_METHOD2(Copy,        Result(const char* sourceFilePath, const char* destinationFilePath));
            MOCK_METHOD2(Rename,      Result(const char* originalFilePath, const char* newFilePath));
            MOCK_METHOD3(FindFiles,   Result(const char* filePath, const char* filter, FindFilesCallbackType callback));
            MOCK_METHOD2(SetAlias,    void(const char* alias, const char* path));
            MOCK_METHOD1(ClearAlias,  void(const char* alias));
            MOCK_CONST_METHOD1(GetAlias,    const char*(const char* alias));
            MOCK_METHOD2(SetDeprecatedAlias, void(VStd::string_view, VStd::string_view));
            MOCK_CONST_METHOD2(ConvertToAlias, VStd::optional<V::u64>(char* inOutBuffer, V::u64 bufferLength));
            MOCK_CONST_METHOD2(ConvertToAlias, bool(V::IO::FixedMaxPath& aliasPath, const V::IO::PathView& path));
            MOCK_CONST_METHOD3(ResolvePath, bool(const char* path, char* resolvedPath, V::u64 resolvedPathSize));
            MOCK_CONST_METHOD2(ResolvePath, bool(V::IO::FixedMaxPath& resolvedPath, const V::IO::PathView& path));
            MOCK_CONST_METHOD2(ReplaceAlias, bool(V::IO::FixedMaxPath& replacedAliasPath, const V::IO::PathView& path));
            MOCK_CONST_METHOD3(GetFilename, bool(HandleType fileHandle, char* filename, V::u64 filenameSize));
            
            using FileIOBase::ConvertToAlias;
            using FileIOBase::ResolvePath;

            /** Installs some default result values for the above functions.
            *   Note that you can always override these in scope of your test by adding additional ON_CALL / EXPECT_CALL
            *   statements in the body of your test or after calling this function, and yours will take precidence.
            **/ 
            static void InstallDefaultReturns(NiceFileIOBaseMock& target)
            {
                using namespace ::testing;
           
                ON_CALL(target, FindFiles(_, _, _))
                    .WillByDefault(
                    Return(V::IO::Result(V::IO::ResultCode::Success)));

                // pretend that Open always succeeds, and sets the fake file handle.
                ON_CALL(target, Open(_, _, _))
                    .WillByDefault(
                    DoAll(
                        SetArgReferee<2>(FakeFileHandle),
                        Return(V::IO::Result(V::IO::ResultCode::Success))));

                ON_CALL(target, CreatePath(_))
                    .WillByDefault(
                    Return(V::IO::Result(V::IO::ResultCode::Success)));

                ON_CALL(target, Remove(_))
                    .WillByDefault(
                    Return(V::IO::Result(V::IO::ResultCode::Success)));

                ON_CALL(target, Eof(_))
                    .WillByDefault(
                    Return(true));

                ON_CALL(target, Close(_))
                    .WillByDefault(
                    Return(V::IO::Result(V::IO::ResultCode::Success)));

                ON_CALL(target, Copy(_, _))
                    .WillByDefault(
                    Return(V::IO::Result(V::IO::ResultCode::Success)));

                ON_CALL(target, Write(_, _, _, _))
                    .WillByDefault(
                    Return(V::IO::Result(V::IO::ResultCode::Success)));

                ON_CALL(target, Flush(_))
                    .WillByDefault(
                        Return(V::IO::Result(V::IO::ResultCode::Success)));

                ON_CALL(target, Read(_, _, _, _, _))
                    .WillByDefault(
                    Return(V::IO::Result(V::IO::ResultCode::Error)));

                ON_CALL(target, Seek(_, _, _))
                    .WillByDefault(
                        Return(V::IO::Result(V::IO::ResultCode::Success)));

                ON_CALL(target, Size(An<const char*>(), _))
                    .WillByDefault(
                        Return(V::IO::Result(V::IO::ResultCode::Success)));

                ON_CALL(target, Size(An<HandleType>(), _))
                    .WillByDefault(
                        Return(V::IO::Result(V::IO::ResultCode::Success)));
            }
        };
    } // namespace IO
} // namespace V



#endif // V_FRAMEWORK_CORE_UNIT_TEST_MOCKS_MOCK_FILE_IO_BASE_H