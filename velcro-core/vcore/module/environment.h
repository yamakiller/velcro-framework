#ifndef V_FRAMEWORK_CORE_MODULE_ENVIRONMENT_H___
#define V_FRAMEWORK_CORE_MODULE_ENVIRONMENT_H___

#include <vcore/std/smart_ptr/sp_convertible.h>
#include <vcore/std/parallel/mutex.h>
#include <vcore/std/parallel/shared_mutex.h>
#include <vcore/std/parallel/spin_mutex.h>
#include <vcore/std/parallel/lock.h>
#include <vcore/std/typetraits/alignment_of.h>
#include <vcore/std/typetraits/has_virtual_destructor.h>
#include <vcore/std/typetraits/aligned_storage.h>


namespace V {
    namespace Internal {
        class EnvironmentInterface;
    }

    typedef Internal::EnvironmentInterface* EnvironmentInstance;

    /// EnvironmentVariable 保存指向实际变量的指针,它应该
    /// 用作任何智能指针. 事件虽然大部分都是"静态的",因为它被视为全局事件.
    template<class T>
    class EnvironmentVariable;

    namespace Environment {

        class AllocatorInterface {
        public:

            virtual void* Allocate(size_t byteSize, size_t alignment) = 0;

            virtual void DeAllocate(void* address) = 0;
        };


        template <typename T, class... Args>
        EnvironmentVariable<T>  CreateVariable(u32 guid, Args&&... args);


        template <typename T, class... Args>
        EnvironmentVariable<T>  CreateVariable(const char* uniqueName, Args&&... args);

        

        template <typename T, class... Args>
        EnvironmentVariable<T>  CreateVariableEx(u32 guid, bool isConstruct, bool isTransferOwnership, Args&&... args);

        /// Same as 参考 CreateVariableEx/CreateVariable.
        template <typename T, class... Args>
        EnvironmentVariable<T>  CreateVariableEx(const char* uniqueName, bool isConstruct, bool isTransferOwnership, Args&&... args);

    
        template <typename T>
        EnvironmentVariable<T>  FindVariable(u32 guid);


        /// Same as 参考 FindVariable 但使用唯一的字符串作为输入.
        /// 请检查 参考 CreateVariable 以获取有关内部使用的热字符串的更多信息.
        template <typename T>
        EnvironmentVariable<T>  FindVariable(const char* uniqueName);

        /// 返回环境，以便您可以与其共享 参考 AttachEnvironment
        EnvironmentInstance GetInstance();

        /// Returns module id (non persistent)
        void* GetModuleId();

   
        bool Create(AllocatorInterface* allocator);

 
        void Destroy();


        void Attach(EnvironmentInstance sourceEnvironment, bool useAsGetFallback = false);

        /// Detaches the active environment (if one is attached)
        void Detach();

        /// Returns true if an environment is attached to this module
        bool IsReady();
    }

    namespace Internal
    {
        struct EnvironmentVariableResult
        {
            enum States
            {
                Added,
                Removed,
                Found,
                NotFound,
                OutOfMemory,
            };

            States m_state;
            void*  m_variable;
        };

        class EnvironmentInterface
        {
        public:
            virtual ~EnvironmentInterface() {}

            virtual VStd::recursive_mutex& GetLock() = 0;

            virtual void AttachFallback(EnvironmentInstance sourceEnvironment) = 0;

            virtual void DetachFallback() = 0;

            virtual EnvironmentInterface* GetFallback() = 0;

            virtual void* FindVariable(u32 guid) = 0;

            virtual EnvironmentVariableResult AddAndAllocateVariable(u32 guid, size_t byteSize, size_t alignment, VStd::recursive_mutex** addedVariableLock) = 0;

            virtual EnvironmentVariableResult RemoveVariable(u32 guid) = 0;

            virtual EnvironmentVariableResult GetVariable(u32 guid) = 0;

            virtual void AddRef() = 0;

            virtual void ReleaseRef() = 0;

            virtual Environment::AllocatorInterface* GetAllocator() = 0;

            virtual void DeleteThis() = 0;

            static EnvironmentInterface*  _environment;
        };

        // Don't use any virtual methods to we make sure we can reuse variables across modules
        class EnvironmentVariableHolderBase
        {
            friend class EnvironmentImpl;
        protected:
            enum class DestroyTarget {
                Member,
                Self
            };
        public:
            EnvironmentVariableHolderBase(u32 guid, V::Internal::EnvironmentInterface* environmentOwner, bool canOwnershipTransfer, Environment::AllocatorInterface* allocator)
                : m_environmentOwner(environmentOwner)
                , m_moduleOwner(nullptr)
                , m_canTransferOwnership(canOwnershipTransfer)
                , m_isConstructed(false)
                , m_guid(guid)
                , m_allocator(allocator)
                , m_useCount(0)
            {
            }

