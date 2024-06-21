#include <vcore/platform_incl.h>
#include <string>
#include <iostream>
#include <vtest/platform.h>
#include <vcore/std/string/conversions.h>


class ModuleHandle : public V::Test::IModuleHandle {
    friend class FunctionHandle;
public:
    explicit ModuleHandle(const std::string& lib)
        : m_libHandle(NULL)
    {
        std::string libext = lib;
        if (!V::Test::EndsWith(libext, ".dll"))
        {
            libext += ".dll";
        }
        m_libHandle = ::LoadLibraryA(libext.c_str());  // LoadLibrary conflicts with CryEngine code, so use LoadLibraryA

        if (m_libHandle == NULL)
        {
            DWORD dw = ::GetLastError();
            std::cerr << "FAILED to load library: " << libext << "; GetLastError() returned " << dw << std::endl;
        }
    }

    ModuleHandle(const ModuleHandle&) = delete;
    ModuleHandle& operator=(const ModuleHandle&) = delete;

    ~ModuleHandle() override
    {
        if (m_libHandle != NULL)
        {
            FreeLibrary(m_libHandle);
        }
    }

    bool IsValid() override { return m_libHandle != NULL; }

    std::shared_ptr<V::Test::IFunctionHandle> GetFunction(const std::string& name) override;
private:
    HINSTANCE m_libHandle;
};

//-------------------------------------------------------------------------------------------------
class FunctionHandle : public V::Test::IFunctionHandle
{
public:
    explicit FunctionHandle(ModuleHandle& module, std::string symbol)
        : m_proc(NULL)
    {
        m_proc = ::GetProcAddress(module.m_libHandle, symbol.c_str());
    }

    FunctionHandle(const FunctionHandle&) = delete;
    FunctionHandle& operator=(const FunctionHandle&) = delete;

    ~FunctionHandle() override = default;

    int operator()(int argc, char** argv) override
    {
        using Fn = int(int, char**);

        Fn* fn = reinterpret_cast<Fn*>(m_proc);
        return (*fn)(argc, argv);
    }

    int operator()() override
    {
        using Fn = int();

        Fn* fn = reinterpret_cast<Fn*>(m_proc);
        return (*fn)();
    }

    bool IsValid() override { return m_proc != NULL; }

private:
    FARPROC m_proc;
};

//-------------------------------------------------------------------------------------------------

std::shared_ptr<V::Test::IFunctionHandle> ModuleHandle::GetFunction(const std::string& name)
{
    return std::make_shared<FunctionHandle>(*this, name);
}


//-------------------------------------------------------------------------------------------------
namespace V
{
    namespace Test
    {
        Platform& GetPlatform()
        {
            static Platform _platform;
            return _platform;
        }

        bool Platform::SupportsWaitForDebugger()
        {
            return true;
        }

        std::shared_ptr<IModuleHandle> Platform::GetModule(const std::string& lib)
        {
            return std::make_shared<ModuleHandle>(lib);
        }

        void Platform::WaitForDebugger()
        {
            while (!::IsDebuggerPresent()) {}
        }

        void Platform::SuppressPopupWindows()
        {
            // use SetErrorMode to disable popup windows in case a required library cannot be found
            DWORD errorMode = ::SetErrorMode(SEM_FAILCRITICALERRORS);
            ::SetErrorMode(errorMode | SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX | SEM_NOOPENFILEERRORBOX);
        }

        std::string Platform::GetModuleNameFromPath(const std::string& path)
        {
            size_t start = path.rfind('\\');
            if (start == std::string::npos)
            {
                start = 0;
            }
            size_t end = path.rfind('.');

            return path.substr(start, end);
        }

        void Platform::Printf(const char* format, ...)
        {
            char message[MAX_PRINT_MSG];

            va_list mark;
            va_start(mark, format);
            v_vsnprintf(message, MAX_PRINT_MSG, format, mark);
            va_end(mark);
            V::Debug::Platform::OutputToDebugger({}, message);
        }
    } // Test
} // V