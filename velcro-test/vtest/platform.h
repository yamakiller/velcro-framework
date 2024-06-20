#ifndef V_FRAMEWORK_VTEST_PLATFORM_H
#define V_FRAMEWORK_VTEST_PLATFORM_H

#include <memory>
#include <string>
#include <vtest/utils.h>

namespace V {
    namespace Test {
        const int INCORRECT_USAGE  = 101;
        const int LIB_NOT_FOUND    = 102;
        const int SYMBOL_NOT_FOUND = 103;

        const int MAX_PRINT_MSG = 4096;

        struct IFunctionHandle;

        struct IModuleHandle {
            virtual ~IModuleHandle() = default;
            virtual bool IsValid() = 0;
            virtual std::shared_ptr<IFunctionHandle> GetFunction(const std::string& name) = 0;
        };

        struct IFunctionHandle {
            virtual ~IFunctionHandle() = default;

            //! Is the function handle valid (was it found inside the module correctly)?
            virtual bool IsValid() = 0;

            //! call as a "main" function
            virtual int operator()(int argc, char** argv) = 0;

            //! call as simple function
            virtual int operator()() = 0;
        };

        class Platform {
        public:
            bool SupportsWaitForDebugger();
            void WaitForDebugger();
            void SuppressPopupWindows();
            std::shared_ptr<IModuleHandle> GetModule(const std::string& lib);
            std::string GetModuleNameFromPath(const std::string& path);
            void Printf(const char* format, ...);
        };

         Platform& GetPlatform();
    }
}

#endif // V_FRAMEWORK_VTEST_PLATFORM_H