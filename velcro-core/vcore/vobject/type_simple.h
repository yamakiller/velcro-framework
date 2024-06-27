#ifndef V_FRAMEWORK_CORE_RTTI_TYPESIMPLE_H
#define V_FRAMEWORK_CORE_RTTI_TYPESIMPLE_H

#include <vcore/math/uuid.h>
#include <vcore/std/typetraits/is_enum.h>

namespace V {
    using TypeId = V::Uuid;
}

#define VOBJECT_INTERNAL_1(_ClassName) static_assert(false, "You must provide a ClassName,ClassUUID")
#define VOBJECT_INTERNAL_2(_ClassName, _ClassUuid)        \
    void VOBJECT_Enable(){}                                   \
    static const char* VOBJECT_Name() { return #_ClassName; } \
    static const V::TypeId& VOBJECT_Id() { static V::TypeId _uuid(_ClassUuid); return _uuid; }

#define VOBJECT_1 VOBJECT_INTERNAL_1
#define VOBJECT_2 VOBJECT_INTERNAL_2

#define VOBJECT(...) V_MACRO_SPECIALIZE(VOBJECT_, V_VA_NUM_ARGS(__VA_ARGS__), (__VA_ARGS__))

#endif // V_FRAMEWORK_CORE_RTTI_TYPESIMPLE_H