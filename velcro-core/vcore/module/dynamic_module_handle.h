#ifndef V_FRAMEWORK_CORE_MODULE_DYNAMIC_MODULE_HANDLE_H
#define V_FRAMEWORK_CORE_MODULE_DYNAMIC_MODULE_HANDLE_H

#include <vcore/std/smart_ptr/unique_ptr.h>
#include <vcore/std/string/fixed_string.h>
#include <vcore/io/path/path_fwd.h>
#include <vcore/module/environment.h>

namespace V {
    class DynamicModuleHandle {
    protected:
        enum class LoadStatus {
            LoadSuccess,
            LoadFailure,
            AlreadyLoaded
        };
    public:
        /// Platform-specific implementation should call Unload().
        virtual ~DynamicModuleHandle() = default;

        // no copy
        DynamicModuleHandle(const DynamicModuleHandle&) = delete;
        DynamicModuleHandle& operator=(const DynamicModuleHandle&) = delete;

        /// Creates a platform-specific DynamicModuleHandle.
        /// Note that the specified module is not loaded until \ref Load is called.
        static VStd::unique_ptr<DynamicModuleHandle> Create(const char* fullFileName);


        bool Load(bool isInitializeFunctionRequired);
        bool Unload();
        virtual bool IsLoaded() const = 0;
        const char*  GetFilename() const { return m_fileName.c_str(); }

        template<typename Function>
        Function GetFunction(const char* functionName) const {
            if (IsLoaded()) {
                return reinterpret_cast<Function>(GetFunctionAddress(functionName));
            } else {
                return nullptr;
            }
        }
    protected:
        DynamicModuleHandle(const char* fileFullName);

        // Attempt to load a module.
        virtual LoadStatus loadModule() = 0;
        virtual bool       unloadModule() = 0;
        virtual void*      getFunctionAddress(const char* functionName) const = 0;
    protected:
        V::IO::FixedMaxPathString    m_fileName;
        V::EnvironmentVariable<bool> m_initialized;
    };

    using InitializeDynamicModuleFunction = void(*)();
    const char InitializeDynamicModuleFunctionName[] = "InitializeDynamicModule";

    using UninitializeDynamicModuleFunction = void(*)();
    const char UninitializeDynamicModuleFunctionName[] = "UninitializeDynamicModule";
}

#endif // V_FRAMEWORK_CORE_MODULE_DYNAMIC_MODULE_HANDLE_H