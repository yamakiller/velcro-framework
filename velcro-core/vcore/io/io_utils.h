#ifndef V_FRAMEWORK_CORE_IO_IO_UTILS_H
#define V_FRAMEWORK_CORE_IO_IO_UTILS_H

namespace V {
    namespace IO
    {        
        class FileIOStream;
        //enum  OpenMode;

        int TranslateOpenModeToSystemFileMode(const char* path, OpenMode mode);

        /**
        * ReOpen a stream until it opens - this can help avoid transient problems where the OS has a brief
        * lock on a file stream.
        * returns false if the stream is not open at the end of the retries.
        */
        bool RetryOpenStream(FileIOStream& stream, int numRetries = 10, int delayBetweenRetry = 250);
    }   // namespace IO
   
}   // namespace V

#endif // V_FRAMEWORK_CORE_IO_IO_UTILS_H