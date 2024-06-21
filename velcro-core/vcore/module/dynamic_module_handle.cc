#include <vcore/module/dynamic_module_handle.h>
#include <vcore/module/environment.h>

namespace V {
    DynamicModuleHandle::DynamicModuleHandle(const char* fileFullName) : m_fileName(fileFullName) {
    }

    bool DynamicModuleHandle::Load(bool isInitializeFunctionRequired) {
        LoadStatus status = loadModule();
        switch(status) {
            case LoadStatus::LoadFailure:
                return false; 
            case LoadStatus::AlreadyLoaded:
            case LoadStatus::LoadSuccess:
                break;
        }

        auto initFunc = GetFunction<InitializeDynamicModuleFunction>(InitializeDynamicModuleFunctionName);
        if (initFunc) {
            OSString variableName = OSString::format("module_init_token: %s", m_fileName.c_str());
            m_initialized = V::Environment::FindVariable<bool>(variableName.c_str());
            if (!m_initialized) {
                // if we get here, it means nobody has initialized this module yet.  We will initialize it and create the variable.
                m_initialized = V::Environment::CreateVariable<bool>(variableName.c_str());
                initFunc();
            }
        } else if (isInitializeFunctionRequired) {
            V_Error("Module", false, "Unable to locate required entry point '%s' within module '%s'.",
                InitializeDynamicModuleFunctionName, m_fileName.c_str());
            Unload();
            return false;
        }
        return true;
    }

    bool DynamicModuleHandle::Unload() {
        if (!IsLoaded()) return false;
        if (m_initialized) {
            if (m_initialized.IsOwner()) {
                auto uninitFunc = GetFunction<UninitializeDynamicModuleFunction>(UninitializeDynamicModuleFunctionName);
                V_Error("Module", uninitFunc, "Unable to locate required entry point '%s' within module '%s'.",
                    UninitializeDynamicModuleFunctionName, m_fileName.c_str());
                if (uninitFunc) {
                    uninitFunc();
                }
            }

            m_initialized.Reset();
        }

        if (!unloadModule()) {
            V_Error("Module", false, "Failed to unload module at \"%s\".", m_fileName.c_str());
            return false;
        }
        return true;
    }
}