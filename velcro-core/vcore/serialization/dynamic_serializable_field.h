#ifndef V_FRAMEWORK_CORE_SERIALIZATION_DYNAMIC_SERIALIZABLE_FIELD_H
#define V_FRAMEWORK_CORE_SERIALIZATION_DYNAMIC_SERIALIZABLE_FIELD_H

#include <vcore/vobject/type.h>
#include <vcore/memory/memory.h>
#include <vcore/memory/system_allocator.h>

namespace V
{
    struct Uuid;
    class SerializeContext;

     // 允许用户使用 void* 和 Uuid 对序列化数据.
     // 它当前由脚本组件使用. 我们可以将其用于其
     // 他目的, 但我们可能必须使其更加强大和用户友好.
    class DynamicSerializableField
    {
    public:
        V_CLASS_ALLOCATOR(DynamicSerializableField, V::SystemAllocator, 0);
        VOBJECT(DynamicSerializableField, "{617b8e0a-55f4-4bb2-b8ec-360c64dcf41f}")
        

        DynamicSerializableField();
        DynamicSerializableField(const DynamicSerializableField& serializableField);
        ~DynamicSerializableField();

        bool IsValid() const;
        void DestroyData(SerializeContext* useContext = nullptr);
        void* CloneData(SerializeContext* useContext = nullptr) const;

        void CopyDataFrom(const DynamicSerializableField& other, SerializeContext* useContext = nullptr);
        bool IsEqualTo(const DynamicSerializableField& other, SerializeContext* useContext = nullptr) const;

        template<class T>
        void Set(T* object)
        {
            DataPtr = object;
            TypeId = VObject<T>::Id();
        }

        // Keep in mind that this function will not do rtti_casts to T, you will need to
        // an instance of serialize context for that.
        template<class T>
        T* Get() const
        {
            if (TypeId == VObject<T>::Id())
            {
                return reinterpret_cast<T*>(DataPtr);
            }
            return nullptr;
        }
    
        void* DataPtr;
        Uuid  TypeId;
    };
}   // namespace V

#endif // V_FRAMEWORK_CORE_SERIALIZATION_DYNAMIC_SERIALIZABLE_FIELD_H