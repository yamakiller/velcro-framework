#ifndef V_FRAMEWORK_CORE_SERIALIZATION_CONTEXT_H
#define V_FRAMEWORK_CORE_SERIALIZATION_CONTEXT_H

#include <limits>

#include <vcore/memory/osallocator.h>
#include <vcore/memory/system_allocator.h>

#include <vcore/std/containers/unordered_set.h>
#include <vcore/std/containers/unordered_map.h>
#include <vcore/std/string/conversions.h>
#include <vcore/std/containers/set.h>
#include <vcore/std/containers/map.h>
#include <vcore/std/string/string.h>
#include <vcore/std/string/string_view.h>

#include <vcore/std/typetraits/disjunction.h>
#include <vcore/std/typetraits/is_pointer.h>
#include <vcore/std/typetraits/is_abstract.h>
#include <vcore/std/typetraits/negation.h>
#include <vcore/std/typetraits/remove_pointer.h>
#include <vcore/std/typetraits/is_base_of.h>
#include <vcore/std/any.h>
#include <vcore/std/parallel/atomic.h>

#include <vcore/std/functional.h>

#include <vcore/vobject/reflect_context.h>

#include <vcore/math/crc.h>
#include <vcore/io/byte_container_stream.h>

#define VELCRO_SERIALIZE_BINARY_STACK_BUFFER 4096

#define V_SERIALIZE_SWAP_ENDIAN(_value, _isSwap)  if (_isSwap) VStd::endian_swap(_value)

namespace V {
    class EditContext;
    class ObjectStream;
    class GenericClassInfo;
    struct DataPatchNodeInfo;

    namespace ObjectStreamInternal
    {
        class ObjectStreamImpl;
    }


    namespace Edit {
        struct ElementData;
        struct ClassData;
    }

    namespace Serialize {
        template<class T>
        struct StaticInstance
        {
            static T _instance;
        };
        template<class T>
        T StaticInstance<T>::_instance;

        namespace Attributes {
            extern const Crc32 EnumValueKey;
            extern const Crc32 EnumUnderlyingType;
        } // namespace Attributes
    }

    using AttributePtr = VStd::shared_ptr<Attribute>;
    using AttributeSharedPair = VStd::pair<AttributeId, AttributePtr>;
    template <typename ContainerType, typename T>
    AttributePtr CreateModuleAttribute(T&& attrValue);


    class SerializeContext : public ReflectContext {
        static const unsigned int VersionClassDeprecated = (unsigned int)-1;
    public:
        class  ClassBuilder;
        class  EnumBuilder;

        class  ClassData;
        struct EnumerateInstanceCallContext;
        struct ClassElement;
        struct DataElement;
        class  DataElementNode;
        class  IObjectFactory;

        using  IRttiHelper = V::IRttiHelper;
        class  IDataSerializer;
        class  IDataContainer;
        class  IEventHandler;
        class  IDataConverter;

        using  IDataSerializerDeleter = VStd::function<void(IDataSerializer* ptr)>;
        using  IDataSerializerPtr = VStd::unique_ptr<IDataSerializer, IDataSerializerDeleter>;
    public:
        VOBJECT_RTTI(SerializeContext, "{6fa6712c-ffb7-4f4d-a375-d81882319dd0}", ReflectContext);
        V_CLASS_ALLOCATOR(SerializeContext, SystemAllocator, 0);
        
        /// @brief Callback to process data conversion.
        typedef bool(* VersionConverter)(SerializeContext& /*context*/, DataElementNode& /* elements to convert */);
        /// @brief Callback for persistent ID for an instance 
        /// @note: We should probably switch this to UUID
        typedef u64(* ClassPersistentId)(const void* /*class instance*/);
        /// @brief Callback to manipulate entity saving on a yes/no base, otherwise 
        /// you will need provide serializer for more advanced logic.
        typedef bool(* ClassDoSave)(const void* /*class instance*/);
        /// @brief  bind allocator to serialize allocator
        typedef VStd::unordered_map<Uuid, ClassData> IdToClassMap;

        explicit SerializeContext(bool registerIntegralTypes = true, bool createEditContext = false);
        virtual ~SerializeContext();

        /// Deleting copy ctor because we own unique_ptr's of IDataContainers
        SerializeContext(const SerializeContext&) = delete;
        SerializeContext& operator =(const SerializeContext&) = delete;

        bool IsTypeReflected(V::Uuid typeId) const override;

        /// @brief SerializeBind 绑定类和变量以进行序列化的代码.
        template<class T, class... TBaseClasses>
        ClassBuilder    Class();

        /// @brief 当无法使用默认对象工厂时, 例如:具有私有构造函数或
        ///        非默认构造函数的单例类, 您将必须提供自定义工厂.
        template<class T, class... TBaseClasses>
        ClassBuilder    Class(IObjectFactory* factory);

        template<typename EnumType>
        EnumBuilder Enum();
        template<typename EnumType>
        EnumBuilder Enum(IObjectFactory* factory);

        /// 函数指针,用于使用序列化上下文为注册类型构造 VStd::any.
        using CreateAnyFunc = VStd::any(*)(SerializeContext*);
        /// 允许注册 TypeId, 而无需提供 C++ 类型 如果该类型尚未注册, 则 ClassData 将移至 SerializeContext 内部结构中.
        ClassBuilder RegisterType(const V::TypeId& typeId, V::SerializeContext::ClassData&& classData,
            CreateAnyFunc createAnyFunc = [](SerializeContext*) -> VStd::any { return {}; });

        /// @brief 从 SerializeContext 取消注册类型并删除所有内部映射
        /// @return true if the type was previously registered
        bool UnregisterType(const V::TypeId& typeId);
        // 获取 ValueType 的通用信息并调用 Reflect 的辅助方法(如果存在)
        template <class ValueType>
        void RegisterGenericType();

        // 弃用以前反映的类, 以便在加载时任何实例都将被静默删除, 而无需保留原始类.
        // 使用此功能应被视为暂时的, 并应尽快删除.
        void ClassDeprecate(const char* name, const V::Uuid& typeUuid, VersionConverter converter = nullptr);

    public:
        struct DbgStackEntry
        {
            void  ToString(VStd::string& str) const;
            const void*         DataPtr;
            const Uuid*         UuidPtr;
            const ClassData*    ClassDataPtr;
            const char*         ElementName;
            const ClassElement* DlassElementPtr;
        };

        class ErrorHandler
        {
            typedef VStd::vector<DbgStackEntry>    DbgStack;
        public:

            V_CLASS_ALLOCATOR(ErrorHandler, V::SystemAllocator, 0);

            ErrorHandler()
                : m_nErrors(0)
                , m_nWarnings(0)
            {
            }

            /// Report an error within the system
            void            ReportError(const char* message);
            /// Report a non-fatal warning within the system
            void            ReportWarning(const char* message);
            /// Pushes an entry onto debug stack.
            void            Push(const DbgStackEntry& de);
            /// Pops the last entry from debug stack.
            void            Pop();
            /// Get the number of errors reported.
            unsigned int    GetErrorCount() const           { return m_nErrors; }
            /// Get the number of warnings reported.
            unsigned int    GetWarningCount() const         { return m_nWarnings; }
            /// Reset error counter
            void            Reset();

        private:
            VStd::string GetStackDescription() const;
        private:
            DbgStack        m_stack;
            unsigned int    m_nErrors;
            unsigned int    m_nWarnings;
        };

        /// @brief 使用序列化信息枚举实例层次结构的函数.
        enum EnumerationAccessFlags
        {
            ENUM_ACCESS_FOR_READ    = 0,        // 您只需要对枚举数据进行读取访问.
            ENUM_ACCESS_FOR_WRITE   = 1 << 0,   // 您需要对枚举数据进行写访问.
            ENUM_ACCESS_HOLD        = 1 << 1    // 您希望在枚举完成后继续访问数据.当您不再需要访问权限时,您有责任通知数据所有者.
        };

        /// EnumerateInstance() 为实例层次结构中的每个节点调用此回调.返回 true 也可以枚举该节点的子节点,否则返回 false.
        typedef VStd::function< bool (void* /* instance pointer */, const ClassData* /* classData */, const ClassElement* /* classElement*/) > BeginElemEnumCB;
        /// EnumerateInstance() 当元素的子分支的枚举完成时调用此回调.返回 true 以继续枚举该元素的同级元素, 否则返回 false.
        typedef VStd::function< bool () > EndElemEnumCB;

        bool EnumerateInstanceConst(EnumerateInstanceCallContext* callContext, const void* ptr, const Uuid& classId, const ClassData* classData, const ClassElement* classElement) const;
        bool EnumerateInstance(EnumerateInstanceCallContext* callContext, void* ptr, const Uuid& classId, const ClassData* classData, const ClassElement* classElement) const;

        /// Traverses a give object \ref EnumerateInstance assuming that object is a root element (not classData/classElement information).
        template<class T>
        bool EnumerateObject(T* obj, const BeginElemEnumCB& beginElemCB, const EndElemEnumCB& endElemCB, unsigned int accessFlags, ErrorHandler* errorHandler = nullptr) const;

        template<class T>
        bool EnumerateObject(const T* obj, const BeginElemEnumCB& beginElemCB, const EndElemEnumCB& endElemCB, unsigned int accessFlags, ErrorHandler* errorHandler = nullptr) const;

        /// TypeInfo 枚举回调.返回 true 继续枚举, 否则返回 false.
        typedef VStd::function< bool(const ClassData* /*class data*/, const Uuid& /* typeId used only when RTTI is triggered, 0 the rest of the time */) > TypeInfoCB;
        /// Enumerate classId 类型的所有派生类此外我们还使用 typeId 来检查任何 rtti 可能的匹配.
        void EnumerateDerived(const TypeInfoCB& callback, const Uuid& classId = Uuid::CreateNull(), const Uuid& typeId = Uuid::CreateNull()) const;
        /// Enumerate classId 类型的所有基类.
        void EnumerateBase(const TypeInfoCB& callback, const Uuid& classId);
        template<class T>
        void EnumerateDerived(const TypeInfoCB& callback);
        template<class T>
        void EnumerateBase(const TypeInfoCB& callback);

        void EnumerateAll(const TypeInfoCB& callback, bool includeGenerics=false) const;
        // @}

        /// 制作 obj 的副本. 其行为与首先序列化 obj 然后从序列化数据创建副本相同.
        template<class T>
        T* CloneObject(const T* obj);
        void* CloneObject(const void* ptr, const Uuid& classId);

        template<class T>
        void CloneObjectInplace(T& dest, const T* obj);
        void CloneObjectInplace(void* dest, const void* ptr, const Uuid& classId);


        // 此处前面列出的类型将具有更高的优先级
        enum DataPatchUpgradeType
        {
            TYPE_UPGRADE,
            NAME_UPGRADE
        };

        // Base type used for single-node version upgrades
        class DataPatchUpgrade
        {
        public:
            VOBJECT_RTTI(DataPatchUpgrade, "{8eb39435-b068-4c4d-a375-228ec7aec07e}");
            V_CLASS_ALLOCATOR(DataPatchUpgrade, SystemAllocator, 0);
            

            DataPatchUpgrade(VStd::string_view fieldName, unsigned int fromVersion, unsigned int toVersion);
            virtual ~DataPatchUpgrade() = default;

            // This will only check to see if this has the same upgrade type and is applied to the same field.
            // Deeper comparisons are left to the individual upgrade types.
            bool operator==(const DataPatchUpgrade& RHS) const;

            // Used to sort nodes upgrades.
            bool operator<(const DataPatchUpgrade& RHS) const;

            virtual void Apply(V::SerializeContext& /*context*/, SerializeContext::DataElementNode& /*elementNode*/) const {}

            unsigned int FromVersion() const;
            unsigned int ToVersion() const;

            const VStd::string& GetFieldName() const;
            V::Crc32 GetFieldCRC() const;

            DataPatchUpgradeType GetUpgradeType() const;

            // Type Upgrade Interface Functions
            virtual V::TypeId GetFromType() const { return V::TypeId::CreateNull(); }
            virtual V::TypeId GetToType() const { return V::TypeId::CreateNull(); }

            virtual VStd::any Apply(const VStd::any& in) const { return in; }

            // Name upgrade interface functions
            virtual VStd::string GetNewName() const { return {}; }

        protected:
            VStd::string m_targetFieldName;
            V::Crc32 m_targetFieldCRC;
            unsigned int m_fromVersion;
            unsigned int m_toVersion;

            DataPatchUpgradeType m_upgradeType;
        };

        // Binary predicate for ordering per-version upgrades
        // When multiple upgrades exist from a particular version, we only want
        // to apply the one that upgrades to the maximum possible version.
        struct NodeUpgradeSortFunctor
        {
            // Provides sorting of lists of node upgrade pointers
            bool operator()(const DataPatchUpgrade* LHS, const DataPatchUpgrade* RHS)
            {
                if (LHS->ToVersion() == RHS->ToVersion())
                {
                    V_Assert(LHS->GetUpgradeType() != RHS->GetUpgradeType(), "Data Patch Upgrades with the same from/to version numbers must be different types.");

                    if (LHS->GetUpgradeType() == NAME_UPGRADE)
                    {
                        return true;
                    }

                    return false;
                }
                return LHS->ToVersion() < RHS->ToVersion();
            }
        };

