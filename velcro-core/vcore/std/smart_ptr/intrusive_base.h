#ifndef V_FRAMEWORK_CORE_STD_SMART_PTR_INTRUSIVE_BASE_H
#define V_FRAMEWORK_CORE_STD_SMART_PTR_INTRUSIVE_BASE_H

#include "intrusive_refcount.h"
#include <vcore/std/parallel/atomic.h>

namespace VStd
{
    /**
     * An intrusive reference counting base class that is compliant with VStd::intrusive_ptr. This
     * version is thread-safe through use of an implicit atomic reference count. Explicit friending of
     * intrusive_ptr is used; the user must use intrusive_ptr to take a reference on the object.
     */
    using intrusive_base = intrusive_refcount<atomic_uint>;

} // namespace VStd


#endif // V_FRAMEWORK_CORE_STD_SMART_PTR_INTRUSIVE_BASE_H