            bool IsConstructed() const
            {
                return m_isConstructed;
            }

            bool IsOwner() const
            {
                return m_moduleOwner == Environment::GetModuleId();
            }

            u32 GetId() const
            {
                return m_guid;
            }
        protected:
            using DestructFunc = void (*)(EnvironmentVariableHolderBase *, DestroyTarget);
            // Assumes the m_mutex is already locked.
            // On return m_mutex is in an unlocked state.
            void UnregisterAndDestroy(DestructFunc destruct, bool moduleRelease);

            V::Internal::EnvironmentInterface* m_environmentOwner; ///< Used to know which environment we should use to free the variable if we can't transfer ownership
            void* m_moduleOwner; ///< Used when the variable can't transfered across module and we need to destruct the variable when the module is going away
            bool m_canTransferOwnership; ///< True if variable can be allocated in one module and freed in other. Usually true for POD types when they share allocator.
            bool m_isConstructed; ///< When we can't transfer the ownership, and the owning module is destroyed we have to "destroy" the variable.
            u32 m_guid;
            Environment::AllocatorInterface* m_allocator;
            int m_useCount;
            VStd::shared_mutex m_mutex;;
            //VStd::spin_mutex m_mutex;
        };

        template<typename T>
        class EnvironmentVariableHolder
            : public EnvironmentVariableHolderBase
        {
            void ConstructImpl(const VStd::true_type& )
            {
                memset(&m_value, 0, sizeof(T));
            }

            template<class... Args>
            void ConstructImpl(const VStd::false_type& , Args&&... args)
            {
                // Construction of non-trivial types is left up to the type's constructor.
                new(&m_value) T(VStd::forward<Args>(args)...);
            }
            static void DestructDispatchNoLock(EnvironmentVariableHolderBase *base, DestroyTarget selfDestruct)
            {
                auto *self = reinterpret_cast<EnvironmentVariableHolder *>(base);
                if (selfDestruct == DestroyTarget::Self)
                {
                    self->~EnvironmentVariableHolder();
                    return;
                }

                V_Assert(self->m_isConstructed, "Variable is not constructed. Please check your logic and guard if needed!");
                self->m_isConstructed = false;
                self->m_moduleOwner = nullptr;
                if constexpr(!VStd::is_trivially_destructible_v<T>) {
                    //reinterpret_cast<T*>(&self->m_value)->~T();
                }
            }
        public:
            EnvironmentVariableHolder(u32 guid, bool isOwnershipTransfer, Environment::AllocatorInterface* allocator)
                : EnvironmentVariableHolderBase(guid, Environment::GetInstance(), isOwnershipTransfer, allocator)
            {
            }

            ~EnvironmentVariableHolder()
            {
                V_Assert(!m_isConstructed, "To get the destructor we should have already destructed the variable!");
            }
            void AddRef()
            {
                VStd::lock_guard<VStd::shared_mutex> lock(m_mutex);
                ++m_useCount;
                ++_moduleUseCount;
            }

            void Release()
            {
                //m_mutex.lock();
                const bool moduleRelease = (--_moduleUseCount == 0);
                UnregisterAndDestroy(DestructDispatchNoLock, moduleRelease);
            }

            void Construct()
            {
                //VStd::lock_guard<VStd::spin_mutex> lock(m_mutex);
                if (!m_isConstructed)
                {
                    ConstructImpl(VStd::is_trivially_constructible<T>{});
                    m_isConstructed = true;
                    m_moduleOwner = Environment::GetModuleId();
                }
            }

            template <class... Args>
            void Construct(Args&&... args)
            {
                //VStd::lock_guard<VStd::spin_mutex> lock(m_mutex);
                if (!m_isConstructed)
                {
                    ConstructImpl(typename VStd::false_type(), VStd::forward<Args>(args)...);
                    m_isConstructed = true;
                    m_moduleOwner = Environment::GetModuleId();
                }
            }

            void Destruct()
            {
                //VStd::lock_guard<VStd::spin_mutex> lock(m_mutex);
                DestructDispatchNoLock(this, DestroyTarget::Member);
            }

            // variable storage
            typename VStd::aligned_storage<sizeof(T), VStd::alignment_of<T>::value>::type m_value;
            static int _moduleUseCount;
        };

        template<typename T>
        int EnvironmentVariableHolder<T>::_moduleUseCount = 0;


        EnvironmentVariableResult AddAndAllocateVariable(u32 guid, size_t byteSize, size_t alignment, VStd::recursive_mutex** addedVariableLock = nullptr);

        /// Returns the value of the variable if found, otherwise nullptr.
        EnvironmentVariableResult GetVariable(u32 guid);

