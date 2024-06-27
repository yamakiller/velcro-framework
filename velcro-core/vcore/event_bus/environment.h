/*
 * Copyright (c) Contributors to the VelcroFramework Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#ifndef V_FRAMEWORK_CORE_EVENT_BUS_ENVIRONMENT_H
#define V_FRAMEWORK_CORE_EVENT_BUS_ENVIRONMENT_H

#include <vcore/module/environment.h>

#include <vcore/math/crc.h>
#include <vcore/memory/osallocator.h> // Needed for Context allocation from the OS
#include <vcore/std/containers/vector.h> // Environment context array

namespace V
{
    template<class Context>
    struct EventBusEnvironmentStoragePolicy;

    class EventBusEnvironment;

    namespace Internal
    {
        typedef EventBusEnvironment* (*EventBusEnvironmentGetterType)();
        typedef void(*EventBusEnvironmentSetterType)(EventBusEnvironment*);

        /**
        * Base for EventBus<T>::Context. We use it to support multiple EventBusEnvironments (have collection of contexts and manage state).
        */
        class ContextBase
        {
            template<class Context>
            friend struct V::EventBusEnvironmentStoragePolicy;
            friend class V::EventBusEnvironment;

        public:
            ContextBase();
            ContextBase(EventBusEnvironment*);

            virtual ~ContextBase() {}

        private:

            int m_ebusEnvTLSIndex;
            EventBusEnvironmentGetterType m_ebusEnvGetter;
        };

        /**
        * Objects that will shared in the environment to access all thread local storage
        * current environment. We use that objects so we can register in the Environment 
        * and share the data into that first module. Keep in mind that if the module that 
        * created that class is unload the code will crash as we store the function pointer
        * to make we read the TSL from only one module, otherwise each module has it's own TSL blocks.
        * If this happens make sure you create this structure in the environment from the main executable
        * or a module that will be loaded before and unloaded before any EventBuses are used.
        */
        struct EventBusEnvironmentTLSAccessors
        {
            EventBusEnvironmentTLSAccessors();

            static u32 GetId();

            EventBusEnvironmentGetterType m_getter;
            EventBusEnvironmentSetterType m_setter;

            static EventBusEnvironment* GetTLSEnvironment();
            static void SetTLSEnvironment(EventBusEnvironment* environment);

            VStd::atomic_int NumUniqueEventBuses; ///< Used to provide unique index for the TLS table

            static V_THREAD_LOCAL EventBusEnvironment* _tlsCurrentEnvironment; ///< Pointer to the current environment for the current thread.
        };

        class EventBusEnvironmentAllocator
        {
            /**
            * VStd allocator wrapper for the EventBus context internals. Don't expose to external code, this allocation are NOT
            * tracked, etc. These are only for EventBus internal usage.
            */
        public:
            typedef void*               pointer_type;
            typedef VStd::size_t       size_type;
            typedef VStd::ptrdiff_t    difference_type;
            typedef VStd::false_type   allow_memory_leaks;

            EventBusEnvironmentAllocator();
            EventBusEnvironmentAllocator(const EventBusEnvironmentAllocator& rhs);
            pointer_type allocate(size_t byteSize, size_t alignment, int flags = 0);
            size_type resize(pointer_type, size_type) { return 0; }
            void deallocate(pointer_type ptr, size_type byteSize, size_type alignment);

            bool operator==(const EventBusEnvironmentAllocator&) { return true; }
            bool operator!=(const EventBusEnvironmentAllocator&) { return false; }
            EventBusEnvironmentAllocator& operator=(const EventBusEnvironmentAllocator&) { return *this; }

            const char* get_name() const { return m_name; }
            void        set_name(const char* name) { m_name = name; }
            constexpr size_type   max_size() const { return V_CORE_MAX_ALLOCATOR_SIZE; }
            size_type   get_allocated_size() const { return 0; }

            bool is_lock_free() { return false; }
            bool is_stale_read_allowed() { return false; }
            bool is_delayed_recycling() { return false; }

        private:
            const char* m_name;
            Environment::AllocatorInterface* m_allocator;
        };
    }

    /**
     * EventBusEnvironment defines a separate EventBus execution context. All EventBuses will have unique instances in each environment, unless specially configured to not do that (not supported yet as it's tricky and will likely create contention and edge cases).
     * If you want EventBusEnvinronments to communicate you should use a combination of listeners/routers and event queuing to implement that. This is by design as the whole purpose of having a separate environment is to cut any possible
     * sharing by default, think of it as a separate VM. When communication is needed it should be explicit and considering the requirements you had in first place when you created separate environment. Otherwise you 
     * will start having contention issues or even worse execute events (handlers) when the environment is not active.
     * EventBusEnvironment is very similar to the way OpenGL contexts operate. You can manage their livecycle from any thread at anytime by calling EventBusEnvironment::Create/Destroy. You can activate/deactivate an environment by calling
     * ActivateOnCurrentThread/DeactivateOnCurrentThread. Every EventBusEnvironment can be activated to only one thread at a time.
     */
    class EventBusEnvironment
    {
        template<class Context>
        friend struct EventBusEnvironmentStoragePolicy;

    public:
        V_CLASS_ALLOCATOR(EventBusEnvironment,V::OSAllocator, 0);

        EventBusEnvironment();

        ~EventBusEnvironment();

        void ActivateOnCurrentThread();

        void DeactivateOnCurrentThread();

        /**
         * Create and Destroy are provided for consistency and writing code with explicitly 
         * set Create/Destroy location. You can also just use it as any other class:
         * EventBusEnvironment* myEventBusEnvironmnet = aznew EventBusEnvironment;
         * and later delete myEventBusEnvironment.
         */
        static EventBusEnvironment* Create();
        
        static void Destroy(EventBusEnvironment* environment);
        
        
        template<class Context>
        Context* GetBusContext(int tlsKey);

        /**
         * Finds an EventBus \ref Internal::ContextBase in the current environment.
         * @returns Pointer to the context base class, or null if context for this doesn't exists.
         */ 
       V::Internal::ContextBase* FindContext(int tlsKey);
        
        /**
         * Inserts an existing context for a specific bus id. When using this function make sure the provided context is alive while this environment is operational.
         * @param busId is of the bus
         * @param context context to insert
         * @param isTakeOwnership true if want this environment to delete the context when destroyed, or false is this context is owned somewhere else
         * @return true if the insert was successful, false if there is already a context for this id and the insert did not happen.
         */
        bool InsertContext(int tlsKey,V::Internal::ContextBase* context, bool isTakeOwnership);

        /**
        * This is a helper function that will insert that global bus context Bus::GetContext and use it for this environment.
        * Currently this function requires that the Bus uses \ref EventBusEnvironmentStoragePolicy, otherwise you will get a compile error.
        * Once we have a way to have a BusId independent of the storage policy, we can remove that restriction.
        * @return true if the insert/redirect was successful, false if there is already a context for this bus and the insert did not happen.
        */
        template<class Bus>
        bool RedirectToGlobalContext();

    protected:

        EnvironmentVariable<Internal::EventBusEnvironmentTLSAccessors> m_tlsAccessor;

        EventBusEnvironment*  m_stackPrevEnvironment;   ///< Pointer to the previous environment on the TLS stack. This is valid only when the context is in use. Each BusEnvironment can only be active on a single thread at a time.

        VStd::vector<VStd::pair<Internal::ContextBase*,bool>, OSStdAllocator> m_busContexts; ///< Array with all EventBus<T>::Context for this environment
    };

    //////////////////////////////////////////////////////////////////////////
    template<class Context>
    inline Context* EventBusEnvironment::GetBusContext(int tlsKey)
    {
       V::Internal::ContextBase* context = FindContext(tlsKey);
        if (!context)
        {
            context = new(V_OS_MALLOC(sizeof(Context), VStd::alignment_of<Context>::value)) Context(this);
            InsertContext(tlsKey, context, true);
        }
        return static_cast<Context*>(context);
    }

    //////////////////////////////////////////////////////////////////////////
    template<class Bus>
    bool EventBusEnvironment::RedirectToGlobalContext()
    {
        EventBusEnvironment* currentEnvironment = m_tlsAccessor->m_getter(); // store current environment
        m_tlsAccessor->m_setter(nullptr); // set the environment to null to make sure that we can global context when we call Get()
        typename Bus::Context& globalContext = Bus::StoragePolicy::GetOrCreate();
        bool isInserted = InsertContext(globalContext.m_ebusEnvTLSIndex, &globalContext, false);
        m_tlsAccessor->m_setter(currentEnvironment);
        return isInserted;
    }

    /**
    * A choice ofV::EventBusTraits::StoragePolicy that specifies
    * that EventBus data is stored in theV::Environment and it also support multiple \ref EventBusEnvironment
    * With this policy, a single EventBus instance is shared across all
    * modules (DLLs) that attach to theV::Environment.
    * @tparam Context A class that contains EventBus data.
    *
    * \note Using separate EventBusEnvironment allows you to manage fully independent EventBus communication environments,
    * most frequently this is use to remove/reduce contention when we choose to process in parallel unrelated 
    * systems.
    */
    template<class Context>
    struct EventBusEnvironmentStoragePolicy
    {
        /**
        * Returns the EventBus data if it's already created, else nullptr.
        * @return A pointer to the EventBus data.
        */
        static Context* Get();

        /**
        * Returns the EventBus data. Creates it if not already created.
        * @return A reference to the EventBus data.
        */
        static Context& GetOrCreate();

        /**
        * Environment variable that contains a pointer to the EventBus data.
        */
        static EnvironmentVariable<Context> _defaultGlobalContext;

        // EventBus traits should provide a valid unique name, so that handlers can 
        // connect to the EventBus across modules.
        // This can fail on some compilers. If it fails, make sure that you give 
        // each bus a unique name.
        static u32 GetVariableId();
    };

    template<class Context>
    EnvironmentVariable<Context> EventBusEnvironmentStoragePolicy<Context>::_defaultGlobalContext;

    //////////////////////////////////////////////////////////////////////////
    template<class Context>
    Context* EventBusEnvironmentStoragePolicy<Context>::Get()
    {
        if (!_defaultGlobalContext && Environment::IsReady())
        {
            _defaultGlobalContext = Environment::FindVariable<Context>(GetVariableId());
        }

        if (_defaultGlobalContext)
        {
            if (_defaultGlobalContext.IsConstructed())
            {
                Context* globalContext = &_defaultGlobalContext.Get();

                if (EventBusEnvironment* tlsEnvironment = globalContext->m_ebusEnvGetter())
                {
                    return tlsEnvironment->GetBusContext<Context>(globalContext->m_ebusEnvTLSIndex);
                }
                return globalContext;
            }
        }

        return nullptr;
    }

    //////////////////////////////////////////////////////////////////////////
    template<class Context>
    Context& EventBusEnvironmentStoragePolicy<Context>::GetOrCreate() {
        if (!_defaultGlobalContext) {
            _defaultGlobalContext = Environment::CreateVariable<Context>(GetVariableId());
        }

        Context& globalContext = *_defaultGlobalContext;

        if (EventBusEnvironment* tlsEnvironment = globalContext.m_ebusEnvGetter()) {
            return *tlsEnvironment->GetBusContext<Context>(globalContext.m_ebusEnvTLSIndex);
        }

        return globalContext;
    }

    //////////////////////////////////////////////////////////////////////////
    template<class Context>
    u32 EventBusEnvironmentStoragePolicy<Context>::GetVariableId()
    {
        static const u32 NameCrc = Crc32(V_FUNCTION_SIGNATURE);
        return NameCrc;
    }
} // namespace V

#endif // V_FRAMEWORK_CORE_EVENT_BUS_ENVIRONMENT_H