#include <vcore/serialization/serialization_context.h>
#include <vcore/serialization/dynamic_serializable_field.h>

#include <vcore/std/containers/variant.h>
#include <vcore/std/functional.h>
#include <vcore/std/bind/bind.h>
#include <vcore/std/smart_ptr/unique_ptr.h>
#include <vcore/std/containers/stack.h>

#include <vcore/math/math_reflection.h>


namespace V
{
     V_THREAD_LOCAL void* Internal::VStdArrayEvents::m_indices = nullptr;
     //////////////////////////////////////////////////////////////////////////
    // Integral types serializers

    template<class T>
    class BinaryValueSerializer
        : public SerializeContext::IDataSerializer
    {
        /// Load the class data from a binary buffer.
        bool    Load(void* classPtr, IO::GenericStream& stream, unsigned int /*version*/, bool isDataBigEndian = false) override
        {
            if (stream.GetLength() < sizeof(T))
            {
                return false;
            }

            T* dataPtr = reinterpret_cast<T*>(classPtr);
            if (stream.Read(sizeof(T), reinterpret_cast<void*>(dataPtr)) == sizeof(T))
            {
                V_SERIALIZE_SWAP_ENDIAN(*dataPtr, isDataBigEndian);
                return true;
            }
            return false;
        }

        /// Store the class data into a stream.
        size_t Save(const void* classPtr, IO::GenericStream& stream, bool isDataBigEndian /*= false*/) override
        {
            T value = *reinterpret_cast<const T*>(classPtr);
            V_SERIALIZE_SWAP_ENDIAN(value, isDataBigEndian);
            return static_cast<size_t>(stream.Write(sizeof(T), reinterpret_cast<const void*>(&value)));
        }
    };


    // char, short, int
    template<class T>
    class IntSerializer
        : public BinaryValueSerializer<T>
    {
        /// Convert binary data to text
        size_t  DataToText(IO::GenericStream& in, IO::GenericStream& out, bool isDataBigEndian /*= false*/) override
        {
            V_Assert(in.GetLength() == sizeof(T), "Invalid data size");

            T value;
            in.Read(sizeof(T), reinterpret_cast<void*>(&value));
            V_SERIALIZE_SWAP_ENDIAN(value, isDataBigEndian);
            VStd::string outText = VStd::string::format("%d", value);
            return static_cast<size_t>(out.Write(outText.size(), outText.c_str()));
        }

        /// Convert text data to binary, to support loading old version formats. We must respect text version if the text->binary format has changed!
        size_t  TextToData(const char* text, unsigned int textVersion, IO::GenericStream& stream, bool isDataBigEndian = false) override
        {
            V_Assert(textVersion == 0, "Unknown char, short, int version!");
            (void)textVersion;
            long value = strtol(text, nullptr, 10);
            V_SERIALIZE_SWAP_ENDIAN(value, isDataBigEndian);
            return static_cast<size_t>(stream.Write(sizeof(T), reinterpret_cast<void*>(&value)));
        }

        bool    CompareValueData(const void* lhs, const void* rhs) override
        {
            return SerializeContext::EqualityCompareHelper<T>::CompareValues(lhs, rhs);
        }
    };

    // unsigned char, short, int, long
    template<class T>
    class UIntSerializer
        : public BinaryValueSerializer<T>
    {
        size_t DataToText(IO::GenericStream& in, IO::GenericStream& out, bool isDataBigEndian /*= false*/) override
        {
            V_Assert(in.GetLength() == sizeof(T), "Invalid data size");

            T value;
            in.Read(sizeof(T), reinterpret_cast<void*>(&value));
            V_SERIALIZE_SWAP_ENDIAN(value, isDataBigEndian);
            VStd::string outText = VStd::string::format("%u", value);

            return static_cast<size_t>(out.Write(outText.size(), outText.data()));
        }

        /// Convert text data to binary, to support loading old version formats. We must respect text version if the text->binary format has changed!
        size_t  TextToData(const char* text, unsigned int textVersion, IO::GenericStream& stream, bool isDataBigEndian = false) override
        {
            V_Assert(textVersion == 0, "Unknown unsigned char, short, int version!");
            (void)textVersion;
            unsigned long value = strtoul(text, nullptr, 10);
            V_SERIALIZE_SWAP_ENDIAN(value, isDataBigEndian);
            return static_cast<size_t>(stream.Write(sizeof(T), reinterpret_cast<void*>(&value)));
        }

        bool    CompareValueData(const void* lhs, const void* rhs) override
        {
            return SerializeContext::EqualityCompareHelper<T>::CompareValues(lhs, rhs);
        }
    };

    // long
    template<class T>
    class LongSerializer
        : public BinaryValueSerializer<T>
    {
        /// Convert binary data to text
        size_t DataToText(IO::GenericStream& in, IO::GenericStream& out, bool isDataBigEndian /*= false*/) override
        {
            V_Assert(in.GetLength() == sizeof(T), "Invalid data size");

            T value;
            in.Read(sizeof(T), reinterpret_cast<void*>(&value));
            V_SERIALIZE_SWAP_ENDIAN(value, isDataBigEndian);
            VStd::string outText = VStd::string::format("%ld", value);

            return static_cast<size_t>(out.Write(outText.size(), outText.data()));
        }

        /// Convert text data to binary, to support loading old version formats. We must respect text version if the text->binary format has changed!
        size_t  TextToData(const char* text, unsigned int textVersion, IO::GenericStream& stream, bool isDataBigEndian = false) override
        {
            V_Assert(textVersion == 0, "Unknown unsigned char, short, int version!");
            (void)textVersion;
            long value = strtol(text, nullptr, 10);
            V_SERIALIZE_SWAP_ENDIAN(value, isDataBigEndian);
            return static_cast<size_t>(stream.Write(sizeof(T), reinterpret_cast<void*>(&value)));
        }

        bool    CompareValueData(const void* lhs, const void* rhs) override
        {
            return SerializeContext::EqualityCompareHelper<T>::CompareValues(lhs, rhs);
        }
    };

    // unsigned long
    template<class T>
    class ULongSerializer
        : public BinaryValueSerializer<T>
    {
        /// Convert binary data to text
        size_t DataToText(IO::GenericStream& in, IO::GenericStream& out, bool isDataBigEndian /*= false*/) override
        {
            V_Assert(in.GetLength() == sizeof(T), "Invalid data size");

            T value;
            in.Read(sizeof(T), reinterpret_cast<void*>(&value));
            V_SERIALIZE_SWAP_ENDIAN(value, isDataBigEndian);
            VStd::string outText = VStd::string::format("%lu", value);

            return static_cast<size_t>(out.Write(outText.size(), outText.data()));
        }

        /// Convert text data to binary, to support loading old version formats. We must respect text version if the text->binary format has changed!
        size_t  TextToData(const char* text, unsigned int textVersion, IO::GenericStream& stream, bool isDataBigEndian = false) override
        {
            V_Assert(textVersion == 0, "Unknown unsigned char, short, int version!");
            (void)textVersion;
            unsigned long value = strtoul(text, nullptr, 10);
            V_SERIALIZE_SWAP_ENDIAN(value, isDataBigEndian);
            return static_cast<size_t>(stream.Write(sizeof(T), reinterpret_cast<void*>(&value)));
        }

        bool    CompareValueData(const void* lhs, const void* rhs) override
        {
            return SerializeContext::EqualityCompareHelper<T>::CompareValues(lhs, rhs);
        }
    };

    // long long
    class LongLongSerializer
        : public BinaryValueSerializer<V::s64>
    {
        /// Convert binary data to text
        size_t DataToText(IO::GenericStream& in, IO::GenericStream& out, bool isDataBigEndian /*= false*/) override
        {
            V_Assert(in.GetLength() == sizeof(V::s64), "Invalid data size");

            V::s64 value;
            in.Read(sizeof(V::s64), reinterpret_cast<void*>(&value));
            V_SERIALIZE_SWAP_ENDIAN(value, isDataBigEndian);
            VStd::string outText = VStd::string::format("%lld", value);

            return static_cast<size_t>(out.Write(outText.size(), outText.data()));
        }

        /// Convert text data to binary, to support loading old version formats. We must respect text version if the text->binary format has changed!
        size_t  TextToData(const char* text, unsigned int textVersion, IO::GenericStream& stream, bool isDataBigEndian = false) override
        {
            V_Assert(textVersion == 0, "Unknown unsigned char, short, int version!");
            (void)textVersion;
            V::s64 value = strtoll(text, nullptr, 10);
            V_SERIALIZE_SWAP_ENDIAN(value, isDataBigEndian);
            return static_cast<size_t>(stream.Write(sizeof(V::s64), reinterpret_cast<void*>(&value)));
        }

        bool    CompareValueData(const void* lhs, const void* rhs) override
        {
            return SerializeContext::EqualityCompareHelper<V::s64>::CompareValues(lhs, rhs);
        }
    };

    // unsigned long long
    class ULongLongSerializer
        : public BinaryValueSerializer<V::u64>
    {
        /// Convert binary data to text
        size_t DataToText(IO::GenericStream& in, IO::GenericStream& out, bool isDataBigEndian /*= false*/) override
        {
            V_Assert(in.GetLength() == sizeof(V::u64), "Invalid data size");

            V::u64 value;
            in.Read(sizeof(V::u64), reinterpret_cast<void*>(&value));
            V_SERIALIZE_SWAP_ENDIAN(value, isDataBigEndian);
            VStd::string outText = VStd::string::format("%llu", value);

            return static_cast<size_t>(out.Write(outText.size(), outText.data()));
        }

        /// Convert text data to binary, to support loading old version formats. We must respect text version if the text->binary format has changed!
        size_t  TextToData(const char* text, unsigned int textVersion, IO::GenericStream& stream, bool isDataBigEndian = false) override
        {
            V_Assert(textVersion == 0, "Unknown unsigned char, short, int version!");
            (void)textVersion;
            unsigned long long value = strtoull(text, nullptr, 10);
            V_SERIALIZE_SWAP_ENDIAN(value, isDataBigEndian);
            return static_cast<size_t>(stream.Write(sizeof(V::u64), reinterpret_cast<void*>(&value)));
        }

        bool    CompareValueData(const void* lhs, const void* rhs) override
        {
            return SerializeContext::EqualityCompareHelper<V::u64>::CompareValues(lhs, rhs);
        }
    };

    // float, double
    template<class T, T GetEpsilon()>
    class FloatSerializer
        : public BinaryValueSerializer<T>
    {
        /// Convert binary data to text
        size_t DataToText(IO::GenericStream& in, IO::GenericStream& out, bool isDataBigEndian /*= false*/) override
        {
            V_Assert(in.GetLength() == sizeof(T), "Invalid data size");

            T value;
            in.Read(sizeof(T), reinterpret_cast<void*>(&value));
            V_SERIALIZE_SWAP_ENDIAN(value, isDataBigEndian);
            VStd::string outText = VStd::string::format("%.7f", value);

            return static_cast<size_t>(out.Write(outText.size(), outText.data()));
        }

        /// Convert text data to binary, to support loading old version formats. We must respect text version if the text->binary format has changed!
        size_t  TextToData(const char* text, unsigned int textVersion, IO::GenericStream& stream, bool isDataBigEndian = false) override
        {
            V_Assert(textVersion == 0, "Unknown float/double version!");
            (void)textVersion;
            double value = strtod(text, nullptr);
            V_SERIALIZE_SWAP_ENDIAN(value, isDataBigEndian);

            T data = static_cast<T>(value);
            size_t bytesWritten = static_cast<size_t>(stream.Write(sizeof(T), reinterpret_cast<void*>(&data)));

            V_Assert(bytesWritten == sizeof(T), "Invalid data size");

            return bytesWritten;
        }

        bool    CompareValueData(const void* lhs, const void* rhs) override
        {
            return V::IsClose(*reinterpret_cast<const T*>(lhs), *reinterpret_cast<const T*>(rhs), GetEpsilon());
        }
    };

    // bool
    class BoolSerializer
        : public BinaryValueSerializer<bool>
    {
        /// Convert binary data to text
        size_t DataToText(IO::GenericStream& in, IO::GenericStream& out, bool isDataBigEndian /*= false*/) override
        {
            V_Assert(in.GetLength() == sizeof(bool), "Invalid data size");

            bool value;
            in.Read(sizeof(bool), reinterpret_cast<void*>(&value));
            V_SERIALIZE_SWAP_ENDIAN(value, isDataBigEndian);

            char outStr[7];
            v_snprintf(outStr, 6, "%s", value ? "true" : "false");

            return static_cast<size_t>(out.Write(strlen(outStr), outStr));
        }

        /// Convert text data to binary, to support loading old version formats. We must respect text version if the text->binary format has changed!
        size_t  TextToData(const char* text, unsigned int textVersion, IO::GenericStream& stream, bool isDataBigEndian = false) override
        {
            (void)isDataBigEndian;
            (void)textVersion;
            bool value = strcmp("true", text) == 0;
            return static_cast<size_t>(stream.Write(sizeof(bool), reinterpret_cast<void*>(&value)));
        }

        bool    CompareValueData(const void* lhs, const void* rhs) override
        {
            return SerializeContext::EqualityCompareHelper<bool>::CompareValues(lhs, rhs);
        }
    };

    // serializer without any data write
    class EmptySerializer : public SerializeContext::IDataSerializer
    {
    public:
        /// Store the class data into a stream.
        size_t  Save(const void* classPtr, IO::GenericStream& stream, bool isDataBigEndian) override
        {
            (void)classPtr;
            (void)stream;
            (void)isDataBigEndian;
            return 0;
        }

        /// Load the class data from a stream.
        bool    Load(void* classPtr, IO::GenericStream& stream, unsigned int /*version*/, bool isDataBigEndian) override
        {
            (void)classPtr;
            (void)stream;
            (void)isDataBigEndian;
            return true;
        }

        /// Convert binary data to text.
        size_t  DataToText(IO::GenericStream& in, IO::GenericStream& out, bool isDataBigEndian) override
        {
            (void)in;
            (void)out;
            (void)isDataBigEndian;
            return 0;
        }

