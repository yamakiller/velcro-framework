#ifndef V_FRAMEWORK_CORE_NATIVEUI_NATIVE_UI_H
#define V_FRAMEWORK_CORE_NATIVEUI_NATIVE_UI_H

#include <vcore/event_bus/event_bus.h>
#include <vcore/std/string/string.h>

namespace V::NativeUI {
    enum class AssertAction {
        IGNORE_ASSERT = 0,
        IGNORE_ALL_ASSERTS,
        BREAK,
        NONE,
    };

    enum class Mode {
        DISABLED = 0,
        ENABLED,
    };

    class NativeUIRequests {
    public:
        VOBJECT_RTTI(NativeUIRequests, "{6e7a61bd-0893-4960-8792-85d2cfce3dd5}");
        virtual ~NativeUIRequests() = default;
          virtual VStd::string DisplayBlockingDialog(
            [[maybe_unused]] const VStd::string& title,
            [[maybe_unused]] const VStd::string& message,
            [[maybe_unused]] const VStd::vector<VStd::string>& options) const
        {
            return {};
        }

        //! Waits for user to select an option ('Ok' or optionally 'Cancel') before execution continues
        //! Returns the option string selected by the user
        virtual VStd::string DisplayOkDialog(
            [[maybe_unused]] const VStd::string& title,
            [[maybe_unused]] const VStd::string& message,
            [[maybe_unused]] bool showCancel) const
        {
            return {};
        }

        //! Waits for user to select an option ('Yes', 'No' or optionally 'Cancel') before execution continues
        //! Returns the option string selected by the user
        virtual VStd::string DisplayYesNoDialog(
            [[maybe_unused]] const VStd::string& title,
            [[maybe_unused]] const VStd::string& message,
            [[maybe_unused]] bool showCancel) const
        {
            return {};
        }

        //! Displays an assert dialog box
        //! Returns the action selected by the user
        virtual AssertAction DisplayAssertDialog([[maybe_unused]] const VStd::string& message) const { return AssertAction::NONE; }

        //! Set the operation mode of the native UI systen
        void SetMode(NativeUI::Mode mode)
        {
            m_mode = mode;
        }

    protected:
        NativeUI::Mode m_mode = NativeUI::Mode::DISABLED;
    };

    class NativeUIEventBusTraits
        : public V::EventBusTraits
    {
    public:
        //////////////////////////////////////////////////////////////////////////
        // EventBusTraits overrides
        static const V::EventBusHandlerPolicy HandlerPolicy = V::EventBusHandlerPolicy::Single;
        static const V::EventBusAddressPolicy AddressPolicy = V::EventBusAddressPolicy::Single;
        using MutexType = VStd::recursive_mutex;
    };

    using NativeUIRequestBus = V::EventBus<NativeUIRequests, NativeUIEventBusTraits>;
} // namespace V::NativeUI

#endif // V_FRAMEWORK_CORE_NATIVEUI_NATIVE_UI_H