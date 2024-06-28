#include <vcore/native_ui/native_ui_system.h>

namespace V::NativeUI
{
    NativeUISystem::NativeUISystem()
    {
        NativeUIRequestBus::Handler::BusConnect();
    }

    NativeUISystem::~NativeUISystem()
    {
        NativeUIRequestBus::Handler::BusDisconnect();
    }

    AssertAction NativeUISystem::DisplayAssertDialog(const VStd::string& message) const
    {
        if (m_mode == NativeUI::Mode::DISABLED)
        {
            return AssertAction::NONE;
        }

        static const char* buttonNames[3] = { "Ignore", "Ignore All", "Break" };
        VStd::vector<VStd::string> options;
        options.push_back(buttonNames[0]);
#if V_TRAIT_SHOW_IGNORE_ALL_ASSERTS_OPTION
        options.push_back(buttonNames[1]);
#endif
        options.push_back(buttonNames[2]);

        VStd::string result = DisplayBlockingDialog("Assert Failed!", message, options);

        if (result.compare(buttonNames[0]) == 0)
        {
            return AssertAction::IGNORE_ASSERT;
        }
        else if (result.compare(buttonNames[1]) == 0)
        {
            return AssertAction::IGNORE_ALL_ASSERTS;
        }
        else if (result.compare(buttonNames[2]) == 0)
        {
            return AssertAction::BREAK;
        }

        return AssertAction::NONE;
    }

    VStd::string NativeUISystem::DisplayOkDialog(const VStd::string& title, const VStd::string& message, bool showCancel) const
    {
        if (m_mode == NativeUI::Mode::DISABLED)
        {
            return {};
        }

        VStd::vector<VStd::string> options{ "OK" };

        if (showCancel)
        {
            options.push_back("Cancel");
        }

        return DisplayBlockingDialog(title, message, options);
    }

    VStd::string NativeUISystem::DisplayYesNoDialog(const VStd::string& title, const VStd::string& message, bool showCancel) const
    {
        if (m_mode == NativeUI::Mode::DISABLED)
        {
            return {};
        }

        VStd::vector<VStd::string> options{ "Yes", "No" };

        if (showCancel)
        {
            options.push_back("Cancel");
        }

        return DisplayBlockingDialog(title, message, options);
    }
} // namespace V::NativeUI