        // A list of node upgrades, sorted in order of the version they convert to
        using DataPatchUpgradeList = VStd::set<DataPatchUpgrade*, NodeUpgradeSortFunctor>;
        // A sorted mapping of upgrade lists, keyed by (and sorted on) the version they convert from
        using DataPatchUpgradeMap = VStd::map<unsigned int, DataPatchUpgradeList>;
        // Stores sets of node upgrades keyed by the field they apply to
        using DataPatchFieldUpgrades = VStd::unordered_map<V::Crc32, DataPatchUpgradeMap>;

        // 用于维护和应用适用于单个字段的所有每字段节点升级的类.
        // 在构建现场数组时执行错误检查, 管理升级的生命周期,并
        // 处理升级对节点和原始值的应用。
        class DataPatchUpgradeHandler
        {
        public:
            DataPatchUpgradeHandler()
            {}

            ~DataPatchUpgradeHandler();

            // This function assumes ownership of the upgrade
            void AddFieldUpgrade(DataPatchUpgrade* upgrade);

            const DataPatchFieldUpgrades& GetUpgrades() const;

        private:
            DataPatchFieldUpgrades m_upgrades;
        };

        class DataPatchNameUpgrade : public DataPatchUpgrade
        {
        public:
            VOBJECT_RTTI(DataPatchNameUpgrade, "{4a4e4c66-b277-4df2-b740-b19d04821fd5}", DataPatchUpgrade);
            V_CLASS_ALLOCATOR(DataPatchNameUpgrade, SystemAllocator, 0);
            

            DataPatchNameUpgrade(unsigned int fromVersion, unsigned int toVersion, VStd::string_view oldName, VStd::string_view newName)
                : DataPatchUpgrade(oldName, fromVersion, toVersion)
                , m_newNodeName(newName)
            {
                m_upgradeType = NAME_UPGRADE;
            }

            ~DataPatchNameUpgrade() override = default;

            // The equivalence operator is used to determine if upgrades are functionally equivalent.
            // Two upgrades are considered equivalent (Incompatible) if they operate on the same field,
            // are the same type of upgrade, and upgrade from the same version.
            bool operator<(const DataPatchUpgrade& RHS) const;
            bool operator<(const DataPatchNameUpgrade& RHS) const;

            void Apply(V::SerializeContext& context, SerializeContext::DataElementNode& node) const override;
            using DataPatchUpgrade::Apply;

            VStd::string GetNewName() const override;

        private:
            VStd::string m_newNodeName;
        };

        template<class FromT, class ToT>
        class DataPatchTypeUpgrade : public DataPatchUpgrade
        {
        public:
            VOBJECT_RTTI(DataPatchTypeUpgrade, "{ef8e5120-bf2c-43f8-a91d-c3705a1dfb6c}", DataPatchUpgrade);
            V_CLASS_ALLOCATOR(DataPatchTypeUpgrade, SystemAllocator, 0);
            

            DataPatchTypeUpgrade(VStd::string_view nodeName, unsigned int fromVersion, unsigned int toVersion, VStd::function<ToT(const FromT& data)> upgradeFunc)
                : DataPatchUpgrade(nodeName, fromVersion, toVersion)
                , m_upgrade(VStd::move(upgradeFunc))
                , m_fromTypeID(vobject_rtti_typeid<FromT>())
                , m_toTypeID(vobject_rtti_typeid<ToT>())
            {
                m_upgradeType = TYPE_UPGRADE;
            }

            ~DataPatchTypeUpgrade() override = default;

            bool operator<(const DataPatchTypeUpgrade& RHS) const
            {
                return DataPatchUpgrade::operator<(RHS);
            }

            VStd::any Apply(const VStd::any& in) const override
            {
                const FromT& inValue = VStd::any_cast<const FromT&>(in);
                return VStd::any(m_upgrade(inValue));
            }

            void Apply(V::SerializeContext& context, SerializeContext::DataElementNode& node) const override
            {
                auto targetElementIndex = node.FindElement(m_targetFieldCRC);

                V_Assert(targetElementIndex >= 0, "Invalid node. Field %s is not a valid element of class %s (Version %d). Check your reflection function.", m_targetFieldName.data(), node.GetNameString(), node.GetVersion());

                if (targetElementIndex >= 0)
                {
                    V::SerializeContext::DataElementNode& targetElement = node.GetSubElement(targetElementIndex);

                    // We're replacing the target node so store the name off for use later
                    const char* targetElementName = targetElement.GetNameString();

                    FromT fromData;

                    // Get the current value at the target node
                    bool success = targetElement.GetData<FromT>(fromData);

                    V_Assert(success, "A single node type conversion of class %s (version %d) failed on field %s. The original node exists but isn't the correct type. Check your class reflection.", node.GetNameString(), node.GetVersion(), targetElementName);

                    if (success)
                    {
                        node.RemoveElement(targetElementIndex);

                        // Apply the user's provided data converter function
                        ToT toData = m_upgrade(fromData);

                        // Add the converted data back into the node as a new element with the same name
                        auto newIndex = node.AddElement<ToT>(context, targetElementName);
                        auto& newElement = node.GetSubElement(newIndex);
                        newElement.SetData(context, toData);
                    }
                }
            }

            V::TypeId GetFromType() const override
            {
                return m_fromTypeID;
            }

            V::TypeId GetToType() const override
            {
                return m_toTypeID;
            }

        private:
            VStd::function<ToT(const FromT& data)> m_upgrade;

            // Used for comparison of upgrade functions to determine uniqueness
            V::TypeId m_fromTypeID;
            V::TypeId m_toTypeID;
        };


        // 类元素. 当一个类没有直接的序列化器
        // 时, 他是ClassElements(可以是另一个类)的聚合.
        struct ClassElement
        {
            enum Flags
            {
                FLG_POINTER             = (1 << 0),       ///< Element is stored as pointer (it's not a value).
                FLG_BASE_CLASS          = (1 << 1),       ///< Set if the element is a base class of the holding class.
                FLG_NO_DEFAULT_VALUE    = (1 << 2),       ///< Set if the class element can't have a default value.
                FLG_DYNAMIC_FIELD       = (1 << 3),       ///< Set if the class element represents a dynamic field (DynamicSerializableField::m_data).
                FLG_UI_ELEMENT          = (1 << 4),       ///< Set if the class element represents a UI element tied to the ClassData of its parent.
            };

            enum class AttributeOwnership
            {
                Parent, // Attributes should be deleted when the ClassData containing us is destroyed
                Self,   // Attributes should be deleted when we are destroyed
                None,   // Attributes should never be deleted by us, their lifetime is managed somewhere else
            };

            ~ClassElement();

            ClassElement& operator=(const ClassElement& other);

            void ClearAttributes();
            Attribute* FindAttribute(AttributeId attributeId) const;

            const char* Name;
            u32         NameCrc;
            Uuid        TypeId;
            size_t      DataSize;
            size_t      Offset;

            IRttiHelper* VObjectRtti;
            GenericClassInfo*  GenericClassInfoPtr = nullptr;
            Edit::ElementData* EditDataPtr;
            VStd::vector<AttributeSharedPair, VStdFunctorAllocator> Attributes {
                VStdFunctorAllocator([]() -> IAllocatorAllocate& { return V::AllocatorInstance<V::SystemAllocator>::Get(); })
            };
            AttributeOwnership AttrOwnership = AttributeOwnership::Parent;
            int                Flags;
        };
        typedef VStd::vector<ClassElement> ClassElementArray;

        /// @brief 类数据包含每个注册类的所有成员
        /// (它们的偏移量等)、创建者、版本转换等的数据/信息.
        class ClassData
        {
        public:
            ClassData();
            ~ClassData() { ClearAttributes(); }
            ClassData(ClassData&&) = default;
            ClassData& operator=(ClassData&&) = default;
            template<class T>
            static ClassData Create(const char* name, const Uuid& typeUuid, IObjectFactory* factory, IDataSerializer* serializer = nullptr, IDataContainer* container = nullptr);

            bool    IsDeprecated() const { return Version == VersionClassDeprecated; }
            void    ClearAttributes();
            Attribute* FindAttribute(AttributeId attributeId) const;
            ///< Checks if the supplied typeid can be converted to an instance of the class represented by this Class Data
            ///< @param convertibleTypeId type to check to determine if it can converted to an element of class represent by this Class Data
            ///< @return if the classData can store the convertible type element true is returned
            bool CanConvertFromType(const TypeId& convertibleTypeId, V::SerializeContext& serializeContext) const;
            ///< Retrieves a memory address that can be used store an element of the convertible type
            ///< @param resultPtr output parameter that is populated with the memory address that can be used to store an element of the convertible type
            ///< @param convertibleTypeId type to check to determine if it can converted to an element of class represent by this Class Data
            ///< @param classPtr memory address of the class represented by the ClassData
            ///< @return true if a non-null memory address has been returned that can store the  convertible type
            bool ConvertFromType(void*& convertibleTypePtr, const TypeId& convertibleTypeId, void* classPtr, V::SerializeContext& serializeContext) const;

            /// Find the persistence id (check base classes) \todo this is a TEMP fix, analyze and cache that information in the class
            ClassPersistentId GetPersistentId(const SerializeContext& context) const;

            const char*         Name;
            Uuid                TypeId;
            unsigned int        Version;             ///< 数据版本(默认0)
            VersionConverter    Converter;           ///< 数据版本转换器,一个静态成员,不需要实例来转换其数据.
            IObjectFactory*     Factory;             ///< 对象创建接口.
            ClassPersistentId   PersistentId;        ///< 检索类实例持久 ID 的函数.
            ClassDoSave         DoSave;              ///< 函数将选择保存或不保存实例.
            IDataSerializerPtr  SerializerPtr;       ///< 实际数据序列化的接口. 如果这不是 NULL m_elements 必须为空.
            IEventHandler*      EventHandlerPtr;     ///< 事件通知的可选接口(开始/停止序列化等)

            IDataContainer*     ContainerPtr;        ///< 如果此类代表数据容器,则为接口.将使用此接口访问数据.
            IRttiHelper*        VObjectRtti;         ///< 用于支持RTTI的接口.根据提供给 Class<T>的类型进行内部设置.
            IDataConverter*     DataConverterPtr{};  ///< 用于将不相关类型转换为此类元素的接口

            Edit::ClassData*    EditDataPtr;         ///< 编辑Class显示数据.
            ClassElementArray   Elements;            ///< 子元素.如果这不为空,则 m_serializer 应该
                                                     ///< 为 NULL(如果我们可以序列化整个类,则没有必要拥有子元素).

            // 在序列化期间应用的单节点升级的集合 
            // 映射由升级所转换的版本作为键, 然后
            // 升级按照升级到的版本的顺序进行排序
            DataPatchUpgradeHandler DataPatchUpgrader;

            /// 该类类型的属性. 这里需要 Lambda，因为 VStdFunctorAllocator 需要一个返回 IAllocatorAllocate& 的函数指针 
            /// 而 V::AllocatorInstance<V::SystemAllocator>::Get 返回一个 V::SystemAllocator&, 虽然它继承自 IAllocatorAllocate, 
            /// 但它不像函数指针那样工作不支持协变返回类型.
            VStd::vector<AttributeSharedPair, VStdFunctorAllocator> Attributes{VStdFunctorAllocator(&GetSystemAllocator) };

        private:
            static IAllocatorAllocate& GetSystemAllocator()
            {
                return V::AllocatorInstance<V::SystemAllocator>::Get();
            }
        };

        /// @brief 用于从序列化器创建和销毁对象的接口.
        class IObjectFactory
        {
        public:

            virtual ~IObjectFactory() {}

            /// Called to create an instance of an object.
            virtual void* Create(const char* name) = 0;

            /// Called to destroy an instance of an object
            virtual void  Destroy(void* ptr) = 0;
            void Destroy(const void* ptr)
            {
                Destroy(const_cast<void*>(ptr));
            }
        };

        /// @brief 数据序列化接口. 应针对最低级别的数据实施. 
        //  一旦检测到此实现,就不会向下钻取该类. 我们假设这个实现覆盖了整个类.
        class IDataSerializer
        {
        public:
            static IDataSerializerDeleter CreateDefaultDeleteDeleter();
            static IDataSerializerDeleter CreateNoDeleteDeleter();

            virtual ~IDataSerializer() {}

            /// 将类数据存储到流中.
            virtual size_t  Save(const void* classPtr, IO::GenericStream& stream, bool isDataBigEndian = false) = 0;

            /// 从流中加载类数据.
            virtual bool    Load(void* classPtr, IO::GenericStream& stream, unsigned int version, bool isDataBigEndian = false) = 0;

            /// 将二进制数据转换为文本.
            virtual size_t  DataToText(IO::GenericStream& in, IO::GenericStream& out, bool isDataBigEndian /*= false*/) = 0;

            /// 将文本数据转换为二进制. 如果文本->二进制格式发生变化，我们必须保持向下兼容!
            virtual size_t  TextToData(const char* text, unsigned int textVersion, IO::GenericStream& stream, bool isDataBigEndian = false) = 0;