        /// Convert text data to binary, to support loading old version formats. We must respect text version if the text->binary format has changed!
        size_t  TextToData(const char* text, unsigned int textVersion, IO::GenericStream& stream, bool isDataBigEndian = false) override
        {
            (void)text;
            (void)textVersion;
            (void)stream;
            (void)isDataBigEndian;
            return 0;
        }

        bool    CompareValueData(const void* lhs, const void* rhs) override
        {
            (void)lhs;
            (void)rhs;
            return true;
        }
    };

    struct EnumerateBaseRTTIEnumCallbackData
    {
        VStd::fixed_vector<Uuid, 32> ReportedTypes; ///< Array with reported types (with more data - classData was found).
        const SerializeContext::TypeInfoCB* Callback; ///< Pointer to the callback.
    };

    //=========================================================================
    // SerializeContext
    //=========================================================================
    SerializeContext::SerializeContext(bool registerIntegralTypes, bool createEditContext)
        : m_editContext(nullptr)
    {
        if (registerIntegralTypes)
        {
            Class<char>()->
                Serializer<IntSerializer<char> >();
            Class<V::s8>()->
                Serializer<IntSerializer<V::s8>>();
            Class<short>()->
                Serializer<IntSerializer<short> >();
            Class<int>()->
                Serializer<IntSerializer<int> >();
            Class<long>()->
                Serializer<LongSerializer<long> >();
            Class<V::s64>()->
                Serializer<LongLongSerializer>();

            Class<unsigned char>()->
                Serializer<UIntSerializer<unsigned char> >();
            Class<unsigned short>()->
                Serializer<UIntSerializer<unsigned short> >();
            Class<unsigned int>()->
                Serializer<UIntSerializer<unsigned int> >();
            Class<unsigned long>()->
                Serializer<ULongSerializer<unsigned long> >();
            Class<V::u64>()->
                Serializer<ULongLongSerializer>();

            Class<float>()->
                Serializer<FloatSerializer<float, &std::numeric_limits<float>::epsilon>>();
            Class<double>()->
                Serializer<FloatSerializer<double, &std::numeric_limits<double>::epsilon>>();

            Class<bool>()->
                Serializer<BoolSerializer>();

            MathReflect(this);

            Class<DataOverlayToken>()->
                Field("Uri", &DataOverlayToken::DataUri);
            Class<DataOverlayInfo>()->
                Field("ProviderId", &DataOverlayInfo::ProviderId)->
                Field("DataToken", &DataOverlayInfo::DataToken);

            Class<DynamicSerializableField>()->
                Field("TypeId", &DynamicSerializableField::TypeId);
                // Value data is injected into the hierarchy per-instance, since type is dynamic.

            // Reflects variant monostate class for serializing a variant with an empty alternative
            Class<VStd::monostate>();
        }

        if (createEditContext)
        {
            CreateEditContext();
        }

        if (registerIntegralTypes)
        {
            Internal::ReflectAny(this);
        }
    }

     //=========================================================================
    // DestroyEditContext
    //=========================================================================
    SerializeContext::~SerializeContext()
    {
        DestroyEditContext();
        decltype(m_perModuleSet) moduleSet = VStd::move(m_perModuleSet);
        for(PerModuleGenericClassInfo* module : moduleSet)
        {
            module->UnregisterSerializeContext(this);
        }
    }

    void SerializeContext::CleanupModuleGenericClassInfo()
    {
        GetCurrentSerializeContextModule().UnregisterSerializeContext(this);
    }

    //=========================================================================
    // CreateEditContext
    //=========================================================================
    EditContext* SerializeContext::CreateEditContext()
    {
        if (m_editContext == nullptr)
        {
            m_editContext = vnew EditContext(*this);
        }

        return m_editContext;
    }

    //=========================================================================
    // DestroyEditContext
    //=========================================================================
    void SerializeContext::DestroyEditContext()
    {
        if (m_editContext)
        {
            delete m_editContext;
            m_editContext = nullptr;
        }
    }

    //=========================================================================
    // GetEditContext
    // [11/8/2012]
    //=========================================================================
    EditContext* SerializeContext::GetEditContext() const
    {
        return m_editContext;
    }

    auto SerializeContext::RegisterType(const V::TypeId& typeId, V::SerializeContext::ClassData&& classData, CreateAnyFunc createAnyFunc) -> ClassBuilder
    {
        auto [typeToClassIter, inserted] = m_uuidMap.try_emplace(typeId, VStd::move(classData));
        m_classNameToUuid.emplace(V::Crc32(typeToClassIter->second.Name), typeId);
        m_uuidAnyCreationMap.emplace(typeId, createAnyFunc);

        return ClassBuilder(this, typeToClassIter);
    }

    bool SerializeContext::UnregisterType(const V::TypeId& typeId)
    {
        if (auto typeToClassIter = m_uuidMap.find(typeId); typeToClassIter != m_uuidMap.end())
        {
            ClassData& classData = typeToClassIter->second;
            RemoveClassData(&classData);

            auto [classNameRangeFirst, classNameRangeLast] = m_classNameToUuid.equal_range(Crc32(classData.m_name));
            while (classNameRangeFirst != classNameRangeLast)
            {
                if (classNameRangeFirst->second == typeId)
                {
                    classNameRangeFirst = m_classNameToUuid.erase(classNameRangeFirst);
                }
                else
                {
                    ++classNameRangeFirst;
                }
            }
            m_uuidAnyCreationMap.erase(typeId);
            m_uuidMap.erase(typeToClassIter);
            return true;
        }

        return false;
    }

    //=========================================================================
    // ClassDeprecate
    //=========================================================================
    void SerializeContext::ClassDeprecate(const char* name, const V::Uuid& typeUuid, VersionConverter converter)
    {
        if (IsRemovingReflection())
        {
            m_uuidMap.erase(typeUuid);
            return;
        }

        IdToClassMap::pair_iter_bool result = m_uuidMap.insert_key(typeUuid);
        V_Assert(result.second, "This class type %s has already been registered", name /*,typeUuid.ToString()*/);

        ClassData& cd = result.first->second;
        cd.Name = name;
        cd.TypeId = typeUuid;
        cd.Version = VersionClassDeprecated;
        cd.Converter = converter;
        cd.SerializerPtr = nullptr;
        cd.Factory = nullptr;
        cd.ContainerPtr = nullptr;
        cd.VObjectRtti = nullptr;
        cd.EventHandlerPtr = nullptr;
        cd.EditDataPtr = nullptr;

        m_classNameToUuid.emplace(V::Crc32(name), typeUuid);
    }

    //=========================================================================
    // FindClassId
    //=========================================================================
    VStd::vector<V::Uuid> SerializeContext::FindClassId(const V::Crc32& classNameCrc) const
    {
        auto&& findResult = m_classNameToUuid.equal_range(classNameCrc);
        VStd::vector<V::Uuid> retVal;
        for (auto&& currentIter = findResult.first; currentIter != findResult.second; ++currentIter)
        {
            retVal.push_back(currentIter->second);
        }
        return retVal;
    }

    //=========================================================================
    // FindClassData
    //=========================================================================
    const SerializeContext::ClassData*
    SerializeContext::FindClassData(const Uuid& classId, const SerializeContext::ClassData* parent, u32 elementNameCrc) const
    {
        SerializeContext::IdToClassMap::const_iterator it = m_uuidMap.find(classId);
        const SerializeContext::ClassData* cd = it != m_uuidMap.end() ? &it->second : nullptr;

        if (!cd)
        {
            // this is not a registered type try to find it in the parent scope by name and check / type and flags
            if (parent)
            {
                if (parent->ContainerPtr)
                {
                    const SerializeContext::ClassElement* classElement = parent->ContainerPtr->GetElement(elementNameCrc);
                    if (classElement && classElement->GenericClassInfoPtr)
                    {
                        if (classElement->GenericClassInfoPtr->CanStoreType(classId))
                        {
                            cd = classElement->GenericClassInfoPtr->GetClassData();
                        }
                    }
                }
                else if (elementNameCrc)
                {
                    for (size_t i = 0; i < parent->Elements.size(); ++i)
                    {
                        const SerializeContext::ClassElement& classElement = parent->Elements[i];
                        if (classElement.NameCrc == elementNameCrc && classElement.GenericClassInfoPtr)
                        {
                            if (classElement.GenericClassInfoPtr->CanStoreType(classId))
                            {
                                cd = classElement.GenericClassInfoPtr->GetClassData();
                            }
                            break;
                        }
                    }
                }
            }

            /* If the ClassData could not be found in the normal UuidMap, then the GenericUuid map will be searched.
             The GenericUuid map contains a mapping of Uuids to ClassData from which registered GenericClassInfo
             when reflecting
             */
            if (!cd)
            {
                auto genericClassInfo = FindGenericClassInfo(classId);
                if (genericClassInfo)
                {
                    cd = genericClassInfo->GetClassData();
                }
            }

            // The supplied Uuid will be searched in the enum -> underlying type map to fallback to using the integral type for enum fields reflected to a class
            // but not being explicitly reflected to the SerializeContext using the EnumBuilder
            if (!cd)
            {
                auto enumToUnderlyingTypeIdIter = m_enumTypeIdToUnderlyingTypeIdMap.find(classId);
                if (enumToUnderlyingTypeIdIter != m_enumTypeIdToUnderlyingTypeIdMap.end())
                {
                    const V::TypeId& underlyingTypeId = enumToUnderlyingTypeIdIter->second;
                    auto underlyingTypeIter = m_uuidMap.find(underlyingTypeId);
                    cd = underlyingTypeIter != m_uuidMap.end() ? &underlyingTypeIter->second : nullptr;
                }
            }
        }

        return cd;
    }

     //=========================================================================
    // FindGenericClassInfo
    //=========================================================================
    GenericClassInfo* SerializeContext::FindGenericClassInfo(const Uuid& classId) const
    {
        Uuid resolvedUuid = classId;
        // If the classId is not in the registered typeid to GenericClassInfo Map attempt to look it up in the legacy specialization
        // typeid map
        if (m_uuidGenericMap.count(resolvedUuid) == 0)
        {
            auto foundIt = m_legacySpecializeTypeIdToTypeIdMap.find(classId);
            if (foundIt != m_legacySpecializeTypeIdToTypeIdMap.end())
            {
                resolvedUuid = foundIt->second;
            }
        }
        auto genericClassIt = m_uuidGenericMap.find(resolvedUuid);
        if (genericClassIt != m_uuidGenericMap.end())
        {
            return genericClassIt->second;
        }

        return nullptr;
    }

    const TypeId& SerializeContext::GetUnderlyingTypeId(const TypeId& enumTypeId) const
    {
        auto enumToUnderlyingTypeIdIter = m_enumTypeIdToUnderlyingTypeIdMap.find(enumTypeId);
        return enumToUnderlyingTypeIdIter != m_enumTypeIdToUnderlyingTypeIdMap.end() ? enumToUnderlyingTypeIdIter->second : enumTypeId;
    }

    void SerializeContext::RegisterGenericClassInfo(const Uuid& classId, GenericClassInfo* genericClassInfo, const CreateAnyFunc& createAnyFunc)
    {
        if (!genericClassInfo)
        {
            return;
        }

        if (IsRemovingReflection())
        {
            RemoveGenericClassInfo(genericClassInfo);
        }
        else
        {
            // Make sure that the ModuleCleanup structure has the serializeContext set on it.
            GetCurrentSerializeContextModule().RegisterSerializeContext(this);
            // Find the module GenericClassInfo in the SerializeContext GenericClassInfo multimap and add if it is not part of the SerializeContext multimap
            auto scGenericClassInfoRange = m_uuidGenericMap.equal_range(classId);
            auto scGenericInfoFoundIt = VStd::find_if(scGenericClassInfoRange.first, scGenericClassInfoRange.second, [genericClassInfo](const VStd::pair<V::Uuid, GenericClassInfo*>& genericPair)
            {
                return genericClassInfo == genericPair.second;
            });

            if (scGenericInfoFoundIt == scGenericClassInfoRange.second)
            {
                m_uuidGenericMap.emplace(classId, genericClassInfo);
                m_uuidAnyCreationMap.emplace(classId, createAnyFunc);
                m_classNameToUuid.emplace(genericClassInfo->GetClassData()->Name, classId);
                m_legacySpecializeTypeIdToTypeIdMap.emplace(genericClassInfo->GetLegacySpecializedTypeId(), classId);
            }
        }
    }

    VStd::any SerializeContext::CreateAny(const Uuid& classId)
    {
        auto anyCreationIt = m_uuidAnyCreationMap.find(classId);
        return anyCreationIt != m_uuidAnyCreationMap.end() ? anyCreationIt->second(this) : VStd::any();
    }

    //=========================================================================
    // CanDowncast
    //=========================================================================
    bool SerializeContext::CanDowncast(const Uuid& fromClassId, const Uuid& toClassId, const IRttiHelper* fromClassHelper, const IRttiHelper* toClassHelper) const
    {
        // Same type
        if (fromClassId == toClassId || GetUnderlyingTypeId(fromClassId) == toClassId)
        {
            return true;
        }

        // rtti info not provided, see if we can cast using reflection data first
        if (!fromClassHelper)
        {
            const ClassData* fromClass = FindClassData(fromClassId);
            if (!fromClass)
            {
                return false;   // Can't cast without class data or rtti
            }
            for (size_t i = 0; i < fromClass->Elements.size(); ++i)
            {
                if ((fromClass->Elements[i].Flags & ClassElement::FLG_BASE_CLASS) != 0)
                {
                    if (fromClass->Elements[i].TypeId == toClassId)
                    {
                        return true;
                    }
                }
            }

            if (!fromClass->VObjectRtti)
            {
                return false;   // Reflection info failed to cast and we can't find rtti info
            }
            fromClassHelper = fromClass->VObjectRtti;
        }

        if (!toClassHelper)
        {
            const ClassData* toClass = FindClassData(toClassId);
            if (!toClass || !toClass->VObjectRtti)
            {
                return false;   // We can't cast without class data or rtti helper
            }
            toClassHelper = toClass->VObjectRtti;
        }

        // Rtti available, use rtti cast
        return fromClassHelper->IsTypeOf(toClassHelper->GetTypeId());
    }

