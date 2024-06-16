#ifndef V_FRAMEWORK_CORE_INTERFACE_INTERFACE_H
#define V_FRAMEWORK_CORE_INTERFACE_INTERFACE_H

#include <core/module/environment.h>
#include <core/std/parallel/shared_mutex.h>
#include <core/std/typetraits/is_constructible.h>
#include <core/std/typetraits/is_assignable.h>
#include <core/std/typetraits/has_virtual_destructor.h>
#include <core/utilitys/type_hash.h>

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
            static void Register(const char*name, T* type);
            // 注销接口
            static void Unregister(const char* name);
            // 获取对象接口
            static T* Get(const char* name);
        private:
            static uint32_t GetVariableName(const char* name);
        private:
            static EnvironmentVariable<T*> _instance;
            static VStd::shared_mutex      _mutex;
    };

    template <typename T>
    EnvironmentVariable<T*> Interface<T>::_instance;

    template <typename T>
    void Interface<T>::Register(const char*name, T* type) {
        if (!type) {
            V_Assert(false, "Interface '%s' registering a null pointer!", name);
            return;
        }

        if (T* foundType = Get(name)) {
            V_Assert(false, "Interface '%s' already registered! [Found: %p]", name, foundType);
            return;
        }

        VStd::unique_lock<VStd::shared_mutex> lock(_mutex);
        _instance = Environment::CreateVariable<T*>(GetVariableName(name));
        _instance.Get(name) = type;
    }

    template <typename T>
    void Interface<T>::Unregister(const char* name) {
        if (!_instance || !_instance.Get(name)) {
            V_Assert(false, "Interface '%s' not registered on this module!", name);
            return;
        }

        if (_instance.Get(name) != type) {
            V_Assert(false, "Interface '%s' is not the same instance that was registered! [Expected '%p', Found '%p']", name, type, _instance.Get());
            return;
        }

        // Assign the internal pointer to null and release the EnvironmentVariable reference.
        VStd::unique_lock<VStd::shared_mutex> lock(_mutex);
        *_instance = nullptr;
        _instance.Reset();
    }

    template <typename T>
    T* Interface<T>::Get(const char* name) {
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
        _instance = Environment::FindVariable<T*>(GetVariableName(name));
        return _instance ? _instance.Get() : nullptr;
    }

    template <typename T>
    uint32_t Interface<T>::GetVariableName(const char* name) {
        // Environment variable name is taken from the hash of the Uuid (truncated to 32 bits).
        return static_cast<uint32_t>(TypeHash32(name));
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