            /// 比较该类型的两个实例.
            /// @return 如果它们匹配则为 true.
            /// @Note: 假设输入指针指向该类的有效实例.
            virtual bool    CompareValueData(const void* lhs, const void* rhs) = 0;

            /// 对克隆数据进行可选的后处理, 以处理未序列化反映的成员.
            virtual void PostClone(void* /*classPtr*/) {}
        };

        /// @brief  用于直接比较给定类型的两个实例的帮助器.
        /// 旨在用于 IDataSerializer::CompareValueData 的实现.
        template<typename T>
        struct EqualityCompareHelper
        {
            static bool CompareValues(const void* lhs, const void* rhs)
            {
                const T* dataLhs = reinterpret_cast<const T*>(lhs);
                const T* dataRhs = reinterpret_cast<const T*>(rhs);
                return (*dataLhs == *dataRhs);
            }
        };

        /// @brief 数据容器的接口. 这可能是一个 VStd 容器, 或者只
        /// 是一个包含以某种模板方式定义的元素的类(通常使用模板:))
        class IDataContainer
        {
        public:
            typedef VStd::function< bool (void* /* instance pointer */, const Uuid& /*elementClassId*/, const ClassData* /* elementGenericClassData */, const ClassElement* /* genericClassElement */) > ElementCB;
            typedef VStd::function< bool(const Uuid& /*elementClassId*/, const ClassElement* /* genericClassElement */) > ElementTypeCB;
            virtual ~IDataContainer() {}

            /// Mix-in for associative container actions, implement or provide this to offer key/value actions
            class IAssociativeDataContainer
            {
            protected:
                /// Reserve a key and get its address. Used by CreateKey.
                virtual void*   AllocateKey() = 0;
                /// Deallocates a key created by ReserveKey. Used by CreateKey.
                virtual void    FreeKey(void* key) = 0;

            public:
                virtual ~IAssociativeDataContainer() {}

                struct KeyPtrDeleter
                {
                    KeyPtrDeleter(IAssociativeDataContainer* associativeDataContainer)
                        : m_associativeDataContainer(associativeDataContainer)
                    {}

                    void operator()(void* key)
                    {
                        m_associativeDataContainer->FreeKey(key);
                    }

                    IAssociativeDataContainer* m_associativeDataContainer{};
                };

                /// Reserve a key that can be used for associative container operations.
                VStd::shared_ptr<void> CreateKey()
                {
                    return VStd::shared_ptr<void>(AllocateKey(), KeyPtrDeleter(this));
                }
                /// Get an element's address by its key. Not used for serialization.
                virtual void*   GetElementByKey(void* instance, const ClassElement* classElement, const void* key) = 0;
                /// Populates element with key (for associative containers). Not used for serialization.
                virtual void    SetElementKey(void* element, void* key) = 0;
            };

            /// Return default element generic name (used by most containers).
            static inline const char*   GetDefaultElementName()     { return "element"; }
            /// Return default element generic name crc (used by most containers).
            static inline u32 GetDefaultElementNameCrc()            { return V_CRC("element", 0x41405e39); }

            // Returns default element generic name unless overridden by an IDataContainer
            virtual const char* GetElementName([[maybe_unused]] int index = 0) { return GetDefaultElementName(); }
            // Returns default element generic name crc unless overridden by an IDataContainer
            virtual u32 GetElementNameCrC([[maybe_unused]] int index = 0) { return GetDefaultElementNameCrc(); }

            /// Returns the generic element (offsets are mostly invalid 0xbad0ffe0, there are exceptions). Null if element with this name can't be found.
            virtual const ClassElement* GetElement(u32 elementNameCrc) const = 0;
            /// Populates the supplied classElement by looking up the name in the DataElement. Returns true if the classElement was populated successfully
            virtual bool GetElement(ClassElement& classElement, const DataElement& dataElement) const = 0;
            /// Enumerate elements in the container based on the stored entries.
            virtual void    EnumElements(void* instance, const ElementCB& cb) = 0;
            /// Enumerate elements in the container based on possible storage types. If the callback is not called it means there are no restrictions on what can be stored in the container.
            virtual void    EnumTypes(const ElementTypeCB& cb) = 0;
            /// Return number of elements in the container.
            virtual size_t  Size(void* instance) const = 0;
            /// Returns the capacity of the container. Returns 0 for objects without fixed capacity.
            virtual size_t  Capacity(void* instance) const = 0;
            /// Returns true if elements pointers don't change on add/remove. If false you MUST enumerate all elements.
            virtual bool    IsStableElements() const = 0;
            /// Returns true if the container is fixed size (not capacity), otherwise false.
            virtual bool    IsFixedSize() const = 0;
            /// Returns if the container is fixed capacity, otherwise false
            virtual bool    IsFixedCapacity() const = 0;
            /// Returns true if the container represents a smart pointer.
            virtual bool    IsSmartPointer() const = 0;
            /// Returns true if elements can be retrieved by index.
            virtual bool    CanAccessElementsByIndex() const = 0;
            /// Returns the associative interface for this container (e.g. the container itself if it inherits it) if available, otherwise null.
            virtual IAssociativeDataContainer* GetAssociativeContainerInterface() { return nullptr; }
            /// Reserve an element and get its address (called before the element is loaded).
            virtual void*   ReserveElement(void* instance, const ClassElement* classElement) = 0;
            /// Free an element that was reserved using ReserveElement, but was not stored by calling StoreElement.
            virtual void    FreeReservedElement(void* instance, void* element, SerializeContext* deletePointerDataContext)
            {
                RemoveElement(instance, element, deletePointerDataContext);
            }
            /// Get an element's address by its index (called before the element is loaded).
            virtual void*   GetElementByIndex(void* instance, const ClassElement* classElement, size_t index) = 0;
            /// Store the element that was reserved before (called post loading)
            virtual void    StoreElement(void* instance, void* element) = 0;
            /// Remove element in the container. Returns true if the element was removed, otherwise false. If deletePointerDataContext is NOT null, this indicated that you want the remove function to delete/destroy any Elements that are pointer!
            virtual bool    RemoveElement(void* instance, const void* element, SerializeContext* deletePointerDataContext) = 0;
            /**
             * Remove elements (removed array of elements) regardless if the container is Stable or not (IsStableElements).
             * Element should be sorted on address in acceding order. Returns number of elements removed.
             * If deletePointerDataContext is NOT null, this indicates that you want the remove function to delete/destroy any Elements that are pointer,
             */
            virtual size_t  RemoveElements(void* instance, const void** elements, size_t numElements, SerializeContext* deletePointerDataContext) = 0;
            /// Clear elements in the instance. If deletePointerDataContext is NOT null, this indicated that you want the remove function to delete/destroy any Elements that are pointer!
            virtual void    ClearElements(void* instance, SerializeContext* deletePointerDataContext) = 0;
            /// Called when elements inside the container have been modified.
            virtual void    ElementsUpdated(void* instance);

        protected:
            /// Free element data (when the class elements are pointers).
            void DeletePointerData(SerializeContext* context, const ClassElement* classElement, const void* element);
        };

        /// @brief Serialize class events.
        /// 可以从序列化线程调用序列化事件. 所以所有函数都应该是线程安全的.
        class IEventHandler
        {
        public:
            virtual ~IEventHandler() {}

            /// 在我们开始读取 classPtr 指向的实例之前调用.
            virtual void OnReadBegin(void* classPtr) { (void)classPtr; }
            /// 在我们完成从 classPtr 指向的实例的读取后调用.
            virtual void OnReadEnd(void* classPtr) { (void)classPtr; }

            /// 在我们开始写入 classPtr 指向的实例之前调用.
            virtual void OnWriteBegin(void* classPtr) { (void)classPtr; }
            /// 在我们完成对 classPtr 指向的实例的写入后调用.
            /// NOTE: 使用此回调时必须小心。当 ID 重新映射发生时调用,
            /// 实例是克隆的或实例是从对象流加载的.
            /// 这意味着在从 ObjectStream 序列化新实例或克隆对象的过程中可以多次调用此函数.
            virtual void OnWriteEnd(void* classPtr) { (void)classPtr; }

            /// 在我们开始对 classPtr 指向的实例进行数据修补之前调用.
            virtual void OnPatchBegin(void* classPtr, const DataPatchNodeInfo& patchInfo) { (void)classPtr; (void)patchInfo; }
            /// 在我们完成对 classPtr 指向的实例的数据修补后调用.
            virtual void OnPatchEnd(void* classPtr, const DataPatchNodeInfo& patchInfo) { (void)classPtr; (void)patchInfo; }

            /// 从源数据流(例如 ObjectStream::Load 或 JsonSerialization::Load)加载实例后调用
            virtual void OnLoadedFromObjectStream(void* classPtr) { (void)classPtr; }
            /// 在 SerializeContext::EndCloneObject 中克隆对象后调用
            virtual void OnObjectCloned(void* classPtr) { (void)classPtr; }
        };

        class IDataConverter
        {
        public:
            virtual ~IDataConverter() = default;
            /// @brief 回调以确定提供的可转换类型是否可以存储在 classData 的实例中
            /// @param convertibleTypeId 类型来检查以确定它是否可以转换为由此类数据表示的类的元素
            /// @param classData 对表示存储在 classPtr 中的类型的元数据的引用
            /// @return 如果classData可以存储可转换类型元素则返回true
            virtual bool CanConvertFromType (const TypeId& convertibleTypeId, const SerializeContext::ClassData& classData, SerializeContext& /*serializeContext*/)
            {
                return classData.TypeId == convertibleTypeId;
            }
            
            /// @brief 可用于检索存储所提供的可转换类型元素的内存地址的回调
            /// @param convertibleTypePtr 应使用可存储可转换类型元素的地址填充结果指针
            /// @param convertibleTypeId  类型来检查以确定它是否可以转换为由此类数据表示的类的元素
            /// @param classPtr @classData类型表示的类的内存地址
            /// @param classData 对表示存储在 classPtr 中的类型的元数据的引用
            /// @return true 如果已返回可存储可转换类型的非空内存地址
            virtual bool ConvertFromType(void*& convertibleTypePtr, const TypeId& convertibleTypeId, void* classPtr, const SerializeContext::ClassData& classData, SerializeContext& /*serializeContext*/)
            {
                if (classData.TypeId == convertibleTypeId)
                {
                    convertibleTypePtr = classPtr;
                    return true;
                }

                return false;
            }
        };

        /// @brief 表示序列化数据树中的一个元素.
        /// 每个元素都包含有关其自身的元数据和(可能)一个数据值.
        /// 表示 int 的元素将具有数据值, 但表示向量或类的元素
        /// 则不会(它们的内容存储在子元素中).
        struct DataElement
        {
            DataElement();
            ~DataElement();
            DataElement(const DataElement& rhs);
            DataElement& operator = (const DataElement& rhs);
            DataElement(DataElement&& rhs);
            DataElement& operator = (DataElement&& rhs);

            enum DataType
            {
                DT_TEXT,        ///< data points to a string representation of the data
                DT_BINARY,      ///< data points to a binary representation of the data in native endian
                DT_BINARY_BE,   ///< data points to a binary representation of the data in big endian
            };

            const char*     Name;                                    ///< 参数的名称，它们在类的范围内必须是唯一的.
            u32             NameCrc;                                 ///< CRC32 of name.
            DataType        DataCategory;                            ///< 如果有的话,是什么类型的数据.
            Uuid            Id = V::Uuid::CreateNull();              ///< 参考 ID, 其含义可能会根据我们引用的内容而变化.
            unsigned int    Version;                                 ///< 流中数据的版本. 这可以是当前版本或更旧的版本. 新版本将在内部处理.
            size_t          DataSize;                                ///< "data" 指向的数据大小(以字节为单位).

            VStd::vector<char> Buffer;                               ///< 当 DataElement 需要拥有数据时 ByteContainerStream 使用的本地缓冲区.
            IO::ByteContainerStream<VStd::vector<char> > ByteStream; ///< 当 DataElement 需要拥有数据时使用流.

            IO::GenericStream* StreamPtr; ///< 指向保存元素数据的流的指针，它可能指向 ByteStream.
        };

        /// @brief 表示序列化数据树中的一个节点.
        /// 保存一个描述自身的 DataElement 和一个子节点列表
        /// 例如, 一个类将表示为父节点, 其成员变量位于子节点中.
        class DataElementNode
        {
            friend class ObjectStreamInternal::ObjectStreamImpl;
            friend class DataOverlayTarget;

        public:
            DataElementNode()
                : m_classData(nullptr) {}

            /**
             * Get/Set data work only on leaf nodes
             */
            template <typename T>
            bool GetData(T& value, ErrorHandler* errorHandler = nullptr);
            template <typename T>
            bool GetChildData(u32 childNameCrc, T& value);
            template <typename T>
            bool SetData(SerializeContext& sc, const T& value, ErrorHandler* errorHandler = nullptr);

            //! @deprecated Use GetData instead
            template <typename T>
            bool GetDataHierarchy(SerializeContext&, T& value, ErrorHandler* errorHandler = nullptr);

            template <typename T>
            bool FindSubElementAndGetData(V::Crc32 crc, T& outValue);

