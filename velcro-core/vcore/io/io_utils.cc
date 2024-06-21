#include <vcore/base.h>
#include <vcore/io/file_io.h>
#include <vcore/io/system_file.h>
#include <vcore/std/parallel/thread.h> /// this_thread sleep_for.

namespace V {
    namespace IO
    {      
        int TranslateOpenModeToSystemFileMode(const char* path, OpenMode mode)
        {
            int systemFileMode = 0;
            bool read = AnyFlag(mode & OpenMode::ModeRead) || AnyFlag(mode & OpenMode::ModeUpdate);
            bool write = AnyFlag(mode & OpenMode::ModeWrite) || AnyFlag(mode & OpenMode::ModeUpdate) || AnyFlag(mode & OpenMode::ModeAppend);
            if (write)
            {
                // If writing the file, create the file in all cases (except r+)
                if (!SystemFile::Exists(path) && !(AnyFlag(mode & OpenMode::ModeRead) && AnyFlag(mode & OpenMode::ModeUpdate)))
                {
                    // LocalFileIO creates by default
                    systemFileMode |= SystemFile::SF_OPEN_CREATE;
                }

                if (AnyFlag(mode & OpenMode::ModeCreatePath))
                {
                    systemFileMode |= SystemFile::SF_OPEN_CREATE_PATH;
                }

                // If appending, append.
                if (AnyFlag(mode & OpenMode::ModeAppend))
                {
                    systemFileMode |= SystemFile::SF_OPEN_APPEND;
                }
                // If writing and not appending, empty the file
                else if (AnyFlag(mode & OpenMode::ModeWrite))
                {
                    systemFileMode |= SystemFile::SF_OPEN_TRUNCATE;
                }

                // If reading, set read/write, otherwise just write
                if (read)
                {
                    systemFileMode |= SystemFile::SF_OPEN_READ_WRITE;
                }
                else
                {
                    systemFileMode |= SystemFile::SF_OPEN_WRITE_ONLY;
                }
            }
            else if (read)
            {
                systemFileMode |= SystemFile::SF_OPEN_READ_ONLY;
            }

            return systemFileMode;
        }

        bool RetryOpenStream(FileIOStream& stream, int numRetries, int delayBetweenRetry)
        {
            while ((!stream.IsOpen()) && (numRetries > 0))
            {
                numRetries--;
                VStd::this_thread::sleep_for(VStd::chrono::milliseconds(delayBetweenRetry));
                stream.ReOpen();
            }
            return stream.IsOpen();
        }
    }   // namespace IO
}   // namespace V