    //=========================================================================
    // DownCast
    //=========================================================================
    void* SerializeContext::DownCast(void* instance, const Uuid& fromClassId, const Uuid& toClassId, const IRttiHelper* fromClassHelper, const IRttiHelper* toClassHelper) const
    {
        // Same type
        if (fromClassId == toClassId || GetUnderlyingTypeId(fromClassId) == toClassId)
        {
            return instance;
        }

        // rtti info not provided, see if we can cast using reflection data first
        if (!fromClassHelper)
        {
            const ClassData* fromClass = FindClassData(fromClassId);
            if (!fromClass)
            {
                return nullptr;
            }

            for (size_t i = 0; i < fromClass->Elements.size(); ++i)
            {
                if ((fromClass->Elements[i].Flags & ClassElement::FLG_BASE_CLASS) != 0)
                {
                    if (fromClass->Elements[i].TypeId == toClassId)
                    {
                        return (char*)(instance) + fromClass->Elements[i].Offset;
                    }
                }
            }

            if (!fromClass->VObjectRtti)
            {
                return nullptr;    // Reflection info failed to cast and we can't find rtti info
            }
            fromClassHelper = fromClass->VObjectRtti;
        }

        if (!toClassHelper)
        {
            const ClassData* toClass = FindClassData(toClassId);
            if (!toClass || !toClass->VObjectRtti)
            {
                return nullptr;    // We can't cast without class data or rtti helper
            }
            toClassHelper = toClass->VObjectRtti;
        }

        // Rtti available, use rtti cast
        return fromClassHelper->Cast(instance, toClassHelper->GetTypeId());
    }

    //=========================================================================
    // DataElement
    //=========================================================================
    SerializeContext::DataElement::DataElement()
        : Name(nullptr)
        , NameCrc(0)
        , DataSize(0)
        , ByteStream(&Buffer)
        , StreamPtr(nullptr)
    {
    }

    //=========================================================================
    // ~DataElement
    //=========================================================================
    SerializeContext::DataElement::~DataElement()
    {
       Buffer.clear();
    }

     //=========================================================================
    // DataElement
    //=========================================================================
    SerializeContext::DataElement::DataElement(const DataElement& rhs)
        : Name(rhs.Name)
        , NameCrc(rhs.NameCrc)
        , DataSize(rhs.DataSize)
        , ByteStream(&Buffer)
    {
        Id = rhs.Id;
        Version = rhs.Version;
        DataCategory = rhs.DataCategory;

        Buffer = rhs.Buffer;
        ByteStream = IO::ByteContainerStream<VStd::vector<char> >(&Buffer);
        ByteStream.Seek(rhs.ByteStream.GetCurPos(), IO::GenericStream::ST_SEEK_BEGIN);
        if (rhs.StreamPtr == &rhs.ByteStream)
        {
            V_Assert(rhs.DataSize == rhs.Buffer.size(), "Temp buffer must contain only the data for current element!");
            StreamPtr = &ByteStream;
        }
        else
        {
            StreamPtr = rhs.StreamPtr;
        }
    }

     //=========================================================================
    // DataElement
    //=========================================================================
    SerializeContext::DataElement& SerializeContext::DataElement::operator= (const DataElement& rhs)
    {
        Name = rhs.Name;
        NameCrc = rhs.NameCrc;
        Id = rhs.Id;
        Version = rhs.Version;
        DataSize = rhs.DataSize;
        DataCategory = rhs.DataCategory;

        Buffer = rhs.Buffer;
        ByteStream = IO::ByteContainerStream<VStd::vector<char> >(&Buffer);
        ByteStream.Seek(rhs.ByteStream.GetCurPos(), IO::GenericStream::ST_SEEK_BEGIN);

        if (rhs.StreamPtr == &rhs.ByteStream)
        {
            V_Assert(rhs.DataSize == rhs.Buffer.size(), "Temp buffer must contain only the data for current element!");
            StreamPtr = &ByteStream;
        }
        else
        {
            StreamPtr = rhs.StreamPtr;
        }

        return *this;
    }

    //=========================================================================
    // DataElement
    //=========================================================================
    SerializeContext::DataElement::DataElement(DataElement&& rhs)
        : Name(rhs.Name)
        , NameCrc(rhs.NameCrc)
        , DataSize(rhs.DataSize)
        , ByteStream(&Buffer)
    {
        Id = rhs.Id;
        Version = rhs.Version;
        DataCategory = rhs.DataCategory;
        if (rhs.StreamPtr == &rhs.ByteStream)
        {
            V_Assert(rhs.DataSize == rhs.Buffer.size(), "Temp buffer must contain only the data for current element!");
            StreamPtr = &ByteStream;
        }
        else
        {
            StreamPtr = rhs.StreamPtr;
        }

        Buffer = VStd::move(rhs.Buffer);
        ByteStream = IO::ByteContainerStream<VStd::vector<char> >(&Buffer);
        ByteStream.Seek(rhs.ByteStream.GetCurPos(), IO::GenericStream::ST_SEEK_BEGIN);

        rhs.Name = nullptr;
        rhs.NameCrc = 0;
        rhs:Id = Uuid::CreateNull();
        rhs.Version = 0;
        rhs.DataSize = 0;
        rhs.ByteStream.Seek(0, IO::GenericStream::ST_SEEK_BEGIN);
        if (StreamPtr == &ByteStream)
        {
            rhs.StreamPtr = &rhs.ByteStream;
        }
        else
        {
            rhs.StreamPtr = nullptr;
        }
    }

    //=========================================================================
    // DataElement
    //=========================================================================
    SerializeContext::DataElement& SerializeContext::DataElement::operator= (DataElement&& rhs)
    {
        Name = rhs.Name;
        NameCrc = rhs.NameCrc;
        Id = rhs.Id;
        Version = rhs.Version;
        DataSize = rhs.DataSize;
        DataCategory = rhs.DataCategory;
        if (rhs.StreamPtr == &rhs.ByteStream)
        {
            V_Assert(rhs.DataSize == rhs.Buffer.size(), "Temp buffer must contain only the data for current element!");
            StreamPtr = &ByteStream;
        }
        else
        {
            StreamPtr = rhs.StreamPtr;
        }


        Buffer = VStd::move(rhs.Buffer);
        ByteStream = IO::ByteContainerStream<VStd::vector<char> >(&Buffer);
        ByteStream.Seek(rhs.ByteStream.GetCurPos(), IO::GenericStream::ST_SEEK_BEGIN);

        rhs.Name = nullptr;
        rhs.NameCrc = 0;
        rhs.Id = Uuid::CreateNull();
        rhs.Version = 0;
        rhs.DataSize = 0;
        rhs.ByteStream.Seek(0, IO::GenericStream::ST_SEEK_BEGIN);
        if (StreamPtr == &ByteStream)
        {
            rhs.StreamPtr = &rhs.ByteStream;
        }
        else
        {
            rhs.StreamPtr = nullptr;
        }

        return *this;
    }

    //=========================================================================
    // Convert
    //=========================================================================
    bool SerializeContext::DataElementNode::Convert(SerializeContext& sc, const Uuid& id)
    {
        // remove sub elements
        while (!m_subElements.empty())
        {
            RemoveElement(static_cast<int>(m_subElements.size()) - 1);
        }

        // replace element
        m_element.Id = id;
        m_element.DataSize = 0;
        m_element.Buffer.clear();
        m_element.StreamPtr = &m_element.ByteStream;

        m_classData = sc.FindClassData(m_element.Id);
        V_Assert(m_classData, "You are adding element to an unregistered class!");
        m_element.Version = m_classData->Version;

        return true;
    }

    //=========================================================================
    // Convert
    //=========================================================================
    bool SerializeContext::DataElementNode::Convert(SerializeContext& sc, const char* name, const Uuid& id)
    {
        V_Assert(name != nullptr && strlen(name) > 0, "Empty name is an INVALID element name!");
        u32 nameCrc = Crc32(name);

#if defined(V_ENABLE_TRACING)
        if (FindElement(nameCrc) != -1)
        {
            V_Error("Serialize", false, "During reflection of class '%s' - member %s is declared more than once.", m_classData->Name ? m_classData->Name : "<unknown class>", name);
            return false;
        }
#endif // V_ENABLE_TRACING

        // remove sub elements
        while (!m_subElements.empty())
        {
            RemoveElement(static_cast<int>(m_subElements.size()) - 1);
        }

        // replace element
        m_element.Name = name;
        m_element.NameCrc = nameCrc;
        m_element.Id = id;
        m_element.DataSize = 0;
        m_element.Buffer.clear();
        m_element.StreamPtr = &m_element.ByteStream;

        m_classData = sc.FindClassData(m_element.Id);
        V_Assert(m_classData, "You are adding element to an unregistered class!");
        m_element.Version = m_classData->Version;

        return true;
    }

    //=========================================================================
    // FindElement
    //=========================================================================
    int SerializeContext::DataElementNode::FindElement(u32 crc)
    {
        for (size_t i = 0; i < m_subElements.size(); ++i)
        {
            if (m_subElements[i].m_element.NameCrc == crc)
            {
                return static_cast<int>(i);
            }
        }
        return -1;
    }

    //=========================================================================
    // FindSubElement
    //=========================================================================
    SerializeContext::DataElementNode* SerializeContext::DataElementNode::FindSubElement(u32 crc)
    {
        int index = FindElement(crc);
        return index >= 0 ? &m_subElements[index] : nullptr;
    }

    //=========================================================================
    // RemoveElement
    //=========================================================================
    void SerializeContext::DataElementNode::RemoveElement(int index)
    {
        V_Assert(index >= 0 && index < static_cast<int>(m_subElements.size()), "Invalid index passed to RemoveElement");

        DataElementNode& node = m_subElements[index];

        node.m_element.DataSize = 0;
        node.m_element.Buffer.clear();
        node.m_element.StreamPtr = nullptr;

        while (!node.m_subElements.empty())
        {
            node.RemoveElement(static_cast<int>(node.m_subElements.size()) - 1);
        }

        m_subElements.erase(m_subElements.begin() + index);
    }

    //=========================================================================
    // RemoveElementByName
    //=========================================================================
    bool SerializeContext::DataElementNode::RemoveElementByName(u32 crc)
    {
        int index = FindElement(crc);
        if (index >= 0)
        {
            RemoveElement(index);
            return true;
        }
        return false;
    }

    //=========================================================================
    // AddElement
    //=========================================================================
    int SerializeContext::DataElementNode::AddElement(const DataElementNode& elem)
    {
        m_subElements.push_back(elem);
        return static_cast<int>(m_subElements.size() - 1);
    }

    //=========================================================================
    // AddElement
    //=========================================================================
    int SerializeContext::DataElementNode::AddElement(SerializeContext& sc, const char* name, const Uuid& id)
    {
        const V::SerializeContext::ClassData* classData = sc.FindClassData(id);
        V_Assert(classData, "You are adding element to an unregistered class!");
        return AddElement(sc, name, *classData);
    }

    //=========================================================================
    // AddElement
    //=========================================================================
    int SerializeContext::DataElementNode::AddElement(SerializeContext& sc, const char* name, const ClassData& classData)
    {
        (void)sc;
        V_Assert(name != nullptr && strlen(name) > 0, "Empty name is an INVALID element name!");
        u32 nameCrc = Crc32(name);

    #if defined(V_ENABLE_TRACING)
        if (!m_classData->ContainerPtr && FindElement(nameCrc) != -1)
        {
            V_Error("Serialize", false, "During reflection of class '%s' - member %s is declared more than once.", classData.Name ? classData.Name : "<unknown class>", name);
            return -1;
        }
    #endif // V_ENABLE_TRACING

        DataElementNode node;
        node.m_element.Name = name;
        node.m_element.NameCrc = nameCrc;
        node.m_element.Id = classData.TypeId;
        node.m_classData = &classData;
        node.m_element.Version = classData.Version;

        m_subElements.push_back(node);
        return static_cast<int>(m_subElements.size() - 1);
    }

    int SerializeContext::DataElementNode::AddElement(SerializeContext& sc, VStd::string_view name, GenericClassInfo* genericClassInfo)
    {
        (void)sc;
        V_Assert(!name.empty(), "Empty name is an INVALID element name!");

        if (!genericClassInfo)
        {
            V_Assert(false, "Supplied GenericClassInfo is nullptr. ClassData cannot be retrieved.");
            return -1;
        }

        u32 nameCrc = Crc32(name.data());
        DataElementNode node;
        node.m_element.Name = name.data();
        node.m_element.NameCrc = nameCrc;
        node.m_element.Id = genericClassInfo->GetClassData()->TypeId;
        node.m_classData = genericClassInfo->GetClassData();
        node.m_element.Version = node.m_classData->Version;

        m_subElements.push_back(node);
        return static_cast<int>(m_subElements.size() - 1);
    }

    //=========================================================================
    // ReplaceElement
    //=========================================================================
    int SerializeContext::DataElementNode::ReplaceElement(SerializeContext& sc, int index, const char* name, const Uuid& id)
    {
        DataElementNode& node = m_subElements[index];
        if (node.Convert(sc, name, id))
        {
            return index;
        }
        else
        {
            return -1;
        }
    }

    //=========================================================================
    // SetName
    //=========================================================================
    void SerializeContext::DataElementNode::SetName(const char* newName)
    {
        m_element.Name = newName;
        m_element.NameCrc = Crc32(newName);
    }