            /**
             * Converts current DataElementNode from one type to another.
             * Keep in mind that if the new "type" has sub-elements (not leaf - serialized element)
             * you will need to add those elements after calling convert.
             */
            bool Convert(SerializeContext& sc, const char* name, const Uuid& id);
            bool Convert(SerializeContext& sc, const Uuid& id);
            template <typename T>
            bool Convert(SerializeContext& sc, const char* name);
            template <typename T>
            bool Convert(SerializeContext& sc);

            DataElement&        GetRawDataElement()             { return m_element; }
            const DataElement&  GetRawDataElement() const       { return m_element; }
            u32                 GetName() const                 { return m_element.NameCrc; }
            const char*         GetNameString() const           { return m_element.Name; }
            void                SetName(const char* newName);
            unsigned int        GetVersion() const              { return m_element.Version; }
            void                SetVersion(unsigned int version) { m_element.Version = version; }
            const Uuid&         GetId() const                   { return m_element.Id; }

            int                 GetNumSubElements() const       { return static_cast<int>(m_subElements.size()); }
            DataElementNode&    GetSubElement(int index)        { return m_subElements[index]; }
            int                 FindElement(u32 crc);
            DataElementNode*    FindSubElement(u32 crc);
            void                RemoveElement(int index);
            bool                RemoveElementByName(u32 crc);
            int                 AddElement(const DataElementNode& elem);
            int                 AddElement(SerializeContext& sc, const char* name, const Uuid& id);
            int                 AddElement(SerializeContext& sc, const char* name, const ClassData& classData);
            int                 AddElement(SerializeContext& sc, VStd::string_view name, GenericClassInfo* genericClassInfo);
            template <typename T>
            int                 AddElement(SerializeContext& sc, const char* name);
            /// @returns index of the replaced element (index) if replaced, otherwise -1
            template <typename T>
            int                 AddElementWithData(SerializeContext& sc, const char* name, const T& dataToSet);
            int                 ReplaceElement(SerializeContext& sc, int index, const char* name, const Uuid& id);
            template <typename T>
            int                 ReplaceElement(SerializeContext& sc, int index, const char* name);

        protected:
            typedef VStd::vector<DataElementNode> NodeArray;

            bool SetDataHierarchy(SerializeContext& sc, const void* objectPtr, const Uuid& classId, ErrorHandler* errorHandler = nullptr, const ClassData* classData = nullptr);
            bool GetDataHierarchy(void* objectPtr, const Uuid& classId, ErrorHandler* errorHandler = nullptr);

            struct DataElementInstanceData
            {
                void* Ptr = nullptr;
                DataElementNode* DataElement = nullptr;
                int CurrentContainerElementIndex = 0;
            };

            using NodeStack = VStd::list<DataElementInstanceData>;
            bool GetDataHierarchyEnumerate(ErrorHandler* errorHandler, NodeStack& nodeStack);
            bool GetClassElement(ClassElement& classElement, const DataElementNode& parentDataElement, ErrorHandler* errorHandler) const;

            DataElement         m_element; ///< Serialization data for this element.
            const ClassData*    m_classData; ///< Reflected ClassData for this element.
            NodeArray           m_subElements; ///< Nodes of sub elements.
        };

        struct EnumerateInstanceCallContext
        {
            EnumerateInstanceCallContext(const BeginElemEnumCB& beginElemCB, const EndElemEnumCB& endElemCB, const SerializeContext* context, unsigned int accessflags, ErrorHandler* errorHandler);

            BeginElemEnumCB                 BeginElemCB;          ///< Optional callback when entering an element's hierarchy.
            EndElemEnumCB                   EndElemCB;            ///< Optional callback when exiting an element's hierarchy.
            unsigned int                    AccessFlags;          ///< Data access flags for the enumeration, see \ref EnumerationAccessFlags.
            ErrorHandler*                   ErrorHandlerPtr;         ///< Optional user error handler.
            const SerializeContext*         Context;              ///< Serialize context containing class reflection required for data traversal.

            IDataContainer::ElementCB       ElementCallback;      // Pre-bound functor computed internally to avoid allocating closures during traversal.
            ErrorHandler                    DefaultErrorHandler;  // If no custom error handler is provided, the context provides one.
        };

          /// Find a class data (stored information) based on a class ID and possible parent class data.
        const ClassData* FindClassData(const Uuid& classId, const SerializeContext::ClassData* parent = nullptr, u32 elementNameCrc = 0) const;

        /// Find a class data (stored information) based on a class name
        VStd::vector<V::Uuid> FindClassId(const V::Crc32& classNameCrc) const;

        /// Find GenericClassData data based on the supplied class ID
        GenericClassInfo* FindGenericClassInfo(const Uuid& classId) const;

        /// Creates an VStd::any based on the provided class Uuid, or returns an empty VStd::any if no class data is found or the class is virtual
        VStd::any CreateAny(const Uuid& classId);

        /// 向 SerializeContext 注册 GenericClassInfo
        void RegisterGenericClassInfo(const V::Uuid& typeId, GenericClassInfo* genericClassInfo, const CreateAnyFunc& createAnyFunc);

        /// 注销当前模块中注册的所有 GenericClassInfo 实例并删除 GenericClassInfo 实例
        void CleanupModuleGenericClassInfo();


        /// 检查是否可以使用 Uuid 和 VOBJECT_RTTI 将一种类型向下转换为另一种类型.
        /// 所有类都必须在系统中注册.
        bool  CanDowncast(const Uuid& fromClassId, const Uuid& toClassId, const IRttiHelper* fromClassHelper = nullptr, const IRttiHelper* toClassHelper = nullptr) const;


        /// 将指针从派生类偏移到公共基类 所有类都必须向系统注册, 否则将返回 NULL.
        void* DownCast(void* instance, const Uuid& fromClassId, const Uuid& toClassId, const IRttiHelper* fromClassHelper = nullptr, const IRttiHelper* toClassHelper = nullptr) const;

        /**
         * Casts a pointer using the instance pointer and its class id.
         * In order for the cast to succeed, both types must either support vobject_rtti or be reflected
         * with the serialization context.
         */

        /// 使用实例指针及其类 ID 转换指针.
        /// 为了使转换成功, 两种类型都必须支持 vobject_rtti 或通过序列化上下文反映.
        template<class T>
        T Cast(void* instance, const Uuid& instanceClassId) const;

        void RegisterDataContainer(VStd::unique_ptr<IDataContainer> dataContainer);

        /// 如果提供的 TypeId 表示 C++ 枚举类型并且该枚举类型已
        /// 向 SerializeContext 注册, 则检索基础 typeid, 否则返回提供的 typeId.
        const TypeId& GetUnderlyingTypeId(const TypeId& enumTypeId) const;
    private:
        /// 调用枚举函数来枚举 vobject_rtti 层次结构
        static void EnumerateBaseRTTIEnumCallback(const Uuid& id, void* userData);

        /// Remove class data
        void RemoveClassData(ClassData* classData);
        /// Removes the GenericClassInfo from the GenericClassInfoMap
        void RemoveGenericClassInfo(GenericClassInfo* genericClassInfo);

        /// Adds class data, including base class element data
        template<class T, class BaseClass>
        void AddClassData(ClassData* classData, size_t index);
        template<class T, class...TBaseClasses>
        void AddClassData(ClassData* classData);

        /// Object cloning callbacks.
        bool BeginCloneElement(void* ptr, const ClassData* classData, const ClassElement* elementData, void* stackData, ErrorHandler* errorHandler, VStd::vector<char>* scratchBuffer);
        bool BeginCloneElementInplace(void* rootDestPtr, void* ptr, const ClassData* classData, const ClassElement* elementData, void* stackData, ErrorHandler* errorHandler, VStd::vector<char>* scratchBuffer);
        bool EndCloneElement(void* stackData);

    
        /// Internal structure to maintain class information while we are describing a class.
        /// User should call variety of functions to describe class features and data.
        /// example:
        /// struct MyStruct {
        ///    int m_data };
        ///
        /// expose for serialization
        /// serializeContext->Class<MyStruct>()
        ///      ->Version(3,&MyVersionConverter)
        ///      ->Field("data",&MyStruct::m_data);
    public:
        class ClassBuilder {
            friend class SerializeContext;
            ClassBuilder(SerializeContext *ctx, const IdToClassMap::iterator& classMapIter)
            : m_context(ctx)
            , m_classData(classMapIter) {
                if (!ctx->IsRemovingReflection()) {
                    m_currentAttributes  = &classMapIter->second.Attributes;
                }
            }
        public:
            ~ClassBuilder();
            ClassBuilder* operator->()  { return this; }

            /// @brief 使用类中变量的地址声明类字段
            template<class ClassType, class FieldType>
            ClassBuilder* Field(const char* name, FieldType ClassType::* address, VStd::initializer_list<AttributePair> attributeIds = {});

            /// @brief 声明序列化字段的类型更改
            /// 这些被序列化器用来修复旧的数据补丁类型升级将在名称升级之前应用.
            /// 如果类型和名称更改同时发生, 则提供给类型转换器的字段名称应为旧名称.
            template <class FromT, class ToT>
            ClassBuilder* TypeChange(VStd::string_view fieldName, unsigned int fromVersion, unsigned int toVersion, VStd::function<ToT(const FromT&)> upgradeFunc);
            
            /// @brief 声明序列化字段的名称更改这些由序列化器用来修复旧的数据补丁
            ClassBuilder* NameChange(unsigned int fromVersion, unsigned int toVersion, VStd::string_view oldFieldName, VStd::string_view newFieldName);

            /// @brief 声明属于该类的基类的类字段. 
            /// 如果您使用此函数来反映基类, 则无法反映整个基类.
            template<class ClassType, class BaseType, class FieldType>
            ClassBuilder* FieldFromBase(const char* name, FieldType BaseType::* address);

            /// @brief 使用可选转换器向结构添加版本控制. 
            /// 如果不受控制,则分配版本 0.
            ClassBuilder* Version(unsigned int version, VersionConverter converter = nullptr);

            /// 对于数据类型(通常是基类型)或我们处理完整序列化的类型, 实现此接口. 
            /// 取得提供的序列化器的所有权.
            ClassBuilder* Serializer(IDataSerializerPtr serializer);

            /// 对于数据类型(通常是基类型)或我们处理完整序列化的类型, 实现此接口.
            ClassBuilder* Serializer(IDataSerializer* serializer);

            /// @brief 用于创建特定序列化器实现的静态实例的帮助函数. 
            /// @ref Serializer(IDataSerializer*)
            template<typename SerializerImplementation>
            ClassBuilder* Serializer()
            {
                return Serializer(&Serialize::StaticInstance<SerializerImplementation>::s_instance);
            }

            /// @brief 对于空的类类型, 我们希望序列化器在加载时创建, 但没有子元素.
            ClassBuilder* SerializeWithNoData();

            /// @brief 必要时实现并提供事件处理接口. 一个例子是当类执行一些线程工作并且需要知道我们何时要访问
            /// 数据以进行序列化(加载/保存)时.
            ClassBuilder* EventHandler(IEventHandler* eventHandler);

            /// 用于创建特定事件处理程序实现的静态实例的辅助函数. 
            /// @ref Serializer(IDataSerializer*)
            template<typename EventHandlerImplementation>
            ClassBuilder* EventHandler()
            {
                return EventHandler(&Serialize::StaticInstance<EventHandlerImplementation>::s_instance);
            }

            /// 添加 DataContainer 结构，用于以自定义方式操作包含的数据
            ClassBuilder* DataContainer(IDataContainer* dataContainer);

            /// 帮助函数创建特定数据容器实现的静态实例. 
            /// @ref DataContainer(IDataContainer*)
            template<typename DataContainerType>
            ClassBuilder* DataContainer()
            {
                return DataContainer(&Serialize::StaticInstance<DataContainerType>::s_instance);
            }

            /// @brief 设置类持久 ID 函数 getter. 当我们将类存储在容器中时使用, 因此我们可以识别元素以覆盖
            /// 值(也称为切片覆盖), 否则由于缺乏可靠的方法来寻址元素, 在容器中调整复杂类型的能力将受到限制在
            /// 一个容器中.
            ClassBuilder* PersistentId(ClassPersistentId persistentId);

            /// @brief 提供一个函数来执行基本的是/否保存. 我们不建议使用这种模式, 因为它很容易创建
            /// 不对称序列化并默默地丢失数据.
            ClassBuilder* SerializerDoSave(ClassDoSave isSave);

            /// @brief 所有 T(属性值)必须是可复制或移动构造的,因为它们存储在内部 AttributeContainer<T> 中, 可
            /// 由 vobject_rtti 和 AttributeData 访问.属性可以分配给类或数据元素.
            template <class T>
            ClassBuilder* Attribute(const char* id, T&& value)
            {
                return Attribute(Crc32(id), VStd::forward<T>(value));
            }

            /// @brief 所有 T(属性值)必须是可复制或移动构造的, 因为它们存储在内部 AttributeContainer<T> 
            /// 中, 可由 vobject_rtti 和 AttributeData 访问.
            template <class T>
            ClassBuilder* Attribute(Crc32 idCrc, T&& value);
        private:
            SerializeContext*      m_context;
            IdToClassMap::iterator m_classData;
            VStd::vector<AttributeSharedPair, VStdFunctorAllocator> * m_currentAttributes = nullptr;
        }; // class ClassBuilder

