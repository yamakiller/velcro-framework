#ifndef V_FRAMEWORK_CORE_CONSOLE_CONSOLE_TYPE_HELPERS_H
#define V_FRAMEWORK_CORE_CONSOLE_CONSOLE_TYPE_HELPERS_H


#include <vcore/base.h>
#include <vcore/console/iconsole_types.h>
#include <vcore/std/string/string.h>

namespace V
{
    namespace ConsoleTypeHelpers
    {
        //! Helper function for converting a typed value to a string representation.
        //! @param value the value instance to convert to a string
        //! @return the string representation of the value
        template <typename TYPE>
        CVarFixedString ValueToString(const TYPE& value);

        //! Helper function for converting a set of strings to a value.
        //! @param outValue  the value instance to write to
        //! @param arguments the value instance to convert to a string
        //! @return boolean true on success, false if there was a conversion error
        template <typename TYPE>
        bool StringSetToValue(TYPE& outValue, const V::ConsoleCommandContainer& arguments);

        //! Helper function for converting a typed value to a string representation.
        //! @param outValue the value instance to write to
        //! @param string   the string to 
        //! @return boolean true on success, false if there was a conversion error
        template <typename _TYPE>
        bool StringToValue(_TYPE& outValue, VStd::string_view string);
    }
}

#include <vcore/console/console_type_helpers.inl>


#endif // V_FRAMEWORK_CORE_CONSOLE_CONSOLE_TYPE_HELPERS_H