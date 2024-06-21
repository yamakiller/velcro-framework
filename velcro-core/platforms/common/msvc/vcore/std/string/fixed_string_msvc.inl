namespace VStd
{
    template<class Element, size_t MaxElementCount, class Traits>
    inline decltype(auto) basic_fixed_string<Element, MaxElementCount, Traits>::format_arg(const wchar_t* format, va_list argList)
    {
        basic_fixed_string<wchar_t, MaxElementCount, char_traits<wchar_t>> result;
        int len = _vscwprintf(format, argList);
        if (len > 0)
        {
            result.resize(len);
            len = v_vsnwprintf(result.data(), result.capacity() + 1, format, argList);
            V_Assert(len == static_cast<int>(result.size()), "v_vsnwprintf failed!");
        }

        return result;
    }
}