        // Internal structure to maintain enum information while we are describing a enum type.
        // This can be used to name the values of the enumeration for serialization.
        // example:
        // enum MyEnum
        // {
        //     First,
        //     Second,
        //     Fourth = 4
        // };
        //
        // expose for serialization
        //  serializeContext->Enum<MyEnum>()
        //      ->Value("First",&MyEnum::First)
        //      ->Value("Second",&MyEnum::Second)
        //      ->Value("Fourth",&MyEnum::Fourth);
        class EnumBuilder
        {
            friend class SerializeContext;
            EnumBuilder(SerializeContext* context, const IdToClassMap::iterator& classMapIter);
        public:
            ~EnumBuilder() = default;
            auto operator->() -> EnumBuilder*;

            //! 声明具有特定值的枚举字段
            template<class EnumType>
            auto Value(const char* name, EnumType value)->EnumBuilder*;

            //! 使用可选转换器向结构添加版本控制. 如果不受控制, 则分配版本 0.
            auto Version(unsigned int version, VersionConverter converter = nullptr) -> EnumBuilder*;

            //! 对于数据类型(通常是基类型)或我们处理完整序列化的类型, 实现此接口.
            auto Serializer(IDataSerializerPtr serializer) -> EnumBuilder*;

            //! 用于创建特定序列化器实现的静态实例的帮助函数. 
            //! @ref Serializer(IDataSerializer*)
            template<typename SerializerImplementation>
            auto Serializer()->EnumBuilder*;

            /**
             * Implement and provide interface for event handling when necessary.
             * An example for this will be when the class does some thread work and need to know when
             * we are about to access data for serialization (load/save).
             */
            auto EventHandler(IEventHandler* eventHandler) -> EnumBuilder*;

            //! 用于创建特定事件处理程序实现的静态实例的辅助函数. 
            //! @ref Serializer(IDataSerializer*)
            template<typename EventHandlerImplementation>
            auto EventHandler()->EnumBuilder*;

            //! 添加 DataContainer 结构, 用于以自定义方式操作包含的数据.
            auto DataContainer(IDataContainer* dataContainer) -> EnumBuilder*;

            //! 创建特定数据容器实现的静态实例的辅助函数. 
            //! @ref DataContainer(IDataContainer*)
            template<typename DataContainerType>
            auto DataContainer()->EnumBuilder*;

            //! 设置类持久 ID 函数 getter. 当我们将类存储在容器中时使用, 因此我们可以识别
            //! 元素以覆盖值(也称为切片覆盖), 否则由于缺乏可靠的方法来寻址元素, 在容器中调
            //! 整复杂类型的能力将受到限制在一个容器中.
            auto PersistentId(ClassPersistentId persistentId) -> EnumBuilder*;

            //! 所有 T(属性值)必须是可复制或移动构造的, 因为它们存储在内部 AttributeContainer<T> 中,可由
            //! vobject_rtti 和 AttributeData 访问. 属性可以分配给类或数据元素.
            template <class T>
            auto Attribute(Crc32 idCrc, T&& value) -> EnumBuilder*;

        private:
            SerializeContext* m_context;
            IdToClassMap::iterator m_classData;
            VStd::vector<AttributeSharedPair, VStdFunctorAllocator>* m_currentAttributes = nullptr;
        };
    private:
        EditContext* m_editContext;  ///< 指向可选编辑上下文的指针.
        IdToClassMap m_uuidMap;      ///< 此序列化上下文中所有类的映射.
        VStd::unordered_multimap<V::Crc32, V::Uuid> m_classNameToUuid;            ///< 将所有类名映射到它们的 uuid
        VStd::unordered_multimap<Uuid, GenericClassInfo*>  m_uuidGenericMap;      ///< 使用 GenericTypeInfo 反射类的 Uuid 到 ClassData 映射
        VStd::unordered_multimap<Uuid, Uuid> m_legacySpecializeTypeIdToTypeIdMap; ///< 保留模板类的旧遗留专用类型 ID 到新专用类型 ID 的映射
        VStd::unordered_map<Uuid, CreateAnyFunc>  m_uuidAnyCreationMap;           ///< Uuid 到 Any 创建函数映射
        VStd::unordered_map<TypeId, TypeId> m_enumTypeIdToUnderlyingTypeIdMap;    ///< Uuid 用于跟踪枚举类型对应的底层类型 id，该枚举类型反映为 SerializeContext 中的字段
        VStd::vector<VStd::unique_ptr<IDataContainer>> m_dataContainers;          ///< 照顾所有相关 IDataContainer 的生命周期

        class PerModuleGenericClassInfo;
        VStd::unordered_set<PerModuleGenericClassInfo*>  m_perModuleSet; ///< Stores the static PerModuleGenericClass structures keeps track of reflected GenericClassInfo per module

        friend PerModuleGenericClassInfo& GetCurrentSerializeContextModule(); 
    
    }; // class SerializeContext

    SerializeContext::PerModuleGenericClassInfo& GetCurrentSerializeContextModule();

    class GenericClassInfo
    {
    public:
        GenericClassInfo()
        { }

        virtual ~GenericClassInfo() {}

        /// Return the generic class "class Data" independent from the underlaying templates
        virtual SerializeContext::ClassData* GetClassData() = 0;

        virtual size_t GetNumTemplatedArguments() = 0;

        virtual const Uuid& GetTemplatedTypeId(size_t element) = 0;

        /// By default returns VObject<ValueType>::Id
        virtual const Uuid& GetSpecializedTypeId() const = 0;

        /// Return the generic Type Id associated with the GenericClassInfo
        virtual const Uuid& GetGenericTypeId() const { return GetSpecializedTypeId(); }

        /// Register the generic class info using the SerializeContext
        virtual void Reflect(SerializeContext*) = 0;

        /// Returns true if the generic class can store the supplied type
        virtual bool CanStoreType(const Uuid& typeId) const { return GetSpecializedTypeId() == typeId || GetGenericTypeId() == typeId || GetLegacySpecializedTypeId() == typeId; }

        /// Returns the legacy specialized type id which removed the pointer types from templates when calculating type ids.
        /// i.e Typeids for VStd::vector<V::Entity> and VStd::vector<V::Entity*> are the same for legacy ids
        virtual const Uuid& GetLegacySpecializedTypeId() const { return GetSpecializedTypeId(); }
    };

    template<class ValueType>
    struct SerializeGenericTypeInfo
    {
        // Provides a specific type alias that can be used to create GenericClassInfo of the
        // specified type. By default this is GenericClassInfo class which is abstract
        using ClassInfoType = GenericClassInfo;

        /// By default we don't have generic class info
        static GenericClassInfo* GetGenericInfo() { return nullptr; }
        /// By default just return the ValueTypeInfo
        static const Uuid& GetClassTypeId();
    };

    /**
    Helper structure to allow the creation of a function pointer for creating VStd::any objects
    It takes advantage of type erasure to allow a mapping of Uuids to VStd::any(*)() function pointers
    without needing to store the template type.
    */
    template<typename ValueType, typename = void>
    struct AnyTypeInfoConcept;

    template<typename ValueType>
    struct AnyTypeInfoConcept<ValueType, VStd::enable_if_t<!VStd::is_abstract<ValueType>::value && VStd::Internal::template_is_copy_constructible<ValueType>::value>>
    {
        static VStd::any CreateAny(SerializeContext*)
        {
            return VStd::any(ValueType());
        }
    };

    template<typename ValueType>
    struct AnyTypeInfoConcept<ValueType, VStd::enable_if_t<!VStd::is_abstract<ValueType>::value && !VStd::Internal::template_is_copy_constructible<ValueType>::value>>
    {
        template<typename T>
        static void* Allocate()
        {
            if constexpr (HasVClassAllocator<T>::value)
            {
                // If the class specializes an AZ_CLASS_ALLOCATOR use that for allocating the memory
                return T::V_CLASS_ALLOCATOR_Allocate();
            }
            else
            {
                // Otherwise use the V::SystemAllocator
                return vmalloc(sizeof(T), alignof(T), V::SystemAllocator, "");
            }
        }
        template<typename T>
        static void DeAllocate(void* ptr)
        {
            if constexpr (HasVClassAllocator<T>::value)
            {
                // If the class specializes an AZ_CLASS_ALLOCATOR use the class DeAllocator
                // static function for returning the memory back to the allocator
                return T::V_CLASS_ALLOCATOR_DeAllocate(ptr);
            }
            else
            {
                // Otherwise free the memory using the V::SystemAllocator
                vfree(ptr);
            }
        }
        // The SerializeContext CloneObject function is used to copy data between Any
        static VStd::any::type_info::HandleFnT NonCopyableAnyHandler(SerializeContext* serializeContext)
        {
            return [serializeContext](VStd::any::Action action, VStd::any* dest, const VStd::any* source)
            {
                V_Assert(serializeContext->FindClassData(dest->type()), "Type %s stored in any must be registered with the serialize context", V::VObject<ValueType>::Name());

                switch (action)
                {
                case VStd::any::Action::Reserve:
                {
                    if (dest->get_type_info().m_useHeap)
                    {
                        // Allocate space for object on heap
                        // This takes advantage of the fact that the pointer for an any is stored at offset 0
                        *reinterpret_cast<void**>(dest) = Allocate<ValueType>();
                    }

                    break;
                }
                case VStd::any::Action::Construct:
                {
                    // Default construct the ValueType object
                    // This occurs in the case where a Copy and Move action is invoked
                    void* ptr = VStd::any_cast<void>(dest);
                    if (ptr)
                    {
                        new (ptr) ValueType();
                    }
                    break;
                }
                case VStd::any::Action::Copy:
                case VStd::any::Action::Move:
                {
                    // CloneObjectInplace should act as copy assignment over an already constructed object
                    // and not a copy constructor over an uninitialized object
                    serializeContext->CloneObjectInplace(VStd::any_cast<void>(dest), VStd::any_cast<void>(source), source->type());
                    break;
                }

                case VStd::any::Action::Destruct:
                {
                    // Call the destructor
                    void* ptr = VStd::any_cast<void>(dest);
                    if (ptr)
                    {
                        reinterpret_cast<ValueType*>(ptr)->~ValueType();
                    }
                    break;
                }
                case VStd::any::Action::Destroy:
                {
                    // Clear memory
                    if (dest->get_type_info().m_useHeap)
                    {
                        DeAllocate<ValueType>(VStd::any_cast<void>(dest));
                    }
                    break;
                }
                }
            };
        }

        static VStd::any CreateAny(SerializeContext* serializeContext)
        {
            VStd::any::type_info typeinfo;
            typeinfo.m_id = vobject_rtti_typeid<ValueType>();
            typeinfo.m_handler = NonCopyableAnyHandler(serializeContext);
            typeinfo.m_isPointer = VStd::is_pointer<ValueType>::value;
            typeinfo.m_useHeap = VStd::GetMax(sizeof(ValueType), VStd::alignment_of<ValueType>::value) > VStd::Internal::ANY_SBO_BUF_SIZE;
            if constexpr (VStd::is_default_constructible_v<ValueType>)
            {
                return serializeContext ? VStd::any(typeinfo, VStd::in_place_type_t<ValueType>{}) : VStd::any();
            }
            else
            {
                ValueType instance;
                return serializeContext ? VStd::any(reinterpret_cast<const void*>(&instance), typeinfo) : VStd::any();
            }
        }
    };

    template<typename ValueType>
    struct AnyTypeInfoConcept<ValueType, VStd::enable_if_t<VStd::is_abstract<ValueType>::value>>
    {
        static VStd::any CreateAny(SerializeContext*)
        {
            return {};
        }
    };

    /*
     * Helper class to retrieve Uuids and perform AZRtti queries.
     * This helper uses AZRtti when available, and does what it can when not.
     * It automatically resolves pointer-to-pointers to their value types.
     * When type is available, use the static functions provided by this helper
     * directly (ex: SerializeTypeInfo<T>::GetUuid()).
     * Call GetRttiHelper<T>() to retrieve an IRttiHelper interface
     * for AZRtti-enabled types while type info is still available, so it
     * can be used for queries after type info is lost.
     */
    template<typename T>
    struct SerializeTypeInfo
    {
        typedef typename VStd::remove_pointer<T>::type ValueType;
        static const Uuid& GetUuid(const T* instance = nullptr)
        {
            return GetUuid(instance, typename VStd::is_pointer<T>::type());
        }
        static const Uuid& GetUuid(const T* instance, const VStd::true_type& /*is_pointer<T>*/)
        {
            return GetUuidHelper(instance ? *instance : nullptr, typename HasVObjectRtti<ValueType>::type());
        }
        static const Uuid& GetUuid(const T* instance, const VStd::false_type& /*is_pointer<T>*/)
        {
            return GetUuidHelper(instance, typename HasVObjectRtti<ValueType>::type());
        }
        static const char* GetRttiTypeName(ValueType* const* instance)
        {
            return GetRttiTypeName(instance ? *instance : nullptr, typename HasVObjectRtti<ValueType>::type());
        }
        static const char* GetRttiTypeName(const ValueType* instance)
        {
            return GetRttiTypeName(instance, typename HasVObjectRtti<ValueType>::type());
        }
        static const char* GetRttiTypeName(const ValueType* instance, const VStd::true_type& /*HasVObjectRtti<ValueType>*/)
        {
            return instance ? GetRttiHelper<ValueType>()->GetActualTypeName(instance) : VObject<ValueType>::Name();
        }
        static const char* GetRttiTypeName(const ValueType* /*instance*/, const VStd::false_type& /*!HasVObjectRtti<ValueType>*/)
        {
            return "NotVObjectRttiType";
        }

