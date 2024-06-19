


#ifndef V_FRAMEWORK_CORE_MATH_GUID_H
#define V_FRAMEWORK_CORE_MATH_GUID_H 1

#if defined(GUID_DEFINED)
#define GUID_FORMAT_DATA1 "lX"
#else
#define GUID_DEFINED

#include <vcore/std/containers/array.h>

struct _GUID {
    uint32_t  Data1;
    unsigned short Data2;
    unsigned short Data3;
    VStd::array<unsigned char,8> Data4;
};
using GUID = _GUID;
#define GUID_FORMAT_DATA1 "X"
#endif // GUID_DEFINED

#if !defined _SYS_GUID_OPERATOR_EQ_ && !defined _NO_SYS_GUID_OPERATOR_EQ_
#define _SYS_GUID_OPERATOR_EQ_
static bool inline operator==(const _GUID& lhs, const _GUID& rhs)
{
    return lhs.Data1 == rhs.Data1 &&
           lhs.Data2 == rhs.Data2 &&
           lhs.Data3 == rhs.Data3 &&
           lhs.Data4 == rhs.Data4;
}
static bool inline operator!=(const _GUID& lhs, const _GUID& rhs)
{
    return !operator==(lhs, rhs);
}
#endif //  !defined _SYS_GUID_OPERATOR_EQ_

#if V_TRAIT_COMPILER_DEFINE_REFGUID

#ifndef _REFGUID_DEFINED
#define _REFGUID_DEFINED
#define REFGUID const GUID &
#endif

# ifndef IID_DEFINED
# define IID_DEFINED
typedef GUID IID;
typedef const IID& REFIID;
# endif

#ifndef _REFIID_DEFINED
#define _REFIID_DEFINED
typedef const GUID& REFIID;
#endif

#define DEFINE_GUID(name, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8) \
const GUID name \
= { l, w1, w2, { b1, b2,  b3,  b4,  b5,  b6,  b7,  b8 } }

inline constexpr GUID GUID_NULL()
{
    return { 0x00000000L, 0x0000, 0x0000, {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00} };
}

#define GUID_NULL GUID_NULL()

#endif // V_TRAIT_COMPILER_DEFINE_REFGUID

#endif // V_FRAMEWORK_CORE_MATH_GUID_H