     //=========================================================================
    // SetDataHierarchy
    //=========================================================================
    bool SerializeContext::DataElementNode::SetDataHierarchy(SerializeContext& sc, const void* objectPtr, const Uuid& classId, ErrorHandler* errorHandler, const ClassData* classData)
    {
        V_Assert(m_element.Id == classId, "SetDataHierarchy called with mismatched class type {%s} for element %s",
            classId.ToString<VStd::string>().c_str(), m_element.Name);

        VStd::vector<DataElementNode*> nodeStack;
        DataElementNode* topNode = this;
        bool success = false;

        auto beginCB = [&sc, &nodeStack, topNode, &success, errorHandler](void* ptr, const SerializeContext::ClassData* elementClassData, const SerializeContext::ClassElement* elementData) -> bool
            {
                if (nodeStack.empty())
                {
                    success = true;
                    nodeStack.push_back(topNode);
                    return true;
                }

                DataElementNode* parentNode = nodeStack.back();
                parentNode->m_subElements.reserve(64);

                V_Assert(elementData, "Missing element data");
                V_Assert(elementClassData, "Missing class data for element %s", elementData ? elementData->Name : "<unknown>");

                int elementIndex = -1;
                if (elementData)
                {
                    elementIndex = elementData->GenericClassInfoPtr
                        ? parentNode->AddElement(sc, elementData->Name, elementData->GenericClassInfoPtr)
                        : parentNode->AddElement(sc, elementData->Name, *elementClassData);
                }
                if (elementIndex >= 0)
                {
                    DataElementNode& newNode = parentNode->GetSubElement(elementIndex);

                    if (elementClassData->SerializerPtr)
                    {
                        void* resolvedObject = ptr;
                        if (elementData && elementData->Flags & SerializeContext::ClassElement::FLG_POINTER)
                        {
                            resolvedObject = *(void**)(resolvedObject);

                            if (resolvedObject && elementData->VObjectRtti)
                            {
                                V::Uuid actualClassId = elementData->VObjectRtti->GetActualUuid(resolvedObject);
                                if (actualClassId != elementData->TypeId)
                                {
                                    const V::SerializeContext::ClassData* actualClassData = sc.FindClassData(actualClassId);
                                    if (actualClassData)
                                    {
                                        resolvedObject = elementData->VObjectRtti->Cast(resolvedObject, actualClassData->VObjectRtti->GetTypeId());
                                    }
                                }
                            }
                        }

                        V_Assert(newNode.m_element.ByteStream.GetCurPos() == 0, "This stream should be only for our data element");
                        newNode.m_element.DataSize = elementClassData->SerializerPtr->Save(resolvedObject, newNode.m_element.ByteStream);
                        newNode.m_element.ByteStream.Truncate();
                        newNode.m_element.ByteStream.Seek(0, IO::GenericStream::ST_SEEK_BEGIN); // reset stream position
                        newNode.m_element.StreamPtr = &newNode.m_element.ByteStream;

                        newNode.m_element.DataCategory = DataElement::DT_BINARY;
                    }

                    nodeStack.push_back(&newNode);
                    success = true;
                }
                else
                {
                    const VStd::string error =
                        VStd::string::format("Failed to add sub-element \"%s\" to element \"%s\".",
                            elementData ? elementData->Name : "<unknown>",
                            parentNode->m_element.Name);

                    if (errorHandler)
                    {
                        errorHandler->ReportError(error.c_str());
                    }
                    else
                    {
                        V_Error("Serialize", false, "%s", error.c_str());
                    }

                    success = false;
                    return false; // Stop enumerating.
                }

                return true;
            };

        auto endCB = [&nodeStack, &success]() -> bool
            {
                if (success)
                {
                    nodeStack.pop_back();
                }

                return true;
            };

        EnumerateInstanceCallContext callContext(
            beginCB,
            endCB,
            &sc,
            SerializeContext::ENUM_ACCESS_FOR_READ,
            errorHandler);

        sc.EnumerateInstanceConst(
            &callContext,
            objectPtr,
            classId,
            classData,
            nullptr
            );

        return success;
    }

    bool SerializeContext::DataElementNode::GetClassElement(ClassElement& classElement, const DataElementNode& parentDataElement, ErrorHandler* errorHandler) const
    {
        bool elementFound = false;
        if (parentDataElement.m_classData)
        {
            const ClassData* parentClassData = parentDataElement.m_classData;
            if (parentClassData->ContainerPtr)
            {
                IDataContainer* classContainer = parentClassData->ContainerPtr;
                // store the container element in classElementMetadata
                ClassElement classElementMetadata;
                bool classElementFound = classContainer->GetElement(classElementMetadata, m_element);
                V_Assert(classElementFound, "'%s'(0x%x) is not a valid element name for container type %s", m_element.Name ? m_element.Name : "NULL", m_element.NameCrc, parentClassData->Name);
                if (classElementFound)
                {
                    // if the container contains pointers, then the elements could be a derived type,
                    // otherwise we need the uuids to be exactly the same.
                    if (classElementMetadata.Flags & SerializeContext::ClassElement::FLG_POINTER)
                    {
                        bool downcastPossible = m_element.Id == classElementMetadata.TypeId;
                        downcastPossible = downcastPossible || (m_classData->VObjectRtti && classElementMetadata.VObjectRtti && m_classData->VObjectRtti->IsTypeOf(classElementMetadata.VObjectRtti->GetTypeId()));
                        if (!downcastPossible)
                        {
                            VStd::string error = VStd::string::format("Element of type %s cannot be added to container of pointers to type %s!"
                                , m_element.Id.ToString<VStd::string>().c_str(), classElementMetadata.TypeId.ToString<VStd::string>().c_str());
                            errorHandler->ReportError(error.c_str());

                            return false;
                        }
                    }
                    else
                    {
                        if (m_element.Id != classElementMetadata.TypeId)
                        {
                            VStd::string error = VStd::string::format("Element of type %s cannot be added to container of type %s!"
                                , m_element.Id.ToString<VStd::string>().c_str(), classElementMetadata.TypeId.ToString<VStd::string>().c_str());
                            errorHandler->ReportError(error.c_str());

                            return false;
                        }
                    }
                    classElement = classElementMetadata;
                    elementFound = true;
                }
            }
            else
            {
                for (size_t i = 0; i < parentClassData->Elements.size(); ++i)
                {
                    const SerializeContext::ClassElement* childElement = &parentClassData->Elements[i];
                    if (childElement->NameCrc == m_element.NameCrc)
                    {
                        // if the member is a pointer type, then the pointer could be a derived type,
                        // otherwise we need the uuids to be exactly the same.
                        if (childElement->Flags & SerializeContext::ClassElement::FLG_POINTER)
                        {
                            // Verify that a downcast is possible
                            bool downcastPossible = m_element.Id == childElement->TypeId;
                            downcastPossible = downcastPossible || (m_classData->VObjectRtti && childElement->VObjectRtti && m_classData->VObjectRtti->IsTypeOf(childElement->VObjectRtti->GetTypeId()));
                            if (downcastPossible)
                            {
                                classElement = *childElement;
                                elementFound = true;
                            }
                        }
                        else
                        {
                            if (m_element.Id == childElement->TypeId)
                            {
                                classElement = *childElement;
                                elementFound = true;
                            }
                        }
                        break;
                    }
                }
            }
        }
        return elementFound;
    }

    bool SerializeContext::DataElementNode::GetDataHierarchyEnumerate(ErrorHandler* errorHandler, NodeStack& nodeStack)
    {
        if (nodeStack.empty())
        {
            return true;
        }

        void* parentPtr = nodeStack.back().Ptr;
        DataElementNode* parentDataElement = nodeStack.back().DataElement;
        V_Assert(parentDataElement, "parentDataElement is null, cannot enumerate data from data element (%s:%s)",
            m_element.Name ? m_element.Name : "", m_element.Id.ToString<VStd::string>().data());
        if (!parentDataElement)
        {
            return false;
        }

        bool success = true;

        if (!m_classData)
        {
            V_Error("Serialize", false, R"(Cannot enumerate data from data element (%s:%s) from parent data element (%s:%s) with class name "%s" because the ClassData does not exist.)"
                " This can indicate that the class is not reflected at the point of this call. If this is class is reflected as part of a gem"
                " check if that gem is loaded", m_element.Name ? m_element.Name : "", m_element.Id.ToString<VStd::string>().data(),
                parentDataElement->m_element.Name ? parentDataElement->m_element.Name : "", parentDataElement->m_element.Id.ToString<VStd::string>().data(), parentDataElement->m_classData->Name);
            return false;
        }

        V::SerializeContext::ClassElement classElement;
        bool classElementFound = parentDataElement && GetClassElement(classElement, *parentDataElement, errorHandler);
        V_Warning("Serialize", classElementFound, R"(Unable to find class element for data element(%s:%s) with class name "%s" that is a child of parent data element(%s:%s) with class name "%s")",
            m_element.Name ? m_element.Name : "", m_element.Id.ToString<VStd::string>().data(), m_classData->Name,
            parentDataElement->m_element.Name ? parentDataElement->m_element.Name : "", parentDataElement->m_element.Id.ToString<VStd::string>().data(), parentDataElement->m_classData->Name);

        if (classElementFound)
        {
            void* dataAddress = nullptr;
            IDataContainer* dataContainer = parentDataElement->m_classData->ContainerPtr;
            if (dataContainer) // container elements
            {
                int& parentCurrentContainerElementIndex = nodeStack.back().CurrentContainerElementIndex;
                // add element to the array
                if (dataContainer->CanAccessElementsByIndex() && dataContainer->Size(parentPtr) > parentCurrentContainerElementIndex)
                {
                    dataAddress = dataContainer->GetElementByIndex(parentPtr, &classElement, parentCurrentContainerElementIndex);
                }
                else
                {
                    dataAddress = dataContainer->ReserveElement(parentPtr, &classElement);
                }

                if (dataAddress == nullptr)
                {
                    VStd::string error = VStd::string::format("Failed to reserve element in container. The container may be full. Element %u will not be added to container.", static_cast<unsigned int>(parentCurrentContainerElementIndex));
                    errorHandler->ReportError(error.c_str());
                }

                parentCurrentContainerElementIndex++;
            }
            else
            {   // normal elements
                dataAddress = reinterpret_cast<char*>(parentPtr) + classElement.Offset;
            }

            void* reserveAddress = dataAddress;

            // create a new instance if needed
            if (classElement.Flags & SerializeContext::ClassElement::FLG_POINTER)
            {
                // create a new instance if we are referencing it by pointer
                V_Assert(m_classData->Factory != nullptr, "We are attempting to create '%s', but no factory is provided! Either provide factory or change data member '%s' to value not pointer!", m_classData->Name, classElement.Name);
                void* newDataAddress = m_classData->Factory->Create(m_classData->Name);

                // we need to account for additional offsets if we have a pointer to
                // a base class.
                void* basePtr = nullptr;
                if (m_element.Id == classElement.TypeId)
                {
                    basePtr = newDataAddress;
                }
                else if(m_classData->VObjectRtti && classElement.VObjectRtti)
                {
                    basePtr = m_classData->VObjectRtti->Cast(newDataAddress, classElement.VObjectRtti->GetTypeId());
                }
                V_Assert(basePtr != nullptr, dataContainer
                    ? "Can't cast container element %s(0x%x) to %s, make sure classes are registered in the system and not generics!"
                    : "Can't cast %s(0x%x) to %s, make sure classes are registered in the system and not generics!"
                    , m_element.Name ? m_element.Name : "NULL"
                    , m_element.NameCrc
                    , m_classData->Name);

                *reinterpret_cast<void**>(dataAddress) = basePtr; // store the pointer in the class
                dataAddress = newDataAddress;
            }

            DataElement& rawElement = m_element;
            if (m_classData->SerializerPtr && rawElement.DataSize != 0)
            {
                rawElement.ByteStream.Seek(0, IO::GenericStream::ST_SEEK_BEGIN);
                if (rawElement.DataCategory == DataElement::DT_TEXT)
                {
                    // convert to binary so we can load the data
                    VStd::string text;
                    text.resize_no_construct(rawElement.DataSize);
                    rawElement.ByteStream.Read(text.size(), reinterpret_cast<void*>(text.data()));
                    rawElement.ByteStream.Seek(0, IO::GenericStream::ST_SEEK_BEGIN);
                    rawElement.DataSize = m_classData->SerializerPtr->TextToData(text.c_str(), rawElement.Version, rawElement.ByteStream);
                    rawElement.ByteStream.Seek(0, IO::GenericStream::ST_SEEK_BEGIN);
                    rawElement.DataCategory = DataElement::DT_BINARY;
                }

                bool isLoaded = m_classData->SerializerPtr->Load(dataAddress, rawElement.ByteStream, rawElement.Version, rawElement.DataCategory == DataElement::DT_BINARY_BE);
                rawElement.ByteStream.Seek(0, IO::GenericStream::ST_SEEK_BEGIN); // reset stream position
                if (!isLoaded)
                {
                    const VStd::string error =
                        VStd::string::format(R"(Failed to load rawElement "%s" for class "%s" with parent element "%s".)",
                            rawElement.Name ? rawElement.Name : "<unknown>",
                            m_classData->Name,
                            m_element.Name);

                    if (errorHandler)
                    {
                        errorHandler->ReportError(error.c_str());
                    }
                    else
                    {
                        V_Error("Serialize", false, "%s", error.c_str());
                    }

                    success = false;
                }
            }

            // Push current DataElementNode to stack
            DataElementInstanceData node;
            node.Ptr = dataAddress;
            node.DataElement = this;
            nodeStack.push_back(node);

            for (int dataElementIndex = 0; dataElementIndex < m_subElements.size(); ++dataElementIndex)
            {
                DataElementNode& subElement = m_subElements[dataElementIndex];
                if (!subElement.GetDataHierarchyEnumerate(errorHandler, nodeStack))
                {
                    success = false;
                    break;
                }
            }

            // Pop stack
            nodeStack.pop_back();

            if (dataContainer)
            {
                dataContainer->StoreElement(parentPtr, reserveAddress);
            }
        }

        return success;
    }

    //=========================================================================
    // GetDataHierarchy
    //=========================================================================
    bool SerializeContext::DataElementNode::GetDataHierarchy(void* objectPtr, const Uuid& classId, ErrorHandler* errorHandler)
    {
        (void)classId;
        V_Assert(m_element.Id == classId, "GetDataHierarchy called with mismatched class type {%s} for element %s",
            classId.ToString<VStd::string>().c_str(), m_element.Name);

        NodeStack nodeStack;
        DataElementInstanceData topNode;
        topNode.Ptr = objectPtr;
        topNode.DataElement = this;
        nodeStack.push_back(topNode);

        bool success = true;
        for (size_t i = 0; i < m_subElements.size(); ++i)
        {
            if (!m_subElements[i].GetDataHierarchyEnumerate(errorHandler, nodeStack))
            {
                success = false;
                break;
            }
        }

        return success;
    }