        static const Uuid& GetRttiTypeId(ValueType* const* instance)
        {
            return GetRttiTypeId(instance ? *instance : nullptr, typename HasVObjectRtti<ValueType>::type());
        }
        static const Uuid& GetRttiTypeId(const ValueType* instance) { return GetRttiTypeId(instance, typename HasVObjectRtti<ValueType>::type()); }
        static const Uuid& GetRttiTypeId(const ValueType* instance, const VStd::true_type& /*HasVObjectRtti<ValueType>*/)
        {
            return instance ? V::RttiTypeId(instance) : VObject<ValueType>::Id();
        }
        static const Uuid& GetRttiTypeId(const ValueType* /*instance*/, const VStd::false_type& /*!HasVObjectRtti<ValueType>*/)
        {
            static Uuid _nullUuid = Uuid::CreateNull();
            return _nullUuid;
        }

        static bool IsRttiTypeOf(const Uuid& id, const VStd::true_type& /*HasVObjectRtti<ValueType>*/)
        {
            return GetRttiHelper<ValueType>()->GetTypeId(id);
        }
        static bool IsRttiTypeOf(const Uuid& /*id*/, const VStd::false_type& /*!HasVObjectRtti<ValueType>*/)
        {
            return false;
        }

        static void RttiEnumHierarchy(RTTI_EnumCallback callback, void* userData, const VStd::true_type& /*HasVObjectRtti<ValueType>*/)
        {
            return V::RttiEnumHierarchy<ValueType>(callback, userData);
        }
        static void RttiEnumHierarchy(RTTI_EnumCallback /*callback*/, void* /*userData*/, const VStd::false_type& /*!HasVObjectRtti<ValueType>*/)
        {
        }

        // const pointers
        static const void* RttiCast(ValueType* const* instance, const Uuid& asType)
        {
            return RttiCast(instance ? *instance : nullptr, asType, typename HasVObjectRtti<ValueType>::type());
        }
        static const void* RttiCast(const ValueType* instance, const Uuid& asType)
        {
            return RttiCast(instance, asType, typename HasVObjectRtti<ValueType>::type());
        }
        static const void* RttiCast(const ValueType* instance, const Uuid& asType, const VStd::true_type& /*HasVObjectRtti<ValueType>*/)
        {
            return V::RttiAddressOf(instance, asType);
        }
        static const void* RttiCast(const ValueType* instance, const Uuid& /*asType*/, const VStd::false_type& /*!HasVObjectRtti<ValueType>*/)
        {
            return instance;
        }
        // non cost pointers
        static void* RttiCast(ValueType** instance, const Uuid& asType)
        {
            return RttiCast(instance ? *instance : nullptr, asType, typename HasVObjectRtti<ValueType>::type());
        }
        static void* RttiCast(ValueType* instance, const Uuid& asType)
        {
            return RttiCast(instance, asType, typename HasVObjectRtti<ValueType>::type());
        }
        static void* RttiCast(ValueType* instance, const Uuid& asType, const VStd::true_type& /*HasVObjectRtti<ValueType>*/)
        {
            return V::RttiAddressOf(instance, asType);
        }
        static void* RttiCast(ValueType* instance, const Uuid& /*asType*/, const VStd::false_type& /*!HasVObjectRtti<ValueType>*/)
        {
            return instance;
        }

    private:
        static const V::Uuid& GetUuidHelper(const ValueType* /* ptr */, const VStd::false_type& /* !HasVObjectRtti<U>::type() */)
        {
            return SerializeGenericTypeInfo<ValueType>::GetClassTypeId();
        }
        static const V::Uuid& GetUuidHelper(const ValueType* ptr, const VStd::true_type& /* HasVObjectRtti<U>::type() */)
        {
            return ptr ? V::RttiTypeId(ptr) : SerializeGenericTypeInfo<ValueType>::GetClassTypeId();
        }
    };

     ///////////////////////////////////////////////////////////////////////////
    // Implementations
    //
    namespace Serialize
    {
        /// Default instance factory to create/destroy a classes (when a factory is not provided)
        template<class T, bool U = HasVClassAllocator<T>::value, bool A = VStd::is_abstract<T>::value >
        struct InstanceFactory
            : public SerializeContext::IObjectFactory
        {
            void* Create(const char* name) override
            {
                (void)name;
                return vnewex(name) T;
            }
            void Destroy(void* ptr) override
            {
                delete reinterpret_cast<T*>(ptr);
            }
        };

        /// Default instance for classes without AZ_CLASS_ALLOCATOR (can't use aznew) defined.
        template<class T>
        struct InstanceFactory<T, false, false>
            : public SerializeContext::IObjectFactory
        {
            void* Create(const char* name) override
            {
                return new(vmalloc(sizeof(T), VStd::alignment_of<T>::value,V::SystemAllocator, name))T;
            }
            void Destroy(void* ptr) override
            {
                reinterpret_cast<T*>(ptr)->~T();
                vfree(ptr,V::SystemAllocator, sizeof(T), VStd::alignment_of<T>::value);
            }
        };

        /// Default instance for abstract classes. We can't instantiate abstract classes, but we have this function for assert!
        template<class T, bool U>
        struct InstanceFactory<T, U, true>
            : public SerializeContext::IObjectFactory
        {
            void* Create(const char* name) override
            {
                (void)name;
                V_Assert(false, "Can't instantiate abstract classs %s", name);
                return nullptr;
            }
            void Destroy(void* ptr) override
            {
                delete reinterpret_cast<T*>(ptr);
            }
        };
    }

    namespace SerializeInternal
    {
        template<class T>
        struct ElementInfo;

        template<class T, class C>
        struct ElementInfo<T C::*>
        {
            typedef typename VStd::RemoveEnum<T>::type ElementType;
            typedef C ClassType;
            typedef T Type;
            typedef typename VStd::remove_pointer<ElementType>::type ValueType;
        };

        template<class Derived, class Base>
        size_t GetBaseOffset()
        {
            Derived* der = reinterpret_cast<Derived*>(V_INVALID_POINTER);
            return reinterpret_cast<char*>(static_cast<Base*>(der)) - reinterpret_cast<char*>(der);
        }
    } // namespace SerializeInternal

    //=========================================================================
    // Cast
    //=========================================================================
    template<class T>
    T SerializeContext::Cast(void* instance, const Uuid& instanceClassId) const
    {
        void* ptr = DownCast(instance, instanceClassId, SerializeTypeInfo<T>::GetUuid());
        return reinterpret_cast<T>(ptr);
    }

    //=========================================================================
    // EnumerateObject
    //=========================================================================
    template<class T>
    bool SerializeContext::EnumerateObject(T* obj, const BeginElemEnumCB& beginElemCB, const EndElemEnumCB& endElemCB, unsigned int accessFlags, ErrorHandler* errorHandler) const
    {
        V_Assert(!VStd::is_pointer<T>::value, "Cannot enumerate pointer-to-pointer as root element! This makes no sense!");

        void* classPtr = SerializeTypeInfo<T>::RttiCast(obj, SerializeTypeInfo<T>::GetRttiTypeId(obj));
        const Uuid& classId = SerializeTypeInfo<T>::GetUuid(obj);
        return EnumerateInstance(classPtr, classId, beginElemCB, endElemCB, accessFlags, nullptr, nullptr, errorHandler);
    }

    template<class T>
    bool SerializeContext::EnumerateObject(const T* obj, const BeginElemEnumCB& beginElemCB, const EndElemEnumCB& endElemCB, unsigned int accessFlags, ErrorHandler* errorHandler) const
    {
        V_Assert(!VStd::is_pointer<T>::value, "Cannot enumerate pointer-to-pointer as root element! This makes no sense!");

        const void* classPtr = SerializeTypeInfo<T>::RttiCast(obj, SerializeTypeInfo<T>::GetRttiTypeId(obj));
        const Uuid& classId = SerializeTypeInfo<T>::GetUuid(obj);
        return EnumerateInstanceConst(classPtr, classId, beginElemCB, endElemCB, accessFlags, nullptr, nullptr, errorHandler);
    }

    //=========================================================================
    // CloneObject
    //=========================================================================
    template<class T>
    T* SerializeContext::CloneObject(const T* obj)
    {
        // This function could have been called with a base type as the template parameter, when the object to be cloned is a derived type.
        // In this case, first cast to the derived type, since the pointer to the derived type might be offset from the base type due to multiple inheritance
        const void* classPtr = SerializeTypeInfo<T>::RttiCast(obj, SerializeTypeInfo<T>::GetRttiTypeId(obj));
        const Uuid& classId = SerializeTypeInfo<T>::GetUuid(obj);
        void* clonedObj = CloneObject(classPtr, classId);

        // Now that the actual type has been cloned, cast back to the requested type of the template parameter.
        return Cast<T*>(clonedObj, classId);
    }

    // CloneObject
    template<class T>
    void SerializeContext::CloneObjectInplace(T& dest, const T* obj)
    {
        const void* classPtr = SerializeTypeInfo<T>::RttiCast(obj, SerializeTypeInfo<T>::GetRttiTypeId(obj));
        const Uuid& classId = SerializeTypeInfo<T>::GetUuid(obj);
        CloneObjectInplace(&dest, classPtr, classId);
    }

    //=========================================================================
    // EnumerateDerived
    //=========================================================================
    template<class T>
    inline void SerializeContext::EnumerateDerived(const TypeInfoCB& callback)
    {
        EnumerateDerived(callback, VObject<T>::Id(), VObject<T>::Id());
    }

    //=========================================================================
    // EnumerateBase
    //=========================================================================
    template<class T>
    inline void SerializeContext::EnumerateBase(const TypeInfoCB& callback)
    {
        EnumerateBase(callback, VObject<T>::Id());
    }

    constexpr char const* c_serializeBaseClassStrings[] = { "BaseClass1", "BaseClass2", "BaseClass3" };
    constexpr size_t c_serializeMaxNumBaseClasses = 3;
    static_assert(V_ARRAY_SIZE(c_serializeBaseClassStrings) == c_serializeMaxNumBaseClasses, "Expected matching array size for the names of serialized base classes.");

    template<class T, class BaseClass>
    void SerializeContext::AddClassData(ClassData* classData, size_t index)
    {
        ClassElement ed;
        ed.Name = c_serializeBaseClassStrings[index];
        ed.NameCrc = V_CRC(c_serializeBaseClassStrings[index]);
        ed.Flags = ClassElement::FLG_BASE_CLASS;
        ed.DataSize = sizeof(BaseClass);
        ed.TypeId = vobject_rtti_typeid<BaseClass>();
        ed.Offset = SerializeInternal::GetBaseOffset<T, BaseClass>();
        ed.GenericClassInfoPtr = SerializeGenericTypeInfo<BaseClass>::GetGenericInfo();
        ed.VObjectRtti = GetRttiHelper<BaseClass>();
        ed.EditDataPtr = nullptr;
        classData->Elements.emplace_back(VStd::move(ed));
    }

    template<class T, class... TBaseClasses>
    void SerializeContext::AddClassData(ClassData* classData)
    {
        size_t index = 0;
        // Note - if there are no base classes for T, the compiler will eat the function call in the initializer list and emit warnings for unrefed locals
        V_UNUSED(classData);
        V_UNUSED(index);
        std::initializer_list<size_t> stuff{ (AddClassData<T, TBaseClasses>(classData, index), index++)... };
        V_UNUSED(stuff);
    }

    //=========================================================================
    // Class
    //=========================================================================
    template<class T, class... TBaseClasses>
    SerializeContext::ClassBuilder
    SerializeContext::Class()
    {
        return Class<T, TBaseClasses...>(&Serialize::StaticInstance<Serialize::InstanceFactory<T> >::s_instance);
    }

