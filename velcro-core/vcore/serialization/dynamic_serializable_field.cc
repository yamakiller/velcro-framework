#include <vcore/serialization/dynamic_serializable_field.h>


#include <vcore/io/byte_container_stream.h>
#include <vcore/serialization/serialization_context.h>
//#include <vcore/serialization/object_stream.h>
//#include <vcore/serialization/utils.h>
#include <vcore/std/containers/vector.h>

namespace V
{
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    DynamicSerializableField::DynamicSerializableField()
        : DataPtr(nullptr)
        , TypeId(Uuid::CreateNull())        
    {
    }
    //-------------------------------------------------------------------------
    DynamicSerializableField::DynamicSerializableField(const DynamicSerializableField& serializableField)
    {
        DataPtr = serializableField.CloneData();
        TypeId  = serializableField.TypeId;
    }
    //-------------------------------------------------------------------------
    DynamicSerializableField::~DynamicSerializableField()
    {
        // Should we call DestroyData() here?
    }
    //-------------------------------------------------------------------------
    bool DynamicSerializableField::IsValid() const
    {
        return DataPtr && !TypeId.IsNull();
    }
    //-------------------------------------------------------------------------
    void DynamicSerializableField::DestroyData(SerializeContext* useContext)
    {
        if (IsValid())
        {
            // Right now we rely on the fact that destructor info has to be reflected in the default
            // SerializeContext held by the ComponentApplication.
            // If we ever need to have multiple contexts, we will have to add a way to refer to the context
            // that created our data and use that to query for the proper destructor info.
            if (!useContext)
            {
                //EBUS_EVENT_RESULT(useContext, ComponentApplicationBus, GetSerializeContext);
                V_Error("DynamicSerializableField", useContext, "Can't find valid serialize context. Dynamic data cannot be deleted without it!");
            }
            if (useContext)
            {
                const SerializeContext::ClassData* classData = useContext->FindClassData(TypeId);
                if (classData)
                {
                    if (classData->Factory)
                    {
                        classData->Factory->Destroy(DataPtr);
                        DataPtr = nullptr;
                        TypeId = Uuid::CreateNull();                        
                        return;
                    }
                    else
                    {
                        V_Error("DynamicSerializableField", false, "Class data for type Uuid %s does not contain a valid destructor. Dynamic data cannot be deleted without it!", TypeId.ToString<VStd::string>().c_str());
                    }
                }
                else
                {
                    V_Error("DynamicSerializableField", false, "Can't find class data for type Uuid %s. Dynamic data cannot be deleted without it!", TypeId.ToString<VStd::string>().c_str());
                }
            }
        }
    }
    //-------------------------------------------------------------------------
    void* DynamicSerializableField::CloneData(SerializeContext* useContext) const
    {
        if (IsValid())
        {
            if (!useContext)
            {
                //EBUS_EVENT_RESULT(useContext, ComponentApplicationBus, GetSerializeContext);
                V_Error("DynamicSerializableField", useContext, "Can't find valid serialize context. Dynamic data cannot be deleted without it!");
            }
            if (useContext)
            {
                void* clonedData = useContext->CloneObject(DataPtr, TypeId);
                return clonedData;
            }
        }
        return nullptr;
    }
    //-------------------------------------------------------------------------
    void DynamicSerializableField::CopyDataFrom(const DynamicSerializableField& other, SerializeContext* useContext)
    {
        DestroyData();
        TypeId    = other.TypeId;        
        DataPtr   = other.CloneData(useContext);
    }
    //-------------------------------------------------------------------------
    bool DynamicSerializableField::IsEqualTo(const DynamicSerializableField& other, SerializeContext* useContext) const
    {
        if (other.TypeId != TypeId)
        {
            return false;
        }

        bool isEqual = false;

        if (!useContext)
        {
            //EBUS_EVENT_RESULT(useContext, ComponentApplicationBus, GetSerializeContext);
            V_Error("DynamicSerializableField", useContext, "Can't find valid serialize context. Dynamic data cannot be compared without it!");
        }        

        if (useContext)
        {
            const SerializeContext::ClassData* classData = useContext->FindClassData(TypeId);
            if (classData)
            {
                if (classData->SerializerPtr)
                {
                    isEqual = classData->SerializerPtr->CompareValueData(DataPtr, other.DataPtr);
                }
                else
                {
                    VStd::vector<V::u8> myData;
                    V::IO::ByteContainerStream<decltype(myData)> myDataStream(&myData);

                    //Utils::SaveObjectToStream(myDataStream, V::ObjectStream::ST_BINARY, DataPtr, TypeId);

                    VStd::vector<V::u8> otherData;
                    V::IO::ByteContainerStream<decltype(otherData)> otherDataStream(&otherData);

                    //Utils::SaveObjectToStream(otherDataStream, V::ObjectStream::ST_BINARY, other.DataPtr, other.TypeId);
                    isEqual = (myData.size() == otherData.size()) && (memcmp(myData.data(), otherData.data(), myData.size()) == 0);
                }
            }
            else
            {
                isEqual = DataPtr == nullptr && other.DataPtr == nullptr;
                V_Error("DynamicSerializableField", DataPtr == nullptr, "Trying to compare unknown data types(%i).", TypeId);
            }
        }

        return isEqual;
    }
    
}   // namespace V