        /// Returns the allocator used by the current environment.
        Environment::AllocatorInterface* GetAllocator();

        /// Converts a string name to an ID (using Crc32 function)
        u32  EnvironmentVariableNameToId(const char* uniqueName);
    } // namespace Internal


    template<class T>
    class EnvironmentVariable {
    public:
        typedef typename V::Internal::EnvironmentVariableHolder<T> HolderType;

        EnvironmentVariable()
            : m_data(nullptr) {
        }

        EnvironmentVariable(HolderType* holder)
            : m_data(holder) {
            if (m_data != nullptr) {
                m_data->AddRef();
            }
        }

        template<class U>
        EnvironmentVariable(EnvironmentVariable<U> const& rhs, typename VStd::Internal::sp_enable_if_convertible<U, T>::type = VStd::Internal::sp_empty())
            : m_data(rhs.m_data) {
            if (m_data != nullptr) {
                m_data->AddRef();
            }
        }
        EnvironmentVariable(EnvironmentVariable const& rhs)
            : m_data(rhs.m_data) {
            if (m_data != nullptr) {
                m_data->AddRef();
            }
        }
        ~EnvironmentVariable() {
            if (m_data != nullptr) {
                m_data->Release();
            }
        }
        template<class U>
        EnvironmentVariable& operator=(EnvironmentVariable<U> const& rhs) {
            EnvironmentVariable(rhs).Swap(*this);
            return *this;
        }

        EnvironmentVariable(EnvironmentVariable&& rhs)
            : m_data(rhs.m_data) {
            rhs.m_data = nullptr;
        }
        EnvironmentVariable& operator=(EnvironmentVariable&& rhs) {
            EnvironmentVariable(static_cast< EnvironmentVariable && >(rhs)).Swap(*this);
            return *this;
        }

        EnvironmentVariable& operator=(EnvironmentVariable const& rhs) {
            EnvironmentVariable(rhs).Swap(*this);
            return *this;
        }

        void Reset() {
            EnvironmentVariable().Swap(*this);
        }

        T& operator*() const {
            V_Assert(IsValid(), "You can't dereference a null pointer");
            V_Assert(m_data->IsConstructed(), "You are using an invalid variable, the owner has removed it!");
            return *reinterpret_cast<T*>(&m_data->m_value);
        }

        T* operator->() const {
            V_Assert(IsValid(), "You can't dereference a null pointer");
            V_Assert(m_data->IsConstructed(), "You are using an invalid variable, the owner has removed it!");
            return reinterpret_cast<T*>(&m_data->m_value);
        }

        T& Get() const {
            V_Assert(IsValid(), "You can't dereference a null pointer");
            V_Assert(m_data->IsConstructed(), "You are using an invalid variable, the owner has removed it!");
            return *reinterpret_cast<T*>(&m_data->m_value);
        }

        void Set(const T& value) {
            Get() = value;
        }

        explicit operator bool() const {
            return IsValid();
        }
        bool operator! () const { return !IsValid(); }

        void Swap(EnvironmentVariable& rhs) {
            HolderType* tmp = m_data;
            m_data = rhs.m_data;
            rhs.m_data = tmp;
        }

        bool IsValid() const {
            return m_data;
        }

        bool IsOwner() const {
            return m_data && m_data->IsOwner();
        }

        bool IsConstructed() const {
            return m_data && m_data->IsConstructed();
        }

        void Construct() {
            V_Assert(IsValid(), "You can't dereference a null pointer");
            m_data->Construct();
        }

    protected:
        HolderType* m_data;
    };

    template <class T, class U>
    inline bool operator==(EnvironmentVariable<T> const& a, EnvironmentVariable<U> const& b) {
        return a.m_data == b.m_data;
    }

    template <class T, class U>
    inline bool operator!=(EnvironmentVariable<T> const& a, EnvironmentVariable<U> const& b) {
        return a.m_data != b.m_data;
    }

    template <class T, class U>
    inline bool operator==(EnvironmentVariable<T> const& a, U* b) {
        return a.m_data == b;
    }

    template <class T, class U>
    inline bool operator!=(EnvironmentVariable<T> const& a, U* b) {
        return a.m_data != b;
    }

    template <class T, class U>
    inline bool operator==(T* a, EnvironmentVariable<U> const& b) {
        return a == b.m_data;
    }

    template <class T, class U>
    inline bool operator!=(T* a, EnvironmentVariable<U> const& b) {
        return a != b.m_data;
    }

    template <class T>
    bool operator==(EnvironmentVariable<T> const& a, std::nullptr_t) {
        return !a.IsValid();
    }