    //=========================================================================
    // Class
    //=========================================================================
    template<class T, class... TBaseClasses>
    SerializeContext::ClassBuilder
    SerializeContext::Class(IObjectFactory* factory)
    {
        static_assert((VStd::negation_v< VStd::disjunction<VStd::is_same<T, TBaseClasses>...> >), "You cannot reflect a type as its own base");
        static_assert(sizeof...(TBaseClasses) <= c_serializeMaxNumBaseClasses, "Only " V_STRINGIZE(c_serializeMaxNumBaseClasses) " base classes are supported. You can add more in c_serializeBaseClassStrings.");

        const Uuid& typeUuid = VObject<T>::Id();
        const char* name = VObject<T>::Name();

        if (IsRemovingReflection())
        {
            auto mapIt = m_uuidMap.find(typeUuid);
            if (mapIt != m_uuidMap.end())
            {
                RemoveClassData(&mapIt->second);

                auto classNameRange = m_classNameToUuid.equal_range(Crc32(name));
                for (auto classNameRangeIter = classNameRange.first; classNameRangeIter != classNameRange.second;)
                {
                    if (classNameRangeIter->second == typeUuid)
                    {
                        classNameRangeIter = m_classNameToUuid.erase(classNameRangeIter);
                    }
                    else
                    {
                        ++classNameRangeIter;
                    }
                }
                m_uuidAnyCreationMap.erase(typeUuid);
                m_uuidMap.erase(mapIt);
            }
            return ClassBuilder(this, m_uuidMap.end());
        }
        else
        {
            m_classNameToUuid.emplace(V::Crc32(name), typeUuid);
            IdToClassMap::pair_iter_bool result = m_uuidMap.insert(VStd::make_pair(typeUuid, ClassData::Create<T>(name, typeUuid, factory)));
            V_Assert(result.second, "This class type %s could not be registered with duplicated Uuid: %s.", name, typeUuid.ToString<VStd::string>().c_str());
            m_uuidAnyCreationMap.emplace(SerializeTypeInfo<T>::GetUuid(), &AnyTypeInfoConcept<T>::CreateAny);

            AddClassData<T, TBaseClasses...>(&result.first->second);

            return ClassBuilder(this, result.first);
        }
    }

    //=========================================================================
    // ClassBuilder::Field
    //=========================================================================
    template<class ClassType, class FieldType>
    SerializeContext::ClassBuilder* SerializeContext::ClassBuilder::Field(const char* name, FieldType ClassType::* member, VStd::initializer_list<AttributePair> attributes)
    {
        using UnderlyingType = VStd::RemoveEnumT<FieldType>;
        using ValueType = VStd::remove_pointer_t<FieldType>;

        if (m_context->IsRemovingReflection())
        {
            // Delete any attributes allocated for this call.
            for (auto& attributePair : attributes)
            {
                delete attributePair.second;
            }
            return this; // we have already removed the class data for this class
        }

        V_Assert(!m_classData->second.SerializerPtr,
            "Class %s has a custom serializer, and can not have additional fields. Classes can either have a custom serializer or child fields.",
            name);

        V_Assert(m_classData->second.TypeId == VObject<ClassType>::Id(),
            "Field %s is serialized with class %s, but belongs to class %s. If you are trying to expose base class field use FieldFromBase",
            name,
            m_classData->second.Name,
            VObject<ClassType>::Name());

        // SerializeGenericTypeInfo<ValueType>::GetClassTypeId() is needed solely because
        // the SerializeGenericTypeInfo specialization forV::Data::Asset<T> returns the GetAssetClassId() value
        // and not the VObject<V::Data::Asset<T>>::Id()
        // Therefore in order to remain backwards compatible the SerializeGenericTypeInfo<ValueType>::GetClassTypeId specialization
        // is used for all cases except when the ValueType is an enum type.
        // In that case VObject is used directly to retrieve the actual Id that the enum specializes
        constV::TypeId& fieldTypeId = VStd::is_enum<ValueType>::value ? VObject<ValueType>::Id() : SerializeGenericTypeInfo<ValueType>::GetClassTypeId();
        constV::TypeId& underlyingTypeId = VObject<UnderlyingType>::Id();

        m_classData->second.Elements.emplace_back();
        ClassElement& ed = m_classData->second.Elements.back();
        ed.Name = name;
        ed.NameCrc = V::Crc32(name);
        // Not really portable but works for the supported compilers. It will crash and not work if we have virtual inheritance. Detect and assert at compile time about it. (something like is_virtual_base_of)
        ed.Offset = reinterpret_cast<size_t>(&(reinterpret_cast<ClassType const volatile*>(0)->*member));
        //ed.m_offset = or pass it to the function with offsetof(typename ElementTypeInfo::ClassType,member);
        ed.DataSize = sizeof(FieldType);
        ed.Flags = VStd::is_pointer<FieldType>::value ? ClassElement::FLG_POINTER : 0;
        ed.EditDataPtr = nullptr;
        ed.VObjectRtti = GetRttiHelper<ValueType>();

        ed.m_genericClassInfo = SerializeGenericTypeInfo<ValueType>::GetGenericInfo();
        if (!fieldTypeId.IsNull())
        {
            ed.TypeId = fieldTypeId;
            // If the field is an enum type add it to the map of enum types -> underlying types
            if (VStd::is_enum<FieldType>::value)
            {
                m_context->m_enumTypeIdToUnderlyingTypeIdMap.emplace(fieldTypeId, underlyingTypeId);
                m_context->m_uuidAnyCreationMap.emplace(fieldTypeId, &AnyTypeInfoConcept<FieldType>::CreateAny);
            }
        }
        else
        {
            // If the Field typeid is null, fallback to using the Underlying typeid  in case the reflected field is an enum
            // This allows reflected enum fields which doen't specialize VObject using the VOBJECT_SPECIALIZE macro to still
            // serialize out using  the underlying type for backwards compatibility
            ed.TypeId = underlyingTypeId;
        }
        V_Assert(!ed.TypeId.IsNull(), "You must provide a valid class id for class %s", name);
        for (const AttributePair& attributePair : attributes)
        {
            ed.Attributes.emplace_back(attributePair.first, attributePair.second);
        }

        if (ed.m_genericClassInfo)
        {
            ed.GenericClassInfoPtr->Reflect(m_context);
        }

        m_currentAttributes = &ed.Attributes;

        // Flag the field with the EnumType attribute if we're an enumeration type aliased by RemoveEnum
        // We use Attribute here, so we have to do this after m_currentAttributes is assigned
        const bool isSpecializedEnum = VStd::is_enum<FieldType>::value && !VObject<FieldType>::Id().IsNull();
        if (isSpecializedEnum)
        {
            Attribute(V_CRC("EnumType", 0xb177e1b5), VObject<FieldType>::Id());
        }

        return this;
    }

    /// Declare a type change between serialized versions of a field
    //=========================================================================
    // ClassBuilder::TypeChange
    //=========================================================================
    template <class FromT, class ToT>
    SerializeContext::ClassBuilder* SerializeContext::ClassBuilder::TypeChange(VStd::string_view fieldName, unsigned int fromVersion, unsigned int toVersion, VStd::function<ToT(const FromT&)> upgradeFunc)
    {
        if (m_context->IsRemovingReflection())
        {
            return this; // we have already removed the class data for this class
        }

        V_Error("Serialize", !m_classData->second.SerializerPtr, "Class has a custom serializer, and can not have per-node version upgrades.");
        V_Error("Serialize", !m_classData->second.Elements.empty(), "Class has no defined elements to add per-node version upgrades to.");

        if (m_classData->second.SerializerPtr || m_classData->second.Elements.empty())
        {
            return this;
        }

        m_classData->second.DataPatchUpgrader.AddFieldUpgrade(vnew DataPatchTypeUpgrade<FromT, ToT>(fieldName, fromVersion, toVersion, upgradeFunc));

        return this;
    }

    //=========================================================================
    // ClassBuilder::FieldFromBase
    //=========================================================================
    template<class ClassType, class BaseType, class FieldType>
    SerializeContext::ClassBuilder* SerializeContext::ClassBuilder::FieldFromBase(const char* name, FieldType BaseType::* member)
    {
        static_assert((VStd::is_base_of<BaseType, ClassType>::value), "Classes BaseType and ClassType are unrelated. BaseType should be base class for ClassType");

        if (m_context->IsRemovingReflection())
        {
            return this; // we have already removed the class data for this class
        }

        V_Assert(!m_classData->second.SerializerPtr,
            "Class %s has a custom serializer, and can not have additional fields. Classes can either have a custom serializer or child fields.",
            name);

        // make sure the class have not been reflected.
        for (ClassElement& element : m_classData->second.Elements)
        {
            if (element.Flags & ClassElement::FLG_BASE_CLASS)
            {
                V_Assert(element.TypeId != VObject<BaseType>::Id(),
                    "You can not reflect %s as base class of %s with Class<%s,%s> and then reflect the some of it's fields with FieldFromBase! Either use FieldFromBase or just reflect the entire base class!",
                    VObject<BaseType>::Name(), VObject<ClassType>::Name(), VObject<ClassType>::Name(), VObject<BaseType>::Name()
                    );
            }
            else
            {
                break; // base classes are sorted at the start
            }
        }

        return Field<ClassType>(name, static_cast<FieldType ClassType::*>(member));
    }

    //=========================================================================
    // ClassBuilder::Attribute
    //=========================================================================
    template <class T>
    SerializeContext::ClassBuilder* SerializeContext::ClassBuilder::Attribute(Crc32 idCrc, T&& value)
    {
        static_assert(!std::is_lvalue_reference<T>::value || std::is_copy_constructible<T>::value, "If passing an lvalue-reference to Attribute, it must be copy constructible");
        static_assert(!std::is_rvalue_reference<T>::value || std::is_move_constructible<T>::value, "If passing an rvalue-reference to Attribute, it must be move constructible");

        if (m_context->IsRemovingReflection())
        {
            return this; // we have already removed the class data for this class
        }

        using ContainerType = AttributeContainerType<VStd::decay_t<T>>;

        V_Assert(m_currentAttributes, "Current data type doesn't support attributes!");
        if (m_currentAttributes)
        {
            m_currentAttributes->emplace_back(idCrc, vnew ContainerType(VStd::forward<T>(value)));
        }
        return this;
    }

    //=========================================================================
    // DataElementNode::GetData
    template <typename T>
    bool SerializeContext::DataElementNode::GetData(T& value, ErrorHandler* errorHandler)
    {
        const Uuid& classTypeId = SerializeGenericTypeInfo<T>::GetClassTypeId();
        const Uuid& underlyingTypeId = SerializeGenericTypeInfo<VStd::RemoveEnumT<T>>::GetClassTypeId();
        GenericClassInfo* genericInfo = SerializeGenericTypeInfo<T>::GetGenericInfo();
        const bool genericClassStoresType = genericInfo && genericInfo->CanStoreType(m_element.Id);
        const bool typeIdsMatch = classTypeId == m_element.Id || underlyingTypeId == m_element.Id;
        if (typeIdsMatch || genericClassStoresType)
        {
            const ClassData* classData = m_classData;

            // check generic types
            if (!classData)
            {
                if (genericInfo)
                {
                    classData = genericInfo->GetClassData();
                }
            }

            if (classData && classData->SerializerPtr)
            {
                if (m_element.DataSize == 0)
                {
                    return true;
                }
                else
                {
                    if (m_element.DataCategory == DataElement::DT_TEXT)
                    {
                        // convert to binary so we can load the data
                        VStd::string text;
                        text.resize_no_construct(m_element.DataSize);
                        m_element.ByteStream.Read(text.size(), reinterpret_cast<void*>(text.data()));
                        m_element.ByteStream.Seek(0, IO::GenericStream::ST_SEEK_BEGIN);
                        m_element.DataSize = classData->m_serializer->TextToData(text.c_str(), m_element.Version, m_element.ByteStream);
                        m_element.ByteStream.Seek(0, IO::GenericStream::ST_SEEK_BEGIN);
                        m_element.DataCategory = DataElement::DT_BINARY;
                    }

                    bool isLoaded = classData->SerializerPtr->Load(&value, m_element.ByteStream, m_element.Version, m_element.DataType == DataElement::DT_BINARY_BE);
                    m_element.ByteStream.Seek(0, IO::GenericStream::ST_SEEK_BEGIN); // reset stream position
                    return isLoaded;
                }
            }
            else
            {
                return GetDataHierarchy(&value, classTypeId, errorHandler);
            }
        }

        if (errorHandler)
        {
            const VStd::string errorMessage =
                VStd::string::format("Specified class type {%s} does not match current element %s with type {%s}.",
                    classTypeId.ToString<VStd::string>().c_str(), m_element.Name ? m_element.Name : "", m_element.Id.ToString<VStd::string>().c_str());
            errorHandler->ReportError(errorMessage.c_str());
        }

        return false;
    }

    //=========================================================================
    // DataElementNode::GetDataHierarchy
    //=========================================================================
    template <typename T>
    bool SerializeContext::DataElementNode::GetDataHierarchy(SerializeContext&, T& value, ErrorHandler* errorHandler)
    {
        return GetData<T>(value, errorHandler);
    }

    //=========================================================================
    // DataElementNode::FindSubElementAndGetData
    //=========================================================================
    template <typename T>
    bool SerializeContext::DataElementNode::FindSubElementAndGetData(V::Crc32 crc, T& outValue)
    {
        if (V::SerializeContext::DataElementNode* subElementNode = FindSubElement(crc))
        {
            return subElementNode->GetData<T>(outValue);
        }

        return false;
    }

    //=========================================================================
    // DataElementNode::GetData
    //=========================================================================
    template<typename T>
    bool SerializeContext::DataElementNode::GetChildData(u32 childNameCrc, T& value)
    {
        int dataElementIndex = FindElement(childNameCrc);
        if (dataElementIndex != -1)
        {
            return GetSubElement(dataElementIndex).GetData(value);
        }
        return false;
    }

