#include <vcore/std/containers/fixed_vector.h>
#include <vcore/std/functional.h>
#include <vcore/std/string/fixed_string.h>
#include <vcore/std/string/conversions.h>
#include <vcore/string_func/string_func.h>


namespace V
{
    namespace ConsoleTypeHelpers
    {
        inline VStd::string ConvertString(const CVarFixedString& value)
        {
            return VStd::string(value.c_str(), value.size());
        }

        inline V::CVarFixedString ConvertString(const VStd::string& value)
        {
            return V::CVarFixedString(value.c_str(), value.size());
        }

        template <typename TYPE>
        inline CVarFixedString ValueToString(const TYPE& value)
        {
            return ConvertString(VStd::to_string(value));
        }

        template <>
        inline CVarFixedString ValueToString(const bool& value)
        {
            return value ? "true" : "false";
        }

        template <>
        inline CVarFixedString ValueToString(const char& value)
        {
            return CVarFixedString(1, value);
        }

        template <>
        inline CVarFixedString ValueToString<V::CVarFixedString>(const V::CVarFixedString& value)
        {
            return value;
        }

        template <>
        inline CVarFixedString ValueToString<VStd::string>(const VStd::string& value)
        {
            return ConvertString(value);
        }

        template <>
        inline bool StringSetToValue<bool>(bool& outValue, const V::ConsoleCommandContainer& arguments)
        {
            if (!arguments.empty())
            {
                V::CVarFixedString lower{ arguments.front() };
                VStd::to_lower(lower.begin(), lower.end());

                if ((lower == "false") || (lower == "0"))
                {
                    outValue = false;
                    return true;
                }
                else if ((lower == "true") || (lower == "1"))
                {
                    outValue = true;
                    return true;
                }
                else
                {
                    V_Warning("Velcro Console", false, "Invalid input for boolean variable");
                }
            }
            return false;
        }

        template <>
        inline bool StringSetToValue<char>(char& outValue, const V::ConsoleCommandContainer& arguments)
        {
            if (!arguments.empty())
            {
                VStd::string_view frontArg = arguments.front();
                outValue = frontArg[0];
            }
            else
            {
                // An empty string is actually a null character '\0'
                outValue = '\0';
            }
            return true;
        }

        // Note that string-stream doesn't behave nicely with char and uint8_t..
        // So instead we string-stream to a very large type that does work, and downcast if the result is within the limits of the target type
        template <typename TYPE, typename MAX_TYPE>
        inline bool StringSetToIntegralValue(TYPE& outValue, const V::ConsoleCommandContainer& arguments, [[maybe_unused]] const char* typeName, [[maybe_unused]] const char* errorString)
        {
            if (!arguments.empty())
            {
                V::CVarFixedString convertCandidate{ arguments.front() };
                char* endPtr = nullptr;
                MAX_TYPE value;
                if constexpr (VStd::is_unsigned_v<MAX_TYPE>)
                {
                    value = vnumeric_cast<MAX_TYPE>(strtoull(convertCandidate.c_str(), &endPtr, 0));
                }
                else
                {
                    value = vnumeric_cast<MAX_TYPE>(strtoll(convertCandidate.c_str(), &endPtr, 0));
                }

                if (endPtr == convertCandidate.c_str())
                {
                    V_Warning("Velcro Console", false, "stringstream failed to convert value %s to %s type\n", convertCandidate.c_str(), typeName);
                    return false;
                }

                //Note: std::numeric_limits<>::min and max have extra parentheses here to min/max being evaluated as macros, to avoid colliding with min/max in Windows.h
                if ((value >= (std::numeric_limits<TYPE>::min)()) && (value <= (std::numeric_limits<TYPE>::max)()))
                {
                    outValue = static_cast<TYPE>(value);
                    return true;
                }
                else
                {
                    V_Warning("Velcro Console", false, errorString, static_cast<MAX_TYPE>(outValue));
                }
            }

            return false;
        }
    }
}