    template <class T>
    bool operator!=(EnvironmentVariable<T> const& a, std::nullptr_t) {
        return a.IsValid();
    }

    template <class T>
    bool operator==(std::nullptr_t, EnvironmentVariable<T> const& a) {
        return !a.IsValid();
    }

    template <class T>
    bool operator!=(std::nullptr_t, EnvironmentVariable<T> const& a) {
        return a.IsValid();
    }

    template <class T>
    inline bool operator<(EnvironmentVariable<T> const& a, EnvironmentVariable<T> const& b) {
        return a.m_data < b.m_data;
    }

    template <typename T, class... Args>
    EnvironmentVariable<T>  Environment::CreateVariable(u32 guid, Args&&... args) {
        // If T has virtual functions, we can't transfer ownership even if we share
        // allocators, because the vtable will point to memory inside the module that created the variable.
        //bool isTransferOwnership = !VStd::has_virtual_destructor<T>::value;
        return CreateVariableEx<T>(guid, true, true, VStd::forward<Args>(args)...);
    }

    template <typename T, class... Args>
    EnvironmentVariable<T> Environment::CreateVariable(const char* uniqueName, Args&&... args) {
        return CreateVariable<T>(V::Internal::EnvironmentVariableNameToId(uniqueName), VStd::forward<Args>(args)...);
    }

    template <typename T, class... Args>
    EnvironmentVariable<T>  Environment::CreateVariableEx(u32 guid, bool isConstruct, bool isTransferOwnership, Args&&... args)
    {
        typedef typename EnvironmentVariable<T>::HolderType HolderType;
        HolderType* variable = nullptr;
        VStd::recursive_mutex* addLock = nullptr;
        V::Internal::EnvironmentVariableResult result = V::Internal::AddAndAllocateVariable(guid, sizeof(HolderType), VStd::alignment_of<HolderType>::value, &addLock);
        if (result.m_state == V::Internal::EnvironmentVariableResult::Added) {
            variable = new(result.m_variable)HolderType(guid, isTransferOwnership, V::Internal::GetAllocator());
        } else if (result.m_state == V::Internal::EnvironmentVariableResult::Found) {
            variable = reinterpret_cast<HolderType*>(result.m_variable);
        }

        if (isConstruct && variable) {
            variable->Construct(VStd::forward<Args>(args)...);
        }

        if (addLock) {
            addLock->unlock();
        }

        return variable;
    }

    /// Same as \ref CreateVariableEx and \ref CreateVariable with advanced control parameters.
    template <typename T, class... Args>
    EnvironmentVariable<T>   Environment::CreateVariableEx(const char* uniqueName, bool isConstruct, bool isTransferOwnership, Args&&... args)
    {
        return CreateVariableEx<T>(Internal::EnvironmentVariableNameToId(uniqueName), isConstruct, isTransferOwnership, VStd::forward<Args>(args)...);
    }

    template <typename T>
    EnvironmentVariable<T> Environment::FindVariable(u32 guid)
    {
        typedef typename EnvironmentVariable<T>::HolderType HolderType;
        V::Internal::EnvironmentVariableResult result = V::Internal::GetVariable(guid);
        if (result.m_state == V::Internal::EnvironmentVariableResult::Found)
        {
            return reinterpret_cast<HolderType*>(result.m_variable);
        }
        return nullptr;
    }

    template <typename T>
    EnvironmentVariable<T>  Environment::FindVariable(const char* uniqueName)
    {
        return FindVariable<T>(V::Internal::EnvironmentVariableNameToId(uniqueName));
    }

    namespace Internal
    {
        inline void AttachGlobalEnvironment(void* globalEnv)
        {
            V_Assert(!V::Environment::IsReady(), "An environment is already created in this module!");
            V::Environment::Attach(static_cast<V::EnvironmentInstance>(globalEnv));
        }
    }
} // namespace V

 

#ifdef V_MONOLITHIC_BUILD
#define V_DECLARE_MODULE_INITIALIZATION
#else

/// Default implementations of functions invoked upon loading a dynamic module.
/// For dynamic modules which define a \ref V::Module class, use \ref AZ_DECLARE_MODULE_CLASS instead.
///
/// For more details see:
/// \ref V::InitializeDynamicModuleFunction, \ref V::UninitializeDynamicModuleFunction
#define V_DECLARE_MODULE_INITIALIZATION \
    extern "C" V_DLL_EXPORT void InitializeDynamicModule(void* env) \
    { \
        V::Internal::AttachGlobalEnvironment(env); \
    } \
    extern "C" V_DLL_EXPORT void UninitializeDynamicModule() { V::Environment::Detach(); }

#endif // V_MONOLITHIC_BUILD

#endif //V_FRAMEWORK_CORE_MODULE_ENVIRONMENT_H___