    //=========================================================================
    // DataElementNode::SetData
    //=========================================================================
    template<typename T>
    bool SerializeContext::DataElementNode::SetData(SerializeContext& sc, const T& value, ErrorHandler* errorHandler)
    {
        const Uuid& classTypeId = SerializeGenericTypeInfo<T>::GetClassTypeId();

        if (classTypeId == m_element.Id)
        {
            const ClassData* classData = m_classData;

            // check generic types
            if (!classData)
            {
                GenericClassInfo* genericInfo = SerializeGenericTypeInfo<T>::GetGenericInfo();
                if (genericInfo)
                {
                    classData = genericInfo->GetClassData();
                }
            }

            if (classData && classData->SerializerPtr && (classData->DoSave == nullptr || classData->DoSave(&value)))
            {
                V_Assert(m_element.ByteStream.GetCurPos() == 0, "We have to be in the beginning of the stream, as it should be for current element only!");
                m_element.DataSize = classData->SerializerPtr->Save(&value, m_element.ByteStream);
                m_element.ByteStream.Truncate();
                m_element.ByteStream.Seek(0, IO::GenericStream::ST_SEEK_BEGIN); // reset stream position
                m_element.StreamPtr = &m_element.ByteStream;

                m_element.DataCategory = DataElement::DT_BINARY;
                return true;
            }
            else
            {
                // clear the value and prepare for the new one, then set the new value
                return Convert<T>(sc) && SetDataHierarchy(sc, &value, classTypeId, errorHandler, classData);
            }
        }

        if (errorHandler)
        {
            const VStd::string errorMessage =
                VStd::string::format("Specified class type {%s} does not match current element %s with type {%s}.",
                    classTypeId.ToString<VStd::string>().c_str(), m_element.Name, m_element.Id.ToString<VStd::string>().c_str());
            errorHandler->ReportError(errorMessage.c_str());
        }

        return false;
    }

    //=========================================================================
    // DataElementNode::Convert
    //=========================================================================
    template<class T>
    bool SerializeContext::DataElementNode::Convert(SerializeContext& sc)
    {
        // remove sub elements
        while (!m_subElements.empty())
        {
            RemoveElement(static_cast<int>(m_subElements.size()) - 1);
        }

        // replace element
        m_element.Id = SerializeGenericTypeInfo<T>::GetClassTypeId();
        m_element.DataSize = 0;
        m_element.Buffer.clear();
        m_element.StreamPtr = &m_element.ByteStream;

        GenericClassInfo* genericClassInfo = SerializeGenericTypeInfo<T>::GetGenericInfo();
        if (genericClassInfo)
        {
            m_classData = genericClassInfo->GetClassData();
        }
        else
        {
            m_classData = sc.FindClassData(m_element.Id);
            V_Assert(m_classData, "You are adding element to an unregistered class!");
        }
        m_element.Version = m_classData->Version;

        return true;
    }

    //=========================================================================
    // DataElementNode::Convert
    //=========================================================================
    template<class T>
    bool SerializeContext::DataElementNode::Convert(SerializeContext& sc, const char* name)
    {
        V_Assert(name != NULL && strlen(name) > 0, "Empty name is an INVALID element name!");
        u32 nameCrc = Crc32(name);

#if defined(V_DEBUG_BUILD)
        if (FindElement(nameCrc) != -1)
        {
            V_Error("Serialize", false, "We already have a class member %s!", name);
            return false;
        }
#endif // V_DEBUG_BUILD

        // remove sub elements
        while (!m_subElements.empty())
        {
            RemoveElement(static_cast<int>(m_subElements.size()) - 1);
        }

        // replace element
        m_element.Name = name;
        m_element.NameCrc = nameCrc;
        m_element.Id = SerializeGenericTypeInfo<T>::GetClassTypeId();
        m_element.DataSize = 0;
        m_element.Buffer.clear();
        m_element.Stream = &m_element.ByteStream;

        GenericClassInfo* genericClassInfo = SerializeGenericTypeInfo<T>::GetGenericInfo();
        if (genericClassInfo)
        {
            m_classData = genericClassInfo->GetClassData();
        }
        else
        {
            m_classData = sc.FindClassData(m_element.Id);
            V_Assert(m_classData, "You are adding element to an unregistered class!");
        }
        m_element.Version = m_classData->Version;

        return true;
    }

    //=========================================================================
    // DataElementNode::AddElement
    // [10/31/2012]
    //=========================================================================
    template<typename T>
    int SerializeContext::DataElementNode::AddElement(SerializeContext& sc, const char* name)
    {
        V_Assert(name != nullptr && strlen(name) > 0, "Empty name in an INVALID element name!");
        u32 nameCrc = Crc32(name);

#if defined(V_DEBUG_BUILD)
        if (FindElement(nameCrc) != -1 && !m_classData->ContainerPtr)
        {
            V_Error("Serialize", false, "We already have a class member %s!", name);
            return -1;
        }
#endif // V_DEBUG_BUILD

        DataElementNode node;
        node.m_element.Name = name;
        node.m_element.NameCrc = nameCrc;
        node.m_element.Id = SerializeGenericTypeInfo<T>::GetClassTypeId();

        GenericClassInfo* genericClassInfo = SerializeGenericTypeInfo<T>::GetGenericInfo();
        if (genericClassInfo)
        {
            node.ClassDataPtr = genericClassInfo->GetClassData();
        }
        else
        {
            // if we are NOT a generic container
            node.m_classData = sc.FindClassData(node.m_element.m_id);
            V_Assert(node.m_classData, "You are adding element to an unregistered class!");
        }

        node.m_element.m_version = node.m_classData->m_version;

        m_subElements.push_back(node);
        return static_cast<int>(m_subElements.size() - 1);
    }

    //=========================================================================
    // DataElementNode::AddElement
    // [7/29/2016]
    //=========================================================================
    template<typename T>
    int SerializeContext::DataElementNode::AddElementWithData(SerializeContext& sc, const char* name, const T& dataToSet)
    {
        int retVal = AddElement<T>(sc, name);
        if (retVal != -1)
        {
            m_subElements[retVal].SetData<T>(sc, dataToSet);
        }
        return retVal;
    }

    //=========================================================================
    // DataElementNode::AddElement
    // [10/31/2012]
    //=========================================================================
    template<typename T>
    int SerializeContext::DataElementNode::ReplaceElement(SerializeContext& sc, int index, const char* name)
    {
        DataElementNode& node = m_subElements[index];

        if (node.Convert<T>(sc, name))
        {
            return index;
        }
        else
        {
            return -1;
        }
    }

    //=========================================================================
    // ClassData::ClassData
    //=========================================================================
    template<class T>
    SerializeContext::ClassData SerializeContext::ClassData::Create(const char* name, const Uuid& typeUuid, IObjectFactory* factory, IDataSerializer* serializer, IDataContainer* container)
    {
        ClassData cd;
        cd.Name = name;
        cd.TypeId = typeUuid;
        cd.Version = 0;
        cd.Converter = nullptr;
        // A raw ptr to an IDataSerializer isn't owned by the SerializeContext class data
        cd.SerializerPtr = IDataSerializerPtr(serializer, IDataSerializer::CreateNoDeleteDeleter());
        cd.Factory = factory;
        cd.PersistentId = nullptr;
        cd.DoSave = nullptr;
        cd.EventHandlerPtr = nullptr;
        cd.ContainerPtr = container;
        cd.VObjectRtti = GetRttiHelper<T>();
        cd.EditDataPtr = nullptr;
        return cd;
    }

    //=========================================================================
    // SerializeContext::RegisterGenericType<ValueType>
    //=========================================================================
    template <class ValueType>
    void SerializeContext::RegisterGenericType()
    {
        auto genericInfo = SerializeGenericTypeInfo<ValueType>::GetGenericInfo();
        if (genericInfo)
        {
            genericInfo->Reflect(this);
        }
    }

    //=========================================================================
    // SerializeGenericTypeInfo<ValueType>::GetClassTypeId
    //=========================================================================
    template<class ValueType>
    const Uuid& SerializeGenericTypeInfo<ValueType>::GetClassTypeId()
    {
        // Detect the scenario when an enum type doesn't specialize VObject
        // The underlying type Uuid is returned instead
        return VStd::is_enum<ValueType>::value && VObject<ValueType>::Uuid().IsNull() ? VObject<VStd::RemoveEnumT<ValueType>>::Uuid() : VObject<ValueType>::Uuid();
    };

    /**
    * PerModuleGenericClassInfo tracks module specific reflections of GenericClassInfo for each serializeContext
    * registered with this module(.dll)
    */
    class SerializeContext::PerModuleGenericClassInfo final
    {
    public:
        using GenericInfoModuleMap = VStd::unordered_map<V::Uuid,V::GenericClassInfo*, VStd::hash<V::Uuid>, VStd::equal_to<V::Uuid>,V::VStdIAllocator>;

        PerModuleGenericClassInfo();
        ~PerModuleGenericClassInfo();

       V::IAllocatorAllocate& GetAllocator();

        void AddGenericClassInfo(V::GenericClassInfo* genericClassInfo);
        void RemoveGenericClassInfo(const V::TypeId& canonicalTypeId);

        void RegisterSerializeContext(V::SerializeContext* serializeContext);
        void UnregisterSerializeContext(V::SerializeContext* serializeContext);

        /// Creates GenericClassInfo and registers it with the current module if it has not already been registered
        /// Returns a pointer to the GenericClassInfo derived class that was created
        template <typename T>
        typename SerializeGenericTypeInfo<T>::ClassInfoType* CreateGenericClassInfo();

        /// Returns GenericClassInfo registered with the current module.
        template <typename T>
       V::GenericClassInfo* FindGenericClassInfo() const;
       V::GenericClassInfo* FindGenericClassInfo(const V::TypeId& genericTypeId) const;
    private:
        void Cleanup();

       V::OSAllocator m_moduleOSAllocator;

        GenericInfoModuleMap m_moduleLocalGenericClassInfos;
        using SerializeContextSet = VStd::unordered_set<SerializeContext*, VStd::hash<SerializeContext*>, VStd::equal_to<SerializeContext*>,V::VStdIAllocator>;
        SerializeContextSet m_serializeContextSet;
    };

    template<typename T>
    typename SerializeGenericTypeInfo<T>::ClassInfoType* SerializeContext::PerModuleGenericClassInfo::CreateGenericClassInfo()
    {
        using GenericClassInfoType = typename SerializeGenericTypeInfo<T>::ClassInfoType;
        static_assert(VStd::is_base_of<V::GenericClassInfo, GenericClassInfoType>::value, "GenericClassInfoType must be be derived fromV::GenericClassInfo");

        constV::TypeId& canonicalTypeId = VObject<T>::Id();
        auto findIt = m_moduleLocalGenericClassInfos.find(canonicalTypeId);
        if (findIt != m_moduleLocalGenericClassInfos.end())
        {
            return static_cast<GenericClassInfoType*>(findIt->second);
        }

        void* rawMemory = m_moduleOSAllocator.Allocate(sizeof(GenericClassInfoType), alignof(GenericClassInfoType));
        new (rawMemory) GenericClassInfoType();
        auto genericClassInfo = static_cast<GenericClassInfoType*>(rawMemory);
        if (genericClassInfo)
        {
            AddGenericClassInfo(genericClassInfo);
        }
        return genericClassInfo;
    }

    template<typename T>
   V::GenericClassInfo* SerializeContext::PerModuleGenericClassInfo::FindGenericClassInfo() const
    {
        return FindGenericClassInfo(vobject_rtti_typeid<T>());
    }

    /// CreatesV::Attribute that is allocated using the the SerializeContext PerModule allocator
    /// associated with current module
    /// @param attrValue value to store within the attribute
    /// @param ContainerType second parameter which is used for function parameter deduction
    template <typename ContainerType, typename T>
    AttributePtr CreateModuleAttribute(T&& attrValue)
    {
        IAllocatorAllocate& moduleAllocator = GetCurrentSerializeContextModule().GetAllocator();
        void* rawMemory = moduleAllocator.Allocate(sizeof(ContainerType), alignof(ContainerType));
        new (rawMemory) ContainerType{ VStd::forward<T>(attrValue) };
        auto attributeDeleter = [](Attribute* attribute)
        {
            IAllocatorAllocate& moduleAllocator = GetCurrentSerializeContextModule().GetAllocator();
            attribute->~Attribute();
            moduleAllocator.DeAllocate(attribute);
        };

        return AttributePtr{ static_cast<ContainerType*>(rawMemory), VStd::move(attributeDeleter), AZStdIAllocator(&moduleAllocator) };
    }
} // namespace V

// Put this macro on your class to allow serialization with a private default constructor.
#define V_SERIALIZE_FRIEND() \
    template <typename, typename> \
    friend struct V::AnyTypeInfoConcept; \
    template <typename, bool, bool> \
    friend struct V::Serialize::InstanceFactory;

/// include VStd containers generics
#include <vcore/serialization/vstd_containers.inl>
//#include <vcore/serialization/std/variant_reflection.inl>

#endif // V_FRAMEWORK_CORE_SERIALIZATION_CONTEXT_H