    //=========================================================================
    // ClassBuilder::~ClassBuilder
    //=========================================================================
    SerializeContext::ClassBuilder::~ClassBuilder() {
#if defined(V_ENABLE_TRACING)
        if (!m_context->IsRemovingReflection()) {
            if (m_classData->second.SerializerPtr) {
                V_Assert(m_classData->second.Elements.empty(),
                    "Reflection error for class %s.\n"
                    "Classes with custom serializers are not permitted to also declare serialized elements.\n"
                    "This is often caused by calling SerializeWithNoData() or specifying a custom serializer on a class which \n"
                    "is derived from a base class that has serialized elements.",
                    m_classData->second.Name ? m_classData->second.Name : "<Unknown Class>"
                );
            }
        }
#endif // V_ENABLE_TRACING
    }
    //=========================================================================
    // ClassBuilder::NameChange
    //=========================================================================
    SerializeContext::ClassBuilder* SerializeContext::ClassBuilder::NameChange(unsigned int fromVersion, 
        unsigned int toVersion, VStd::string_view oldFieldName, VStd::string_view newFieldName)
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

        m_classData->second.DataPatchUpgrader.AddFieldUpgrade(vnew DataPatchNameUpgrade(fromVersion, toVersion, oldFieldName, newFieldName));

        return this;
    }
    //=========================================================================
    // ClassBuilder::Version
    //=========================================================================
    SerializeContext::ClassBuilder* SerializeContext::ClassBuilder::Version(unsigned int version, VersionConverter converter)
    {
        if (m_context->IsRemovingReflection())
        {
            return this; // we have already removed the class data.
        }
        V_Assert(version != VersionClassDeprecated, "You cannot use %u as the version, it is reserved by the system!", version);
        m_classData->second.Version = version;
        m_classData->second.Converter = converter;
        return this;
    }

    SerializeContext::ClassBuilder* SerializeContext::ClassBuilder::Serializer(IDataSerializerPtr serializer)
    {
        if (m_context->IsRemovingReflection())
        {
            return this; // we have already removed the class data.
        }

        V_Assert(m_classData->second.Elements.empty(),
            "Class %s has a custom serializer, and can not have additional fields. Classes can either have a custom serializer or child fields.",
            m_classData->second.Name);

        m_classData->second.SerializerPtr = VStd::move(serializer);
        return this;
    }
    //=========================================================================
    // ClassBuilder::Serializer
    //=========================================================================
      SerializeContext::ClassBuilder* SerializeContext::ClassBuilder::Serializer(IDataSerializer* serializer)
    {
        return Serializer(IDataSerializerPtr(serializer, IDataSerializer::CreateNoDeleteDeleter()));
    }

    //=========================================================================
    // ClassBuilder::Serializer
    //=========================================================================
    SerializeContext::ClassBuilder* SerializeContext::ClassBuilder::SerializeWithNoData()
    {
        if (m_context->IsRemovingReflection())
        {
            return this; // we have already removed the class data.
        }
        m_classData->second.SerializerPtr = IDataSerializerPtr(&Serialize::StaticInstance<EmptySerializer>::_instance, IDataSerializer::CreateNoDeleteDeleter());
        return this;
    }

    //=========================================================================
    // ClassBuilder::EventHandler
    //=========================================================================
    SerializeContext::ClassBuilder* SerializeContext::ClassBuilder::EventHandler(IEventHandler* eventHandler)
    {
        if (m_context->IsRemovingReflection())
        {
            return this; // we have already removed the class data.
        }
        m_classData->second.EventHandlerPtr = eventHandler;
        return this;
    }

    //=========================================================================
    // ClassBuilder::DataContainer
    //=========================================================================
    SerializeContext::ClassBuilder* SerializeContext::ClassBuilder::DataContainer(IDataContainer* dataContainer)
    {
        if (m_context->IsRemovingReflection())
        {
            return this;
        }
        m_classData->second.ContainerPtr = dataContainer;
        return this;
    }

    //=========================================================================
    // ClassBuilder::PersistentId
    //=========================================================================
    SerializeContext::ClassBuilder* SerializeContext::ClassBuilder::PersistentId(ClassPersistentId persistentId)
    {
        if (m_context->IsRemovingReflection())
        {
            return this; // we have already removed the class data.
        }
        m_classData->second.PersistentId = persistentId;
        return this;
    }

    //=========================================================================
    // ClassBuilder::SerializerDoSave
    //=========================================================================
    SerializeContext::ClassBuilder* SerializeContext::ClassBuilder::SerializerDoSave(ClassDoSave doSave)
    {
        if (m_context->IsRemovingReflection())
        {
            return this; // we have already removed the class data.
        }
        m_classData->second.DoSave = doSave;
        return this;
    }

    //=========================================================================
    // EnumerateInstanceConst
    //=========================================================================
    bool SerializeContext::EnumerateInstanceConst(SerializeContext::EnumerateInstanceCallContext* callContext, const void* ptr, const Uuid& classId, const ClassData* classData, const ClassElement* classElement) const
    {
        V_Assert((callContext->AccessFlags & ENUM_ACCESS_FOR_WRITE) == 0, "You are asking the serializer to lock the data for write but you only have a const pointer!");
        return EnumerateInstance(callContext, const_cast<void*>(ptr), classId, classData, classElement);
    }

     //=========================================================================
    // EnumerateInstance
    // [10/31/2012]
    //=========================================================================
    bool SerializeContext::EnumerateInstance(SerializeContext::EnumerateInstanceCallContext* callContext, void* ptr, const Uuid& classId, const ClassData* classData, const ClassElement* classElement) const
    {
        // if useClassData is provided, just use it, otherwise try to find it using the classId provided.
        void* objectPtr = ptr;
        const V::Uuid* classIdPtr = &classId;
        const SerializeContext::ClassData* dataClassInfo = classData;

        if (classElement)
        {
            // if we are a pointer, then we may be pointing to a derived type.
            if (classElement->Flags & SerializeContext::ClassElement::FLG_POINTER)
            {
                // if ptr is a pointer-to-pointer, cast its value to a void* (or const void*) and dereference to get to the actual object pointer.
                objectPtr = *(void**)(ptr);

                if (!objectPtr)
                {
                    return true;    // nothing to serialize
                }
                if (classElement->VObjectRtti)
                {
                    const V::Uuid& actualClassId = classElement->VObjectRtti->GetActualUuid(objectPtr);
                    if (actualClassId != *classIdPtr)
                    {
                        // we are pointing to derived type, adjust class data, uuid and pointer.
                        classIdPtr = &actualClassId;
                        dataClassInfo = FindClassData(actualClassId);
                        if ( (dataClassInfo) && (dataClassInfo->VObjectRtti) ) // it could be missing RTTI if its deprecated.
                        {
                            objectPtr = classElement->VObjectRtti->Cast(objectPtr, dataClassInfo->VObjectRtti->GetTypeId());

                            V_Assert(objectPtr, "Potential Data Loss: VOBJECT_RTTI Cast to type %s Failed on element: %s", dataClassInfo->Name, classElement->Name);
                            if (!objectPtr)
                            {
                                #if defined (V_ENABLE_SERIALIZER_DEBUG)
                                // Additional error information: Provide Type IDs and the serialization stack from our enumeration
                                VStd::string sourceTypeID = dataClassInfo->TypeId.ToString<VStd::string>();
                                VStd::string targetTypeID = classElement->TypeId.ToString<VStd::string>();
                                VStd::string error = VStd::string::format("EnumarateElements RTTI Cast Error: %s -> %s", sourceTypeID.c_str(), targetTypeID.c_str());
                                callContext->EventHandlerPtr->ReportError(error.c_str());
                                #endif
                                return true;    // RTTI Error. Continue serialization
                            }
                        }
                    }
                }
            }
        }

        if (!dataClassInfo)
        {
            dataClassInfo = FindClassData(*classIdPtr);
        }

    #if defined(V_ENABLE_SERIALIZER_DEBUG)
        {
            DbgStackEntry de;
            de.DataPtr = objectPtr;
            de.UuidPtr = classIdPtr;
            de.ElementName     = classElement ? classElement->Name : nullptr;
            de.ClassDataPtr    = dataClassInfo;
            de.DlassElementPtr = classElement;
            callContext->ErrorHandlerPtr->Push(de);
        }
    #endif // V_ENABLE_SERIALIZER_DEBUG

        if (dataClassInfo == nullptr)
        {
    #if defined (V_ENABLE_SERIALIZER_DEBUG)
            VStd::string error;

            // output an error
            if (classElement && classElement->m_flags & SerializeContext::ClassElement::FLG_BASE_CLASS)
            {
                error = VStd::string::format("Element with class ID '%s' was declared as a base class of another type but is not registered with the serializer.  Either remove it from the Class<> call or reflect it.", classIdPtr->ToString<VStd::string>().c_str());
            }
            else
            {
                error = VStd::string::format("Element with class ID '%s' is not registered with the serializer!", classIdPtr->ToString<VStd::string>().c_str());
            }

            callContext->ErrorHandlerPtr->ReportError(error.c_str());

            callContext->ErrorHandlerPtr->Pop();
    #endif // V_ENABLE_SERIALIZER_DEBUG

            return true;    // we errored, but return true to continue enumeration of our siblings and other unrelated hierarchies
        }

        if (dataClassInfo->EventHandlerPtr)
        {
            if ((callContext->AccessFlags & ENUM_ACCESS_FOR_WRITE) == ENUM_ACCESS_FOR_WRITE)
            {
                dataClassInfo->EventHandlerPtr->OnWriteBegin(objectPtr);
            }
            else
            {
                dataClassInfo->EventHandlerPtr->OnReadBegin(objectPtr);
            }
        }

        bool keepEnumeratingSiblings = true;

        // Call beginElemCB for this element if there is one. If the callback
        // returns false, stop enumeration of this branch
        // pass the original ptr to the user instead of objectPtr because
        // he may want to replace the actual object.
        if (!callContext->BeginElemCB || callContext->BeginElemCB(ptr, dataClassInfo, classElement))
        {
            if (dataClassInfo->ContainerPtr)
            {
                dataClassInfo->ContainerPtr->EnumElements(objectPtr, callContext->ElementCallback);
            }
            else
            {
                for (size_t i = 0, n = dataClassInfo->Elements.size(); i < n; ++i)
                {
                    const SerializeContext::ClassElement& ed = dataClassInfo->Elements[i];
                    void* dataAddress = (char*)(objectPtr) + ed.Offset;
                    if (dataAddress)
                    {
                        const SerializeContext::ClassData* elemClassInfo = ed.GenericClassInfoPtr ? ed.GenericClassInfoPtr->GetClassData() : FindClassData(ed.TypeId, dataClassInfo, ed.NameCrc);

                        keepEnumeratingSiblings = EnumerateInstance(callContext, dataAddress, ed.TypeId, elemClassInfo, &ed);
                        if (!keepEnumeratingSiblings)
                        {
                            break;
                        }
                    }
                }

                if (dataClassInfo->TypeId == SerializeTypeInfo<DynamicSerializableField>::GetUuid())
                {
                    V::DynamicSerializableField* dynamicFieldDesc = reinterpret_cast<V::DynamicSerializableField*>(objectPtr);
                    if (dynamicFieldDesc->IsValid())
                    {
                        const V::SerializeContext::ClassData* dynamicTypeMetadata = FindClassData(dynamicFieldDesc->TypeId);
                        if (dynamicTypeMetadata)
                        {
                            V::SerializeContext::ClassElement dynamicElementData;
                            dynamicElementData.Name = "m_data";
                            dynamicElementData.NameCrc = V_CRC("m_data", 0x335cc942);
                            dynamicElementData.TypeId = dynamicFieldDesc->TypeId;
                            dynamicElementData.DataSize = sizeof(void*);
                            dynamicElementData.Offset = reinterpret_cast<size_t>(&(reinterpret_cast<V::DynamicSerializableField const volatile*>(0)->DataPtr));
                            dynamicElementData.VObjectRtti = nullptr; // we won't need this because we always serialize top classes.
                            dynamicElementData.GenericClassInfoPtr = FindGenericClassInfo(dynamicFieldDesc->TypeId);
                            dynamicElementData.EditDataPtr = nullptr; // we cannot have element edit data for dynamic fields.
                            dynamicElementData.Flags = ClassElement::FLG_DYNAMIC_FIELD | ClassElement::FLG_POINTER;
                            EnumerateInstance(callContext, &dynamicFieldDesc->DataPtr, dynamicTypeMetadata->TypeId, dynamicTypeMetadata, &dynamicElementData);
                        }
                        else
                        {
                            V_Error("Serialization", false, "Failed to find class data for 'Dynamic Serializable Field' with type=%s address=%p. Make sure this type is reflected, \
                                otherwise you will lose data during serialization!\n", dynamicFieldDesc->TypeId.ToString<VStd::string>().c_str(), dynamicFieldDesc->DataPtr);
                        }
                    }
                }
            }
        }

        // call endElemCB
        if (callContext->EndElemCB)
        {
            keepEnumeratingSiblings = callContext->EndElemCB();
        }

        if (dataClassInfo->EventHandlerPtr)
        {
            if ((callContext->AccessFlags & ENUM_ACCESS_HOLD) == 0)
            {
                if ((callContext->AccessFlags & ENUM_ACCESS_FOR_WRITE) == ENUM_ACCESS_FOR_WRITE)
                {
                    dataClassInfo->EventHandlerPtr->OnWriteEnd(objectPtr);
                }
                else
                {
                    dataClassInfo->EventHandlerPtr->OnReadEnd(objectPtr);
                }
            }
        }

    #if defined(V_ENABLE_SERIALIZER_DEBUG)
        callContext->EventHandlerPtr->Pop();
    #endif // V_ENABLE_SERIALIZER_DEBUG

        return keepEnumeratingSiblings;
    }

    /// ObjectCloneData
    struct ObjectCloneData
    {
        ObjectCloneData()
        {
            ParentStack.reserve(10);
        }
        struct ParentInfo
        {
            void* Ptr;
            void* ReservePtr; ///< Used for associative containers like set to store the original address returned by ReserveElement
            const SerializeContext::ClassData* ClassDataPtr;
            size_t ContainerIndexCounter; ///< Used for fixed containers like array, where the container doesn't store the size.
        };
        typedef VStd::vector<ParentInfo> ObjectParentStack;

        void*               Ptr;
        ObjectParentStack   ParentStack;
    };

    //=========================================================================
    // CloneObject
    //=========================================================================
    void* SerializeContext::CloneObject(const void* ptr, const Uuid& classId)
    {
        VStd::vector<char> scratchBuffer;

        ObjectCloneData cloneData;
        ErrorHandler m_errorLogger;

        V_Assert(ptr, "SerializeContext::CloneObject - Attempt to clone a nullptr.");

        if (!ptr)
        {
            return nullptr;
        }

        EnumerateInstanceCallContext callContext(
            VStd::bind(&SerializeContext::BeginCloneElement, this, VStd::placeholders::_1, VStd::placeholders::_2, VStd::placeholders::_3, &cloneData, &m_errorLogger, &scratchBuffer),
            VStd::bind(&SerializeContext::EndCloneElement, this, &cloneData),
            this,
            SerializeContext::ENUM_ACCESS_FOR_READ,
            &m_errorLogger);

        EnumerateInstance(
            &callContext
            , const_cast<void*>(ptr)
            , classId
            , nullptr
            , nullptr
            );

        return cloneData.Ptr;
    }

    //=========================================================================
    // CloneObjectInplace
    //=========================================================================
    void SerializeContext::CloneObjectInplace(void* dest, const void* ptr, const Uuid& classId)
    {
        VStd::vector<char> scratchBuffer;

        ObjectCloneData cloneData;
        cloneData.Ptr = dest;
        ErrorHandler errorLogger;

        V_Assert(ptr, "SerializeContext::CloneObjectInplace - Attempt to clone a nullptr.");

        if (ptr)
        {
            EnumerateInstanceCallContext callContext(
            VStd::bind(&SerializeContext::BeginCloneElementInplace, this, dest, VStd::placeholders::_1, VStd::placeholders::_2, VStd::placeholders::_3, &cloneData, &errorLogger, &scratchBuffer),
                VStd::bind(&SerializeContext::EndCloneElement, this, &cloneData),
                this,
                SerializeContext::ENUM_ACCESS_FOR_READ,
                &errorLogger);

            EnumerateInstance(
                &callContext
                , const_cast<void*>(ptr)
                , classId
                , nullptr
                , nullptr
            );
        }
    }

    V::SerializeContext::DataPatchUpgrade::DataPatchUpgrade(VStd::string_view fieldName, unsigned int fromVersion, unsigned int toVersion)
        : m_targetFieldName(fieldName)
        , m_targetFieldCRC(m_targetFieldName.data(), m_targetFieldName.size(), true)
        , m_fromVersion(fromVersion)
        , m_toVersion(toVersion)
    {}

    bool V::SerializeContext::DataPatchUpgrade::operator==(const DataPatchUpgrade& RHS) const
    {
        return m_upgradeType == RHS.m_upgradeType
            && m_targetFieldCRC == RHS.m_targetFieldCRC
            && m_fromVersion == RHS.m_fromVersion
            && m_toVersion == RHS.m_toVersion;
    }

    bool V::SerializeContext::DataPatchUpgrade::operator<(const DataPatchUpgrade& RHS) const
    {
        if (m_fromVersion < RHS.m_fromVersion)
        {
            return true;
        }

        if (m_fromVersion > RHS.m_fromVersion)
        {
            return false;
        }

        // We sort on to version in reverse order
        if (m_toVersion > RHS.m_toVersion)
        {
            return true;
        }

        if (m_toVersion < RHS.m_toVersion)
        {
            return false;
        }

        // When versions are equal, upgrades are prioritized by type in the
        // order in which they appear in the DataPatchUpgradeType enum.
        return m_upgradeType < RHS.m_upgradeType;
    }

    unsigned int V::SerializeContext::DataPatchUpgrade::FromVersion() const
    {
        return m_fromVersion;
    }

    unsigned int V::SerializeContext::DataPatchUpgrade::ToVersion() const
    {
        return m_toVersion;
    }

    const VStd::string& V::SerializeContext::DataPatchUpgrade::GetFieldName() const
    {
        return m_targetFieldName;
    }

    V::Crc32 V::SerializeContext::DataPatchUpgrade::GetFieldCRC() const
    {
        return m_targetFieldCRC;
    }

    V::SerializeContext::DataPatchUpgradeType V::SerializeContext::DataPatchUpgrade::GetUpgradeType() const
    {
        return m_upgradeType;
    }

    V::SerializeContext::DataPatchUpgradeHandler::~DataPatchUpgradeHandler()
    {
        for (const auto& fieldUpgrades : m_upgrades)
        {
            for (const auto& versionUpgrades : fieldUpgrades.second)
            {
                for (auto* upgrade : versionUpgrades.second)
                {
                    delete upgrade;
                }
            }
        }
    }

    void V::SerializeContext::DataPatchUpgradeHandler::AddFieldUpgrade(DataPatchUpgrade* upgrade)
    {
        // Find the field
        auto fieldUpgrades = m_upgrades.find(upgrade->GetFieldCRC());

        // If we don't have any upgrades for the field, add this item.
        if (fieldUpgrades == m_upgrades.end())
        {
            m_upgrades[upgrade->GetFieldCRC()][upgrade->FromVersion()].insert(upgrade);
        }
        else
        {
            auto versionUpgrades = fieldUpgrades->second.find(upgrade->FromVersion());
            if (versionUpgrades == fieldUpgrades->second.end())
            {
                fieldUpgrades->second[upgrade->FromVersion()].insert(upgrade);
            }
            else
            {
                for (auto* existingUpgrade : versionUpgrades->second)
                {
                    if (*existingUpgrade == *upgrade)
                    {
                        V_Assert(false, "Duplicate upgrade to field %s from version %u to version %u", upgrade->GetFieldName().c_str(), upgrade->FromVersion(), upgrade->ToVersion());

                        // In a failure case, delete the upgrade as we've assumed control of it.
                        delete upgrade;
                        return;
                    }
                }

                m_upgrades[upgrade->GetFieldCRC()][upgrade->FromVersion()].insert(upgrade);
            }
        }
    }

    const V::SerializeContext::DataPatchFieldUpgrades& V::SerializeContext::DataPatchUpgradeHandler::GetUpgrades() const
    {
        return m_upgrades;
    }

    bool V::SerializeContext::DataPatchNameUpgrade::operator<(const DataPatchUpgrade& RHS) const
    {
        // If the right side is also a Field Name Upgrade, forward this to the
        // appropriate equivalence operator.
        return DataPatchUpgrade::operator<(RHS);
    }

    bool V::SerializeContext::DataPatchNameUpgrade::operator<(const DataPatchNameUpgrade& RHS) const
    {
        // The default operator is fine for name upgrades
        return DataPatchUpgrade::operator<(RHS);
    }

    void V::SerializeContext::DataPatchNameUpgrade::Apply(V::SerializeContext& context, SerializeContext::DataElementNode& node) const
    {
        V_UNUSED(context);

        int targetElementIndex = node.FindElement(m_targetFieldCRC);

        V_Assert(targetElementIndex >= 0, "Invalid node. Field %s is not a valid element of class %s (Version %u). Check your reflection function.", m_targetFieldName.c_str(), node.GetNameString(), node.GetVersion());

        if (targetElementIndex >= 0)
        {
            auto& targetElement = node.GetSubElement(targetElementIndex);
            targetElement.SetName(m_newNodeName.c_str());
        }
    }

    VStd::string V::SerializeContext::DataPatchNameUpgrade::GetNewName() const
    {
        return m_newNodeName;
    }

      //=========================================================================
    // BeginCloneElement (internal element clone callbacks)
    //=========================================================================
    bool SerializeContext::BeginCloneElement(void* ptr, const ClassData* classData, const ClassElement* elementData, void* data, ErrorHandler* errorHandler, VStd::vector<char>* scratchBuffer)
    {
        ObjectCloneData* cloneData = reinterpret_cast<ObjectCloneData*>(data);

        if (cloneData->ParentStack.empty())
        {
            // Since this is the root element, we will need to allocate it using the creator provided
            V_Assert(classData->Factory != nullptr, "We are attempting to create '%s', but no factory is provided! Either provide factory or change data member '%s' to value not pointer!", classData->Name, elementData->Name);
            cloneData->Ptr = classData->Factory->Create(classData->Name);
        }

        return BeginCloneElementInplace(cloneData->Ptr, ptr, classData, elementData, data, errorHandler, scratchBuffer);
    }

    //=========================================================================
    // BeginCloneElementInplace (internal element clone callbacks)
    //=========================================================================
    bool SerializeContext::BeginCloneElementInplace(void* rootDestPtr, void* ptr, const ClassData* classData, const ClassElement* elementData, void* data, ErrorHandler* errorHandler, VStd::vector<char>* scratchBuffer)
    {
        ObjectCloneData* cloneData = reinterpret_cast<ObjectCloneData*>(data);

        void* srcPtr = ptr;
        void* destPtr = nullptr;
        void* reservePtr = nullptr;

        if (classData->Version == VersionClassDeprecated)
        {
            if (classData->Converter)
            {
                V_Assert(false, "A deprecated element with a data converter was passed to CloneObject, this is not supported.");
            }
            // push a dummy node in the stack
            cloneData->ParentStack.push_back();
            ObjectCloneData::ParentInfo& parentInfo = cloneData->ParentStack.back();
            parentInfo.Ptr = destPtr;
            parentInfo.ReservePtr = reservePtr;
            parentInfo.ClassDataPtr = classData;
            parentInfo.ContainerIndexCounter = 0;
            return false;    // do not iterate further.
        }


        if (!cloneData->ParentStack.empty())
        {
            V_Assert(elementData, "Non-root nodes need to have a valid elementData!");
            ObjectCloneData::ParentInfo& parentInfo = cloneData->ParentStack.back();
            void* parentPtr = parentInfo.Ptr;
            const ClassData* parentClassData = parentInfo.ClassDataPtr;
            if (parentClassData->ContainerPtr)
            {
                if (parentClassData->ContainerPtr->CanAccessElementsByIndex() && parentClassData->ContainerPtr->Size(parentPtr) > parentInfo.ContainerIndexCounter)
                {
                    destPtr = parentClassData->ContainerPtr->GetElementByIndex(parentPtr, elementData, parentInfo.ContainerIndexCounter);
                }
                else
                {
                    destPtr = parentClassData->ContainerPtr->ReserveElement(parentPtr, elementData);
                }

                ++parentInfo.ContainerIndexCounter;
            }
            else
            {
                // Allocate memory for our element using the creator provided
                destPtr = reinterpret_cast<char*>(parentPtr) + elementData->Offset;
            }

            reservePtr = destPtr;
            if (elementData->Flags & ClassElement::FLG_POINTER)
            {
                V_Assert(classData->Factory != nullptr, "We are attempting to create '%s', but no factory is provided! Either provide a factory or change data member '%s' to value not pointer!", classData->Name, elementData->Name);
                void* newElement = classData->Factory->Create(classData->Name);
                void* basePtr = DownCast(newElement, classData->TypeId, elementData->TypeId, classData->VObjectRtti, elementData->VObjectRtti);
                *reinterpret_cast<void**>(destPtr) = basePtr; // store the pointer in the class
                destPtr = newElement;
            }

            if (!destPtr && errorHandler)
            {
                VStd::string error = VStd::string::format("Failed to reserve element in container. The container may be full. Element %u will not be added to container.", static_cast<unsigned int>(parentInfo.ContainerIndexCounter));
                errorHandler->ReportError(error.c_str());
            }
        }
        else
        {
            destPtr = rootDestPtr;
            reservePtr = rootDestPtr;
        }

        if (!destPtr)
        {
            // There is no valid destination pointer so a dummy node is added to the clone data parent stack
            // and further descendent type iteration is halted
            // An error has been reported to the supplied errorHandler in the code above
            cloneData->ParentStack.push_back();
            ObjectCloneData::ParentInfo& parentInfo = cloneData->ParentStack.back();
            parentInfo.Ptr = destPtr;
            parentInfo.ReservePtr = reservePtr;
            parentInfo.ClassDataPtr = classData;
            parentInfo.ContainerIndexCounter = 0;
            return false;
        }

        if (elementData && elementData->Flags & ClassElement::FLG_POINTER)
        {
            // if ptr is a pointer-to-pointer, cast its value to a void* (or const void*) and dereference to get to the actual object pointer.
            srcPtr = *(void**)(ptr);
            if (elementData->VObjectRtti)
            {
                srcPtr = elementData->VObjectRtti->Cast(srcPtr, classData->VObjectRtti->GetTypeId());
            }
        }

        if (classData->EventHandlerPtr)
        {
            classData->EventHandlerPtr->OnWriteBegin(destPtr);
        }

        if (classData->SerializerPtr)
        {
            if (classData->TypeId == GetAssetClassId())
            {
                // Optimized clone path for asset references.
                static_cast<AssetSerializer*>(classData->SerializerPtr.get())->Clone(srcPtr, destPtr);
            }
            else
            {
                scratchBuffer->clear();
                IO::ByteContainerStream<VStd::vector<char>> stream(scratchBuffer);

                classData->SerializerPtr->Save(srcPtr, stream);
                stream.Seek(0, IO::GenericStream::ST_SEEK_BEGIN);

                classData->SerializerPtr->Load(destPtr, stream, classData->Version);
            }
        }

        // If it is a container, clear it before loading the child
        // nodes, otherwise we end up with more elements than the ones
        // we should have
        if (classData->ContainerPtr)
        {
            classData->ContainerPtr->ClearElements(destPtr, this);
        }

        // push this node in the stack
        cloneData->ParentStack.push_back();
        ObjectCloneData::ParentInfo& parentInfo = cloneData->ParentStack.back();
        parentInfo.Ptr = destPtr;
        parentInfo.ReservePtr = reservePtr;
        parentInfo.ClassDataPtr = classData;
        parentInfo.ContainerIndexCounter = 0;
        return true;
    }

     //=========================================================================
    // EndCloneElement (internal element clone callbacks)
    //=========================================================================
    bool SerializeContext::EndCloneElement(void* data)
    {
        ObjectCloneData* cloneData = reinterpret_cast<ObjectCloneData*>(data);
        void* dataPtr = cloneData->ParentStack.back().Ptr;
        void* reservePtr = cloneData->ParentStack.back().ReservePtr;

        if (!dataPtr)
        {
            // we failed to clone an object - an assertion was already raised if it needed to be.
            cloneData->ParentStack.pop_back();
            return true; // continue on to siblings.
        }

        const ClassData* classData = cloneData->ParentStack.back().ClassDataPtr;
        if (classData->EventHandlerPtr)
        {
            classData->EventHandlerPtr->OnWriteEnd(dataPtr);
            classData->EventHandlerPtr->OnObjectCloned(dataPtr);
        }

        if (classData->SerializerPtr)
        {
            classData->SerializerPtr->PostClone(dataPtr);
        }

        cloneData->ParentStack.pop_back();
        if (!cloneData->ParentStack.empty())
        {
            const ClassData* parentClassData = cloneData->ParentStack.back().ClassDataPtr;
            if (parentClassData->ContainerPtr)
            {
                // Pass in the address returned by IDataContainer::ReserveElement.
                //VStdAssociativeContainer is the only DataContainer that uses the second argument passed into IDataContainer::StoreElement
                parentClassData->ContainerPtr->StoreElement(cloneData->ParentStack.back().Ptr, reservePtr);
            }
        }
        return true;
    }

    //=========================================================================
    // EnumerateDerived
    //=========================================================================
    void SerializeContext::EnumerateDerived(const TypeInfoCB& callback, const Uuid& classId, const Uuid& typeId) const
    {
        // right now this function is SLOW, traverses all serialized types. If we need faster
        // we will need to cache/store derived type in the base type.
        for (SerializeContext::IdToClassMap::const_iterator it = m_uuidMap.begin(); it != m_uuidMap.end(); ++it)
        {
            const ClassData& cd = it->second;

            if (cd.TypeId == classId)
            {
                continue;
            }

            if (cd.VObjectRtti && typeId != Uuid::CreateNull())
            {
                if (cd.VObjectRtti->IsTypeOf(typeId))
                {
                    if (!callback(&cd, nullptr))
                    {
                        return;
                    }
                }
            }

            if (!classId.IsNull())
            {
                for (size_t i = 0; i < cd.Elements.size(); ++i)
                {
                    if ((cd.Elements[i].Flags & ClassElement::FLG_BASE_CLASS) != 0)
                    {
                        if (cd.Elements[i].TypeId == classId)
                        {
                            // if both classes have azRtti they will be enumerated already by the code above (azrtti)
                            if (cd.VObjectRtti == nullptr || cd.Elements[i].VObjectRtti == nullptr)
                            {
                                if (!callback(&cd, nullptr))
                                {
                                    return;
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    //=========================================================================
    // EnumerateBase
    //=========================================================================
    void SerializeContext::EnumerateBase(const TypeInfoCB& callback, const Uuid& classId)
    {
        const ClassData* cd = FindClassData(classId);
        if (cd)
        {
            EnumerateBaseRTTIEnumCallbackData callbackData;
            callbackData.Callback = &callback;
            callbackData.ReportedTypes.push_back(cd->TypeId);
            for (size_t i = 0; i < cd->Elements.size(); ++i)
            {
                if ((cd->Elements[i].Flags & ClassElement::FLG_BASE_CLASS) != 0)
                {
                    const ClassData* baseClassData = FindClassData(cd->Elements[i].TypeId);
                    if (baseClassData)
                    {
                        callbackData.ReportedTypes.push_back(baseClassData->TypeId);
                        if (!callback(baseClassData, nullptr))
                        {
                            return;
                        }
                    }
                }
            }
            if (cd->VObjectRtti)
            {
                cd->VObjectRtti->EnumHierarchy(&SerializeContext::EnumerateBaseRTTIEnumCallback, &callbackData);
            }
        }
    }

    //=========================================================================
    // EnumerateAll
    //=========================================================================
    void SerializeContext::EnumerateAll(const TypeInfoCB& callback, bool includeGenerics) const
    {
        for (auto& uuidToClassPair : m_uuidMap)
        {
            const ClassData& classData = uuidToClassPair.second;
            if (!callback(&classData, classData.TypeId))
            {
                return;
            }
        }

        if (includeGenerics)
        {
            for (auto& uuidToGenericPair : m_uuidGenericMap)
            {
                const ClassData* classData = uuidToGenericPair.second->GetClassData();
                if (classData)
                {
                    if (!callback(classData, classData->TypeId))
                    {
                        return;
                    }
                }
            }
        }
    }

    void SerializeContext::RegisterDataContainer(VStd::unique_ptr<IDataContainer> dataContainer)
    {
        m_dataContainers.push_back(VStd::move(dataContainer));
    }

    //=========================================================================
    // EnumerateBaseRTTIEnumCallback
    //=========================================================================
    void SerializeContext::EnumerateBaseRTTIEnumCallback(const Uuid& id, void* userData)
    {
        EnumerateBaseRTTIEnumCallbackData* callbackData = reinterpret_cast<EnumerateBaseRTTIEnumCallbackData*>(userData);
        // if not reported, report
        if (VStd::find(callbackData->ReportedTypes.begin(), callbackData->ReportedTypes.end(), id) == callbackData->ReportedTypes.end())
        {
            (*callbackData->Callback)(nullptr, id);
        }
    }

     //=========================================================================
    // ClassData
    //=========================================================================
    SerializeContext::ClassData::ClassData()
    {
        Name = nullptr;
        TypeId = Uuid::CreateNull();
        Version = 0;
        Converter = nullptr;
        SerializerPtr = nullptr;
        Factory = nullptr;
        PersistentId = nullptr;
        DoSave = nullptr;
        EventHandlerPtr = nullptr;
        ContainerPtr = nullptr;
        VObjectRtti = nullptr;
        EditDataPtr = nullptr;
    }

     //=========================================================================
    // ClassData
    //=========================================================================
    void SerializeContext::ClassData::ClearAttributes()
    {
        Attributes.clear();

        for (ClassElement& classElement : Elements)
        {
            if (classElement.AttrOwnership == ClassElement::AttributeOwnership::Parent)
            {
                classElement.ClearAttributes();
            }
        }
    }

     SerializeContext::ClassPersistentId SerializeContext::ClassData::GetPersistentId(const SerializeContext& context) const
    {
        ClassPersistentId persistentIdFunction = PersistentId;
        if (!persistentIdFunction)
        {
            // check the base classes
            for (const SerializeContext::ClassElement& element : Elements)
            {
                if (element.Flags & ClassElement::FLG_BASE_CLASS)
                {
                    const SerializeContext::ClassData* baseClassData = context.FindClassData(element.TypeId);
                    if (baseClassData)
                    {
                        persistentIdFunction = baseClassData->GetPersistentId(context);
                        if (persistentIdFunction)
                        {
                            break;
                        }
                    }
                }
                else
                {
                    // base classes are in the beginning of the array
                    break;
                }
            }
        }
        return persistentIdFunction;
    }

    Attribute* SerializeContext::ClassData::FindAttribute(AttributeId attributeId) const
    {
        for (const V::AttributeSharedPair& attributePair : Attributes)
        {
            if (attributePair.first == attributeId)
            {
                return attributePair.second.get();
            }
        }
        return nullptr;
    }

    bool SerializeContext::ClassData::CanConvertFromType(const V::TypeId& convertibleTypeId, V::SerializeContext& serializeContext) const
    {
        // If the convertible type is exactly the type being stored by the ClassData.
        // True will always be returned in this case
        if (convertibleTypeId == TypeId)
        {
            return true;
        }

        return DataConverterPtr ? DataConverterPtr->CanConvertFromType(convertibleTypeId, *this, serializeContext) : false;
    }

    bool SerializeContext::ClassData::ConvertFromType(void*& convertibleTypePtr, const V::TypeId& convertibleTypeId, void* classPtr, V::SerializeContext& serializeContext) const
    {
        // If the convertible type is exactly the type being stored by the ClassData.
        // the result convertTypePtr is equal to the classPtr
        if (convertibleTypeId == TypeId)
        {
            convertibleTypePtr = classPtr;
            return true;
        }

        return DataConverterPtr ? DataConverterPtr->ConvertFromType(convertibleTypePtr, convertibleTypeId, classPtr, *this, serializeContext) : false;
    }

     //=========================================================================
    // ToString
    //=========================================================================
    void SerializeContext::DbgStackEntry::ToString(VStd::string& str) const
    {
        str += "[";
        if (ElementName)
        {
            str += VStd::string::format(" Element: '%s' of", ElementName);
        }
        //if( m_classElement )
        //  str += VStd::string::format(" Offset: %d",m_classElement->m_offset);
        if (ClassDataPtr)
        {
            str += VStd::string::format(" Class: '%s' Version: %d", ClassDataPtr->Name, ClassDataPtr->Version);
        }
        str += VStd::string::format(" Address: %p Uuid: %s", DataPtr, UuidPtr->ToString<VStd::string>().c_str());
        str += " ]\n";
    }
    
    //=========================================================================
    // FreeElementPointer
    //=========================================================================
    void SerializeContext::IDataContainer::DeletePointerData(SerializeContext* context, const ClassElement* classElement, const void* element)
    {
        V_Assert(context != nullptr && classElement != nullptr && element != nullptr, "Invalid input");
        const V::Uuid* elemUuid = &classElement->TypeId;
        // find the class data for the specific element
        const SerializeContext::ClassData* classData = classElement->GenericClassInfoPtr ? classElement->GenericClassInfoPtr->GetClassData() : context->FindClassData(*elemUuid, nullptr, 0);
        if (classElement->Flags & SerializeContext::ClassElement::FLG_POINTER)
        {
            const void* dataPtr = *reinterpret_cast<void* const*>(element);
            // if dataAddress is a pointer in this case, cast it's value to a void* (or const void*) and dereference to get to the actual class.
            if (dataPtr && classElement->VObjectRtti)
            {
                const V::Uuid* actualClassId = &classElement->VObjectRtti->GetActualUuid(dataPtr);
                if (*actualClassId != *elemUuid)
                {
                    // we are pointing to derived type, adjust class data, uuid and pointer.
                    classData = context->FindClassData(*actualClassId, nullptr, 0);
                    elemUuid = actualClassId;
                    if (classData)
                    {
                        dataPtr = classElement->VObjectRtti->Cast(dataPtr, classData->VObjectRtti->GetTypeId());
                    }
                }
            }
        }
        if (classData == nullptr)
        {
            if ((classElement->Flags & ClassElement::FLG_POINTER) != 0)
            {
                const void* dataPtr = *reinterpret_cast<void* const*>(element);
                V_UNUSED(dataPtr); // this prevents a L4 warning if the below line is stripped out in release.
                V_Warning("Serialization", false, "Failed to find class id%s for %p! Memory could leak.", elemUuid->ToString<VStd::string>().c_str(), dataPtr);
            }
            return;
        }

        if (classData->ContainerPtr)  // if element is container forward the message
        {
            // clear all container data
            classData->ContainerPtr->ClearElements(const_cast<void*>(element), context);
        }
        else
        {
            if ((classElement->Flags & ClassElement::FLG_POINTER) == 0)
            {
                return; // element is stored by value nothing to free
            }

            // if we get here, its a FLG_POINTER
            const void* dataPtr = *reinterpret_cast<void* const*>(element);
            if (classData->Factory)
            {
                classData->Factory->Destroy(dataPtr);
            }
            else
            {
                V_Warning("Serialization", false, "Failed to delete %p '%s' element, no destructor is provided! Memory could leak.", dataPtr, classData->Name);
            }
        }
    }


     //=========================================================================
    // RemoveClassData
    //=========================================================================
    void SerializeContext::RemoveClassData(ClassData* classData)
    {
        if (m_editContext)
        {
            m_editContext->RemoveClassData(classData);
        }
    }

    void SerializeContext::RemoveGenericClassInfo(GenericClassInfo* genericClassInfo)
    {
        const Uuid& classId = genericClassInfo->GetSpecializedTypeId();
        RemoveClassData(genericClassInfo->GetClassData());
        // Find the module GenericClassInfo in the SerializeContext GenericClassInfo multimap and remove it from there
        auto scGenericClassInfoRange = m_uuidGenericMap.equal_range(classId);
        auto scGenericInfoFoundIt = VStd::find_if(scGenericClassInfoRange.first, scGenericClassInfoRange.second, [genericClassInfo](const VStd::pair<V::Uuid, GenericClassInfo*>& genericPair)
        {
            return genericClassInfo == genericPair.second;
        });

        if (scGenericInfoFoundIt != scGenericClassInfoRange.second)
        {
            m_uuidGenericMap.erase(scGenericInfoFoundIt);
            if (m_uuidGenericMap.count(classId) == 0)
            {
                m_uuidAnyCreationMap.erase(classId);
                auto classNameRange = m_classNameToUuid.equal_range(Crc32(genericClassInfo->GetClassData()->Name));
                for (auto classNameRangeIter = classNameRange.first; classNameRangeIter != classNameRange.second;)
                {
                    if (classNameRangeIter->second == classId)
                    {
                        classNameRangeIter = m_classNameToUuid.erase(classNameRangeIter);
                    }
                    else
                    {
                        ++classNameRangeIter;
                    }
                }

                auto legacyTypeIdRangeIt = m_legacySpecializeTypeIdToTypeIdMap.equal_range(genericClassInfo->GetLegacySpecializedTypeId());
                for (auto legacySpecializedTypeIdIt = legacyTypeIdRangeIt.first; legacySpecializedTypeIdIt != legacyTypeIdRangeIt.second; ++legacySpecializedTypeIdIt)
                {
                    if (classId == legacySpecializedTypeIdIt->second)
                    {
                        m_legacySpecializeTypeIdToTypeIdMap.erase(classId);
                        break;
                    }
                }
            }
        }
    }

    //=========================================================================
    // GetStackDescription
    //=========================================================================
    VStd::string SerializeContext::ErrorHandler::GetStackDescription() const
    {
        VStd::string stackDescription;

    #ifdef V_ENABLE_SERIALIZER_DEBUG
        if (!m_stack.empty())
        {
            stackDescription += "\n=== Serialize stack ===\n";
            for (size_t i = 0; i < m_stack.size(); ++i)
            {
                m_stack[i].ToString(stackDescription);
            }
            stackDescription += "\n";
        }
    #endif // V_ENABLE_SERIALIZER_DEBUG

        return stackDescription;
    }

    //=========================================================================
    // ReportError
    //=========================================================================
    void SerializeContext::ErrorHandler::ReportError(const char* message)
    {
        (void)message;
        V_Error("Serialize", false, "%s\n%s", message, GetStackDescription().c_str());
        m_nErrors++;
    }

    //=========================================================================
    // ReportWarning
    //=========================================================================
    void SerializeContext::ErrorHandler::ReportWarning(const char* message)
    {
        (void)message;
        V_Warning("Serialize", false, "%s\n%s", message, GetStackDescription().c_str());
        m_nWarnings++;
    }

    //=========================================================================
    // Push
    //=========================================================================
    void SerializeContext::ErrorHandler::Push(const DbgStackEntry& de)
    {
        (void)de;
    #ifdef AZ_ENABLE_SERIALIZER_DEBUG
        m_stack.push_back((de));
    #endif // AZ_ENABLE_SERIALIZER_DEBUG
    }

    //=========================================================================
    // Pop
    //=========================================================================
    void SerializeContext::ErrorHandler::Pop()
    {
    #ifdef V_ENABLE_SERIALIZER_DEBUG
        m_stack.pop_back();
    #endif // V_ENABLE_SERIALIZER_DEBUG
    }

    //=========================================================================
    // Reset
    //=========================================================================
    void SerializeContext::ErrorHandler::Reset()
    {
    #ifdef V_ENABLE_SERIALIZER_DEBUG
        m_stack.clear();
    #endif // V_ENABLE_SERIALIZER_DEBUG
        m_nErrors = 0;
    }

       //=========================================================================
    // EnumerateInstanceCallContext
    //=========================================================================

    SerializeContext::EnumerateInstanceCallContext::EnumerateInstanceCallContext(
        const SerializeContext::BeginElemEnumCB& beginElemCB,
        const SerializeContext::EndElemEnumCB& endElemCB,
        const SerializeContext* context,
        unsigned int accessFlags,
        SerializeContext::ErrorHandler* errorHandler)
        : BeginElemCB(beginElemCB)
        , EndElemCB(endElemCB)
        , AccessFlags(accessFlags)
        , Context(context)
    {
        ErrorHandlerPtr = errorHandler ? errorHandler : &DefaultErrorHandler;

        ElementCallback = VStd::bind(static_cast<bool (SerializeContext::*)(EnumerateInstanceCallContext*, void*, const Uuid&, const ClassData*, const ClassElement*) const>(&SerializeContext::EnumerateInstance)
            , Context
            , this
            , VStd::placeholders::_1
            , VStd::placeholders::_2
            , VStd::placeholders::_3
            , VStd::placeholders::_4
        );
    }

    //=========================================================================
    // ~ClassElement
    //=========================================================================
    SerializeContext::ClassElement::~ClassElement()
    {
        if (AttrOwnership == AttributeOwnership::Self)
        {
            ClearAttributes();
        }
    }

    //=========================================================================
    // ClassElement::operator=
    //=========================================================================
    SerializeContext::ClassElement& SerializeContext::ClassElement::operator=(const SerializeContext::ClassElement& other)
    {
        Name = other.Name;
        NameCrc = other.NameCrc;
        TypeId = other.TypeId;
        DataSize = other.DataSize;
        Offset = other.Offset;

        VObjectRtti = other.VObjectRtti;
        GenericClassInfoPtr = other.GenericClassInfoPtr;
        EditDataPtr = other.EditDataPtr;
        Attributes  = Attributes;
        // If we're a copy, we don't assume attribute ownership
        AttrOwnership = AttributeOwnership::None;
        Flags = other.Flags;

        return *this;
    }

    //=========================================================================
    // ClearAttributes
    //=========================================================================
    void SerializeContext::ClassElement::ClearAttributes()
    {
        Attributes.clear();
    }

    //=========================================================================
    // FindAttribute
    //=========================================================================

    Attribute* SerializeContext::ClassElement::FindAttribute(AttributeId attributeId) const
    {
        for (const AttributeSharedPair& attributePair : Attributes)
        {
            if (attributePair.first == attributeId)
            {
                return attributePair.second.get();
            }
        }
        return nullptr;
    }

    void SerializeContext::IDataContainer::ElementsUpdated(void* instance)
    {
        (void)instance;
    }

    void Internal::VStdArrayEvents::OnWriteBegin(void* classPtr)
    {
        (void)classPtr;
        if (m_indices)
        {
            if ((reinterpret_cast<uintptr_t>(m_indices) & 1) == 1)
            {
                // Pointer is already in use to store an index so convert it to a stack
                size_t previousIndex = reinterpret_cast<uintptr_t>(m_indices) >> 1;
                Stack* stack = new Stack();
                V_Assert((reinterpret_cast<uintptr_t>(stack) & 1) == 0, "Expected memory allocation to be at least 2 byte aligned.");
                stack->push(previousIndex);
                stack->push(0);
                m_indices = stack;
            }
            else
            {
                Stack* stack = reinterpret_cast<Stack*>(m_indices);
                stack->push(0);
            }
        }
        else
        {
            // Use the pointer to just store the one counter instead of allocating memory. Using 1 bit to identify this as a regular
            // index and not a pointer.
            m_indices = reinterpret_cast<void*>(1);
        }
    }

     void Internal::VStdArrayEvents::OnWriteEnd(void* classPtr)
    {
        (void)classPtr;
        if (m_indices)
        {
            if ((reinterpret_cast<uintptr_t>(m_indices) & 1) == 1)
            {
                // There was only one entry so no stack. Clear out the final bit that indicated this was an index and not a pointer.
                m_indices = nullptr;
            }
            else
            {
                Stack* stack = reinterpret_cast<Stack*>(m_indices);
                stack->pop();
                if (stack->empty())
                {
                    delete stack;
                    m_indices = nullptr;
                }
            }
        }
        else
        {
            V_Warning("Serialization", false, "VStdArrayEvents::OnWriteEnd called too often.");
        }

    }

    size_t Internal::VStdArrayEvents::GetIndex() const
    {
        if (m_indices)
        {
            if ((reinterpret_cast<uintptr_t>(m_indices) & 1) == 1)
            {
                // The first bit is used to indicate this is a regular index instead of a pointer so shift down one to get the actual index.
                return reinterpret_cast<uintptr_t>(m_indices) >> 1;
            }
            else
            {
                const Stack* stack = reinterpret_cast<const Stack*>(m_indices);
                return stack->top();
            }
        }
        else
        {
            V_Warning("Serialization", false, "VStdArrayEvents is not in a valid state to return an index.");
            return 0;
        }
    }

    void Internal::VStdArrayEvents::Increment()
    {
        if (m_indices)
        {
            if ((reinterpret_cast<uintptr_t>(m_indices) & 1) == 1)
            {
                // Increment by 2 because the first bit is used to indicate whether or not a stack is used so the real
                //      value starts one bit later.
                size_t index = reinterpret_cast<uintptr_t>(m_indices) + (1 << 1);
                m_indices = reinterpret_cast<void*>(index);
            }
            else
            {
                Stack* stack = reinterpret_cast<Stack*>(m_indices);
                stack->top()++;
            }
        }
        else
        {
            V_Warning("Serialization", false, "VStdArrayEvents is not in a valid state to increment.");
        }
    }

    void Internal::VStdArrayEvents::Decrement()
    {
        if (m_indices)
        {
            if ((reinterpret_cast<uintptr_t>(m_indices) & 1) == 1)
            {
                // Decrement by 2 because the first bit is used to indicate whether or not a stack is used so the real
                //      value starts one bit later. This assumes that index is check to be larger than 0 before calling
                //      this function.
                size_t index = reinterpret_cast<uintptr_t>(m_indices) - (1 << 1);
                m_indices = reinterpret_cast<void*>(index);
            }
            else
            {
                Stack* stack = reinterpret_cast<Stack*>(m_indices);
                stack->top()--;
            }
        }
        else
        {
            V_Warning("Serialization", false, "VStdArrayEvents is not in a valid state to decrement.");
        }
    }

    bool Internal::VStdArrayEvents::IsEmpty() const
    {
        return m_indices == nullptr;
    }

    bool SerializeContext::IsTypeReflected(V::Uuid typeId) const
    {
        const V::SerializeContext::ClassData* reflectedClassData = FindClassData(typeId);
        return (reflectedClassData != nullptr);
    }

    // Create the member OSAllocator and construct the unordered_map with that allocator
    SerializeContext::PerModuleGenericClassInfo::PerModuleGenericClassInfo()
        : m_moduleLocalGenericClassInfos(V::VStdIAllocator(&m_moduleOSAllocator))
        , m_serializeContextSet(V::VStdIAllocator(&m_moduleOSAllocator))
    {
    }

    SerializeContext::PerModuleGenericClassInfo::~PerModuleGenericClassInfo()
    {
        Cleanup();

        // Reconstructs the module generic info map with the OSAllocator so that it the previous allocated memory is cleared
        // Afterwards destroy the OSAllocator
        {
            m_moduleLocalGenericClassInfos = GenericInfoModuleMap(V::VStdIAllocator(&m_moduleOSAllocator));
            m_serializeContextSet = SerializeContextSet(V::VStdIAllocator(&m_moduleOSAllocator));
        }
    }

    void SerializeContext::PerModuleGenericClassInfo::Cleanup()
    {
        decltype(m_moduleLocalGenericClassInfos) genericClassInfoContainer = VStd::move(m_moduleLocalGenericClassInfos);
        decltype(m_serializeContextSet) serializeContextSet = VStd::move(m_serializeContextSet);
        // Un-reflect GenericClassInfo from each serialize context registered with the module
        // The SerailizeContext uses the SystemAllocator so it is required to be ready in order to remove the data
        if (V::AllocatorInstance<V::SystemAllocator>::IsReady())
        {
            for (V::SerializeContext* serializeContext : serializeContextSet)
            {
                for (const VStd::pair<V::Uuid, V::GenericClassInfo*>& moduleGenericClassInfoPair : genericClassInfoContainer)
                {
                    serializeContext->RemoveGenericClassInfo(moduleGenericClassInfoPair.second);
                }

                serializeContext->m_perModuleSet.erase(this);
            }
        }

        // Cleanup the memory for the GenericClassInfo objects.
        // This isn't explicitly needed as the OSAllocator owned by this class will take the memory with it.
        for (const VStd::pair<V::Uuid, V::GenericClassInfo*>& moduleGenericClassInfoPair : genericClassInfoContainer)
        {
            GenericClassInfo* genericClassInfo = moduleGenericClassInfoPair.second;
            // Explicitly invoke the destructor and clear the memory from the module OSAllocator
            genericClassInfo->~GenericClassInfo();
            m_moduleOSAllocator.DeAllocate(genericClassInfo);
        }
    }

    void SerializeContext::PerModuleGenericClassInfo::RegisterSerializeContext(V::SerializeContext* serializeContext)
    {
        m_serializeContextSet.emplace(serializeContext);
        serializeContext->m_perModuleSet.emplace(this);
    }

    void SerializeContext::PerModuleGenericClassInfo::UnregisterSerializeContext(V::SerializeContext* serializeContext)
    {
        m_serializeContextSet.erase(serializeContext);
        serializeContext->m_perModuleSet.erase(this);
        for (const VStd::pair<V::Uuid, V::GenericClassInfo*>& moduleGenericClassInfoPair : m_moduleLocalGenericClassInfos)
        {
            serializeContext->RemoveGenericClassInfo(moduleGenericClassInfoPair.second);
        }
    }

    void SerializeContext::PerModuleGenericClassInfo::AddGenericClassInfo(V::GenericClassInfo* genericClassInfo)
    {
        if (!genericClassInfo)
        {
            V_Error("SerializeContext", false, "The supplied generic class info object is nullptr. It cannot be added to the SerializeContext module structure");
            return;
        }

        m_moduleLocalGenericClassInfos.emplace(genericClassInfo->GetSpecializedTypeId(), genericClassInfo);
    }

    void SerializeContext::PerModuleGenericClassInfo::RemoveGenericClassInfo(const V::TypeId& genericTypeId)
    {
        if (genericTypeId.IsNull())
        {
            V_Error("SerializeContext", false, "The supplied generic typeidis invalid. It is not stored the SerializeContext module structure ");
            return;
        }

        auto genericClassInfoFoundIt = m_moduleLocalGenericClassInfos.find(genericTypeId);
        if (genericClassInfoFoundIt != m_moduleLocalGenericClassInfos.end())
        {
            m_moduleLocalGenericClassInfos.erase(genericClassInfoFoundIt);
        }
    }

    V::GenericClassInfo* SerializeContext::PerModuleGenericClassInfo::FindGenericClassInfo(const V::TypeId& genericTypeId) const
    {
        auto genericClassInfoFoundIt = m_moduleLocalGenericClassInfos.find(genericTypeId);
        return genericClassInfoFoundIt != m_moduleLocalGenericClassInfos.end() ? genericClassInfoFoundIt->second : nullptr;
    }

    V::IAllocatorAllocate& SerializeContext::PerModuleGenericClassInfo::GetAllocator()
    {
        return m_moduleOSAllocator;
    }

    // Take advantage of static variables being unique per dll module to clean up module specific registered classes when the module unloads
    SerializeContext::PerModuleGenericClassInfo& GetCurrentSerializeContextModule()
    {
        static SerializeContext::PerModuleGenericClassInfo s_ModuleCleanupInstance;
        return s_ModuleCleanupInstance;
    }

    SerializeContext::IDataSerializerDeleter SerializeContext::IDataSerializer::CreateDefaultDeleteDeleter()
    {
        return [](IDataSerializer* ptr)
        {
            delete ptr;
        };
    }
    SerializeContext::IDataSerializerDeleter SerializeContext::IDataSerializer::CreateNoDeleteDeleter()
    {
        return [](IDataSerializer*)
        {
        };
    }
} // namespace V

