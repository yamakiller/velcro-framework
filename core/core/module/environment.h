#ifndef V_FRAMEWORK_CORE_MODULE_ENVIRONMENT_H
#define V_FRAMEWORK_CORE_MODULE_ENVIRONMENT_H

#include <core/std/smart_ptr/sp_convertible.h>
#include <core/std/parallel/mutex.h>
#include <core/std/parallel/spin_mutex.h>
#include <core/std/parallel/lock.h>
#include <core/std/typetraits/alignment_of.h>
#include <core/std/typetraits/has_virtual_destructor.h>
#include <core/std/typetraits/aligned_storage.h>

namespace V {
    namespace Internal {
        class EnvironmentInterface;
    }

    typedef Internal::EnvironmentInterface* EnvironmentInstance;

    /// EnvironmentVariable 保存指向实际变量的指针,它应该
    /// 用作任何智能指针. 事件虽然大部分都是"静态的",因为它被视为全局事件.
    template<class T>
    class EnvironmentVariable;

    /**
     * Environment is a module "global variable" provider. It's used when you want to share globals (which should happen as little as possible)
     * between modules/DLLs.
     *
     * The environment is automatically created on demand. This is why if you want to share the environment between modules you should call \ref
     * AttachEnvironment before anything else that will use the environment. At a high level the environment is basically a hash table with variables<->guid.
     * The environment doesn't own variables they are managed though reference counting.
     *
     * All environment "internal" allocations are directly allocated from OS C heap, so we don't use any allocators as they use the environment themselves.
     *
     * You can create/destroy variables from multiple threads, HOWEVER accessing the data of the environment is NOT. If you want to access your variable
     * across threads you will need to add extra synchronization, which is the same if your global was "static".
     *
     * Using the environment with POD types is straight and simple. On the other hand when your environment variable has virtual methods, things become more
     * complex. We advice that you avoid this as much as possible. When a variable has virtual function, even if we shared environment and memory allocations,
     * the variable will still be "bound" to the module as it's vtable will point inside the module. Which mean that if we unload the module we have to destroy
     * your variable. The system handles this automatically if your class has virtual functions. You can still keep your variables, but if you try to access the data
     * you will trigger an assert, unless you recreated the variable from one of the exiting modules.
     *
     * \note we use class for the Environment (instead of simple global function) so can easily friend it to create variables where the ctor/dtor are protected
     * that way you need to add friend V::Environment and that's it.
     */
    namespace Environment {
        /**
         * Allocator hook interface. 这不应该使用任何 Velcro 分配器或接,因为它将被调用来创建它们.
         */
        class AllocatorInterface {
        public:

            virtual void* Allocate(size_t byteSize, size_t alignment) = 0;

            virtual void DeAllocate(void* address) = 0;
        };

        /**
         * 创建一个环境变量，如果该变量已经存在则返回现有变量.
         * 如果没有，它将创建一个并返回新变量.
         * \param guid - 变量的唯一持久 ID. 它将在模块/不同和编译之间进行匹配.
         * 返回的变量将被释放, 并且当对它的最后一个引用消失时, 它的内存将被释放.
         */
        template <typename T, class... Args>
        EnvironmentVariable<T>  CreateVariable(u32 guid, Args&&... args);

        /**
         * Same as 参考 CreateVariable 除了使用唯一的字符串来标识变量之外, 该字符串将被转换为 Crc32 并使用 guid.
         */
        template <typename T, class... Args>
        EnvironmentVariable<T>  CreateVariable(const char* uniqueName, Args&&... args);

        
        /**
         * Advanced 参考 CreateVariable,您有额外的参数来控制变量的创建和生命周期。
         * \param guid - same as 参考 CreateVariable guid
         * \param isConstruct - 如果我们真的应该“创建”变量. 如果为 false, 我们将只注册变量持有者, 但 
         * EnvironmentVariable<T>::IsConstructed 将返回 false. 您稍后需要使用 EnvironmentVariable<T>::Construct() 手动创建它.
         * \param isTransferOwnership - 如果我们可以跨模块转移变量的所有权.
         * 例如, Module_1 创建了该变量, 而 Module_2 正在使用它. 如果 isTransferOwnership 为 true, 则卸载 
         * Module_1, Module_2 将获得所有权. 如果 isTransferOwnership 为 fals, 它将销毁该变量, 并且 Module_2 
         * 的任何使用尝试都将触发并出错(直到其他人再次创建它).
         */
        template <typename T, class... Args>
        EnvironmentVariable<T>  CreateVariableEx(u32 guid, bool isConstruct, bool isTransferOwnership, Args&&... args);

        /// Same as 参考 CreateVariableEx/CreateVariable.
        template <typename T, class... Args>
        EnvironmentVariable<T>  CreateVariableEx(const char* uniqueName, bool isConstruct, bool isTransferOwnership, Args&&... args);

        /**
         * 检查变量是否存在,如果存在则返回有效变量,否则结果变量指针将为空.
         */
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

        /**
         * 使用客户分配器接口创建环境. 您不必调用 create 或 destroy, 因为它们将根据需要创建, 但在
         * 这种情况下将使用模块分配器. 例如, 在 Windows 上, 如果链接 CRT, 两个环境最终将位于不同的堆上.
         * \returns 如果创建成功则为 true, 如果环境已创建/附加则为 false.
         */
        bool Create(AllocatorInterface* allocator);

        /**
         * Explicit Destroy, 除非你想控制顺序, 否则你不必调用它.当模块被卸载时它将被调用. 当然, 不保证订单.
         */
        void Destroy();

        /**
         * 从 sourceEnvironment 附加当前模块环境.
         * note: 这不是副本, 它实际上会引用源环境, 因此您添加删除的任何变量将对所有共享模块可见
         * \param useAsFallback 如果设置为 true, 将创建一个新环境, 并且仅检查共享环境的 GetVariable 失败.
         * 这样你就可以改变环境.
         */
        void Attach(EnvironmentInstance sourceEnvironment, bool useAsGetFallback = false);

        /// Detaches the active environment (if one is attached)
        void Detach();

        /// Returns true if an environment is attached to this module
        bool IsReady();
    }
}

#endif 