#ifndef V_FRAMEWORK_CORE_NATIVEUI_NATIVE_UI_SYSTEM_H
#define V_FRAMEWORK_CORE_NATIVEUI_NATIVE_UI_SYSTEM_H

#include <vcore/native_ui/native_ui_requests.h>

namespace V::NativeUI {
    class NativeUISystem : public NativeUIRequestBus::Handler
    {
    public:
        VOBJECT_RTTI(NativeUISystem, "{b2ba7b1d-bb65-4a45-b405-a7de5c9f295f}", NativeUIRequests);
        V_CLASS_ALLOCATOR(NativeUISystem, V::OSAllocator, 0);

         NativeUISystem();
        ~NativeUISystem() override;

        VStd::string DisplayBlockingDialog(const VStd::string& title, const VStd::string& message, const VStd::vector<VStd::string>& options) const override;
        VStd::string DisplayOkDialog(const VStd::string& title, const VStd::string& message, bool showCancel) const override;
        VStd::string DisplayYesNoDialog(const VStd::string& title, const VStd::string& message, bool showCancel) const override;
        AssertAction DisplayAssertDialog(const VStd::string& message) const override;
    };
} // namespace V::NativeUI

#endif // V_FRAMEWORK_CORE_NATIVEUI_NATIVE_UI_SYSTEM_H