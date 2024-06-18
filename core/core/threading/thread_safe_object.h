#ifndef V_FRAMEWORK_CORE_THREDING_THREAD_SAFE_OBJECT_H
#define V_FRAMEWORK_CORE_THREDING_THREAD_SAFE_OBJECT_H

#include <core/std/parallel/scoped_lock.h>

namespace V {
    //! @class ThreadSafeObjet
    //! Wraps an object in a thread-safe interface.
    template <typename _TYPE>
    class ThreadSafeObject
    {
    public:

        ThreadSafeObject() = default;
        ThreadSafeObject(const _TYPE& rhs);
        ~ThreadSafeObject() = default;

        //! Assigns a new value to the object under lock.
        //! @param value the new value to set the object instance to
        void operator =(const _TYPE& value);

        //! Implicit conversion to underlying type.
        //! @return a copy of the current value of the object retrieved under lock
        operator _TYPE() const;

    private:

        V_DISABLE_COPY_MOVE(ThreadSafeObject);

        mutable VStd::mutex m_mutex;
        _TYPE m_object;
    };
}

#include <core/threading/thread_safe_object.inl>

#endif // V_FRAMEWORK_CORE_THREDING_THREAD_SAFE_OBJECT_H