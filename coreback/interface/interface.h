#ifndef V_FRAMEWORK_CORE_INTERFACE_INTERFACE_H
#define V_FRAMEWORK_CORE_INTERFACE_INTERFACE_H

#include <vcore/module/environment.h>
#include <vcore/std/parallel/shared_mutex.h>
#include <vcore/std/typetraits/is_constructible.h>
#include <vcore/std/typetraits/is_assignable.h>
#include <vcore/std/typetraits/has_virtual_destructor.h>
#include <vcore/utilitys/type_hash.h>

// 用于跨模块边界访问已注册单例的接口类.
// Example Usage:
// @code{*.cpp}
//      class IClick {
//            public:
//                virtual ~IClick();
//
//                virtual void DoSomething() = 0;
//      };
//
//      class Click : public V::Interface<IClick>::Registrar {
//           public:
//                void DoSomething() override;
//      };
//
//      // used code
//
//      if (IClick* click = V::Interface<ISystem>::Get()) {
//          click->DoSomething();
//      }
//@endcode
//

namespace V {
    template <typename T>
    class Interface final {
        public:
            class Registrar
                : public T {
            public:
                Registrar();
                virtual ~Registrar();
            };
        public:
            // 注册一个指向接口的实例指针. 一次只允许注册一个实例.
            static void Register(T* type);
            // 注销接口
            static void Unregister(T* type);
            // 获取对象接口
            static T* Get();
        private:
            static uint32_t GetVariableName();
        private:
            static EnvironmentVariable<T*> _instance;
            static VStd::shared_mutex      _mutex;
    };

    template <typename T>
    EnvironmentVariable<T*> Interface<T>::_instance;

    template <typename T>
    void Interface<T>::Register(T* type) {
        if (!type) {
            V_Assert(false, "Interface '%s' registering a null pointer!", VObject<T>::Name());
            return;
        }

        if (T* foundType = Get()) {
            V_Assert(false, "Interface '%s' already registered! [Found: %p]", VObject<T>::Name(), foundType);
            return;
        }

        VStd::unique_lock<VStd::shared_mutex> lock(_mutex);
        _instance = Environment::CreateVariable<T*>(GetVariableName());
        _instance.Get() = type;
    }

    template <typename T>
    void Interface<T>::Unregister(T* type) {
        if (!_instance || !_instance.Get()) {
            V_Assert(false, "Interface '%s' not registered on this module!", VObject<T>::Name());
            return;
        }

        if (_instance.Get() != type) {
            V_Assert(false, "Interface '%s' is not the same instance that was registered! [Expected '%p', Found '%p']",
                    VObject<T>::Name(), type, _instance.Get());
            return;
        }

        // Assign the internal pointer to null and release the EnvironmentVariable reference.
        VStd::unique_lock<VStd::shared_mutex> lock(_mutex);
        *_instance = nullptr;
        _instance.Reset();
    }

    template <typename T>
    T* Interface<T>::Get() {
        // First attempt to use the module-static reference; take a read lock to check it.
        // This is the fast path which won't block.
        {
            VStd::shared_lock<VStd::shared_mutex> lock(_mutex);
            if (_instance) {
                return _instance.Get();
            }
        }

        // If the instance doesn't exist (which means we could be in a different module),
        // take the full lock and request it.
        VStd::unique_lock<VStd::shared_mutex> lock(_mutex);
        _instance = Environment::FindVariable<T*>(GetVariableName());
        return _instance ? _instance.Get() : nullptr;
    }

    template <typename T>
    uint32_t Interface<T>::GetVariableName() {
        // Environment variable name is taken from the hash of the Uuid (truncated to 32 bits).
        return static_cast<uint32_t>(TypeHash32(VObject<T>::Name()));
    }

    template <typename T>
    Interface<T>::Registrar::Registrar() {
        Interface<T>::Register(this);
    }

    template <typename T>
    Interface<T>::Registrar::~Registrar() {
        Interface<T>::Unregister(this);
    }
}
#endif // V_FRAMEWORK_CORE_INTERFACE_INTERFACE_H