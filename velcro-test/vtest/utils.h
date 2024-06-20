#ifndef V_FRAMEWORK_VTEST_UTILS_H
#define V_FRAMEWORK_VTEST_UTILS_H

#include <string>
#include <vector>
#include <vcore/io/path/path_fwd.h>
#include <vcore/std/string/string.h>
#include <vcore/outcome/outcome.h>
#include <vtest/printers.h>

namespace V {
    namespace Test {
          /*! Command line parameter functions */
        //! Check to see if the parameters includes the specified parameter
        bool ContainsParameter(int argc, char** argv, const std::string& param);

        //! Performs a deep copy of an array of char* parameters into a new array
        //! New parameters are dynamically allocated
        //! This does not set the size for the output array (assumed to be same as argc)
        void CopyParameters(int argc, char** target, char** source);

        //! Get index of the specified parameter
        int GetParameterIndex(int argc, char** argv, const std::string& param);

        //! Get multi-value parameter list based on a flag (and remove from argv if specified)
        //! Returns a string vector for ease of use and cleanup
        std::vector<std::string> GetParameterList(int& argc, char** argv, const std::string& param, bool removeOnReturn = false);

        //! Get value of the specified parameter (and remove from argv if specified)
        //! This assumes that the value is the next argument after the specified parameter
        std::string GetParameterValue(int& argc, char** argv, const std::string& param, bool removeOnReturn = false);

        //! Remove parameters and shift remaining down, startIndex and endIndex are inclusive
        void RemoveParameters(int& argc, char** argv, int startIndex, int endIndex);

        //! Split a C-string command line into an array of char* parameters (argv/argc)
        //! Char* array is dynamically allocated
        char** SplitCommandLine(int& size /* out */, char* const cmdLine);

        /*! General string functions */
        //! Check if a string has a specific ending substring
        bool EndsWith(const std::string& s, const std::string& ending);

        //! Check if a string has a specific beginning substring
        bool StartsWith(const std::string& s, const std::string& beginning);

        class ScopedAutoTempDirectory {
        public:
            ScopedAutoTempDirectory();
            ~ScopedAutoTempDirectory();

            const char* GetDirectory() const;
            V::IO::Path GetDirectoryAsPath() const;
            V::IO::FixedMaxPath GetDirectoryAsFixedMaxPath() const;
            V::IO::Path Resolve(const char* path) const;
        private:
            char m_tempDirectory[V::IO::MaxPathLength] = { '\0' };
        };
    }
}

#endif // V_FRAMEWORK_VTEST_UTILS_H