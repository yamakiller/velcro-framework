#include <vcore/settings/setting.h>
#include <vcore/std/string/conversions.h>

namespace V {
    SettingInterface::CommandLineArgumentSettings::CommandLineArgumentSettings()
    {
        DelimiterFunc = [](VStd::string_view line) -> JsonPathValue
        {
            constexpr VStd::string_view CommandLineArgumentDelimiters{ "=:" };
            JsonPathValue pathValue;
            pathValue.Value = line;

            // Splits the line on the first delimiter and stores that in the pathValue.m_path variable
            // The StringFunc::TokenizeNext function updates the pathValue.m_value parameter in place
            // to contain all the text after the first delimiter
            // So if pathValue.m_value="foo = Hello Ice Cream=World:17", the call to TokenizeNext would
            // split the value as follows
            // pathValue.m_path = "foo"
            // pathValue.m_value = "Hello Ice Cream=World:17"
            if (auto path = V::StringFunc::TokenizeNext(pathValue.Value, CommandLineArgumentDelimiters); path.has_value())
            {
                pathValue.Path = V::StringFunc::StripEnds(*path);
            }
            pathValue.Value = V::StringFunc::StripEnds(pathValue.Value);
            return pathValue;
        };
    }
}