#ifndef V_FRAMEWORK_CORE_MODULE_MODULE_H
#define V_FRAMEWORK_CORE_MODULE_MODULE_H

#include <vcore/memory/system_allocator.h>
#include <vcore/module/environment.h>
#include <vcore/interface/interface.h>
#include <vcore/console/console.h>
#include <vcore/console/console_functor.h>

namespace V {
    class Module {
    public:
        VOBJECT_RTTI(Module, "{cafcdc95-7d2a-401e-8e4b-639dc3043651}");
        V_CLASS_ALLOCATOR(Module, V::SystemAllocator, 0);

        Module();
        virtual ~Module();

        // no copy
        Module(const Module&) = delete;
        Module& operator=(const Module&) = delete;
    };
}

#if defined(V_MONOLITHIC_BUILD)
    #define V_DECLARE_MODULE_CLASS(MODULE_NAME, MODULE_CLASSNAME) \
    extern "C" V::Module *CreateModuleClass_##MODULE_NAME() { return vnew MODULE_CLASSNAME; }
#else
    #define V_DECLARE_MODULE_CLASS(MODULE_NAME, MODULE_CLASSNAME) \
    extern "C" V_DLL_EXPORT V::Module* CreateModuleClass() \
    {\
        if (auto console = V::Interface<V::IConsole>::Get(); console != nullptr) \
        {\
            console->LinkDeferredFunctors(V::ConsoleFunctorBase::GetDeferredHead()); \
            console->ExecuteDeferredConsoleCommands(); \
        }\
        return vnew MODULE_CLASSNAME; \
    } \
    extern "C" V_DLL_EXPORT void DestoryModuleClass(V::Module* module) { delete module; }
#endif // V_MONOLITHIC_BUILD

#endif // V_FRAMEWORK_CORE_MODULE_MODULE_H