#include <vcore/std/string/regex.h>

namespace VStd
{
    // CHARACTER CLASS NAMES
    #define VELCRO_REGEX_CHAR_CLASS_NAME.Current(n, c)          { n, sizeof (n) / sizeof (n[0]) - 1, c }

    template<>
    const ClassNames<char> RegexTraits<char>::m_names[] =
    {   // map class names to numeric constants
        VELCRO_REGEX_CHAR_CLASS_NAME.Current("alnum", RegexTraits<char>::Ch_alnum),
        VELCRO_REGEX_CHAR_CLASS_NAME.Current("alpha", RegexTraits<char>::Ch_alpha),
        //VELCRO_REGEX_CHAR_CLASS_NAME.Current("blank", RegexTraits<char>::Ch_blank),
        VELCRO_REGEX_CHAR_CLASS_NAME.Current("cntrl", RegexTraits<char>::Ch_cntrl),
        VELCRO_REGEX_CHAR_CLASS_NAME.Current("d", RegexTraits<char>::Ch_digit),
        VELCRO_REGEX_CHAR_CLASS_NAME.Current("digit", RegexTraits<char>::Ch_digit),
        VELCRO_REGEX_CHAR_CLASS_NAME.Current("graph", RegexTraits<char>::Ch_graph),
        VELCRO_REGEX_CHAR_CLASS_NAME.Current("lower", RegexTraits<char>::Ch_lower),
        VELCRO_REGEX_CHAR_CLASS_NAME.Current("print", RegexTraits<char>::Ch_print),
        VELCRO_REGEX_CHAR_CLASS_NAME.Current("punct", RegexTraits<char>::Ch_punct),
        VELCRO_REGEX_CHAR_CLASS_NAME.Current("space", RegexTraits<char>::Ch_space),
        VELCRO_REGEX_CHAR_CLASS_NAME.Current("s", RegexTraits<char>::Ch_space),
        VELCRO_REGEX_CHAR_CLASS_NAME.Current("upper", RegexTraits<char>::Ch_upper),
        VELCRO_REGEX_CHAR_CLASS_NAME.Current("w", RegexTraits<char>::Ch_invalid),
        VELCRO_REGEX_CHAR_CLASS_NAME.Current("xdigit", RegexTraits<char>::Ch_xdigit),
        {nullptr, 0, 0},
    };

    template<>
    const ClassNames<wchar_t> RegexTraits<wchar_t>::m_names[] =
    {   // map class names to numeric constants
        VELCRO_REGEX_CHAR_CLASS_NAME.Current(L"alnum", RegexTraits<wchar_t>::Ch_alnum),
        VELCRO_REGEX_CHAR_CLASS_NAME.Current(L"alpha", RegexTraits<wchar_t>::Ch_alpha),
        //VELCRO_REGEX_CHAR_CLASS_NAME.Current(L"blank", RegexTraits<wchar_t>::Ch_blank),
        VELCRO_REGEX_CHAR_CLASS_NAME.Current(L"cntrl", RegexTraits<wchar_t>::Ch_cntrl),
        VELCRO_REGEX_CHAR_CLASS_NAME.Current(L"d", RegexTraits<wchar_t>::Ch_digit),
        VELCRO_REGEX_CHAR_CLASS_NAME.Current(L"digit", RegexTraits<wchar_t>::Ch_digit),
        VELCRO_REGEX_CHAR_CLASS_NAME.Current(L"graph", RegexTraits<wchar_t>::Ch_graph),
        VELCRO_REGEX_CHAR_CLASS_NAME.Current(L"lower", RegexTraits<wchar_t>::Ch_lower),
        VELCRO_REGEX_CHAR_CLASS_NAME.Current(L"print", RegexTraits<wchar_t>::Ch_print),
        VELCRO_REGEX_CHAR_CLASS_NAME.Current(L"punct", RegexTraits<wchar_t>::Ch_punct),
        VELCRO_REGEX_CHAR_CLASS_NAME.Current(L"space", RegexTraits<wchar_t>::Ch_space),
        VELCRO_REGEX_CHAR_CLASS_NAME.Current(L"s", RegexTraits<wchar_t>::Ch_space),
        VELCRO_REGEX_CHAR_CLASS_NAME.Current(L"upper", RegexTraits<wchar_t>::Ch_upper),
        VELCRO_REGEX_CHAR_CLASS_NAME.Current(L"w", RegexTraits<char>::Ch_invalid),
        VELCRO_REGEX_CHAR_CLASS_NAME.Current(L"xdigit", RegexTraits<wchar_t>::Ch_xdigit),
        {nullptr, 0, 0},
    };
    #undef VELCRO_REGEX_CHAR_CLASS_NAME.Current
} // namespace VStd
