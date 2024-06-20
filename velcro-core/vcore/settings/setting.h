#ifndef V_FRAMEWORK_CORE_SETTINGS_SETTING_H
#define V_FRAMEWORK_CORE_SETTINGS_SETTING_H

#include <vcore/math/uuid.h>
#include <vcore/std/functional.h>
#include <vcore/io/system_file.h>
#include <vcore/interface/interface.h>
#include <vcore/std/containers/fixed_vector.h>
#include <vcore/std/containers/vector.h>
#include <vcore/std/string/string.h>
#include <vcore/std/string/string_view.h>
#include <vcore/string_func/string_func.h>

namespace V {
    class SettingInterface {
    public:
        V_DISABLE_COPY_MOVE(SettingInterface);
        using FixedValueString = V::StringFunc::Path::FixedString;
        struct CommandLineArgumentSettings {
            struct JsonPathValue {
                VStd::string_view Path;
                VStd::string_view Value;
            };

            CommandLineArgumentSettings();

            //! Callback function which is invoked to determine how to split a command line argument
            //! into a JSON path and a JSON value
            using DelimiterFunc = VStd::function<JsonPathValue(VStd::string_view line)>;
            DelimiterFunc DelimiterFunc;
        };
    };
}

#endif // V_FRAMEWORK_CORE_SETTINGS_SETTING_H