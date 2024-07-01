#ifndef V_FRAMEWORK_CORE_VOBJECT_ATTRIBUTE_READER_H
#define V_FRAMEWORK_CORE_VOBJECT_ATTRIBUTE_READER_H

#include <vcore/vobject/reflect_context.h>
#include <vcore/std/string/string.h>

namespace V {
    namespace Internal {
        template<class AttrType, class DestType, class InstType, typename ... Args>
        bool AttributeRead(DestType& value, Attribute* attr, InstType&& instance, Args&& ... args)
        {
            // try a value
            
            if (auto data = vobject_dynamic_cast<AttributeData<AttrType>*>(attr); data != nullptr)
            {
                value = static_cast<DestType>(data->Get(instance));
                return true;
            }
            // try a function with return type
            if (auto func = vobject_rtti_cast<AttributeFunction<AttrType(Args...)>*>(attr); func != nullptr)
            {
                value = static_cast<DestType>(func->Invoke(VStd::forward<InstType>(instance), VStd::forward<Args>(args)...));
                return true;
            }
            // try a type with an invocable function
            if (auto invocable = azrtti_cast<AttributeInvocable<AttrType(InstType, Args...)>*>(attr); invocable != nullptr)
            {
                value = static_cast<DestType>(invocable->operator()(VStd::forward<InstType>(instance), VStd::forward<Args>(args)...));
                return true;
            }
            // else you are on your own!
            return false;
        }

        template<class RetType, class InstType, typename ... Args>
        bool AttributeInvoke(Attribute* attr, InstType&& instance, Args&& ... args)
        {
            // try a function 
            if (auto func = vobject_rtti_cast<AttributeFunction<RetType(Args...)>*>(attr); func != nullptr)
            {
                func->Invoke(VStd::forward<InstType>(instance), VStd::forward<Args>(args)...);
                return true;
            }
            // try a type with an invocable function
            if (auto invocable = vobject_rtti_cast<AttributeInvocable<RetType(InstType, Args...)>*>(attr); invocable != nullptr)
            {
                invocable->operator()(VStd::forward<InstType>(instance), VStd::forward<Args>(args)...);
                return true;
            }
            return false;
        }

          /**
        * Integral types (except bool) can be converted char <-> short <-> int <-> int64
        * for bool it requires exact matching (as comparing to zero is ambiguous)
        * The same applied for floating point types,can convert float <-> double, but conversion to integers is not allowed
        */
        template<class AttrType, class DestType, typename ... Args>
        struct AttributeReader
        {
            template<typename InstType>
            static bool Read(DestType& value, Attribute* attr, InstType&& instance, const Args& ... args)
            {
                if constexpr (VStd::is_integral_v<DestType> && !VStd::is_same_v<DestType, bool>)
                {
                    if (AttributeRead<bool, DestType, Args...>(value, attr, instance, args ...))
                    {
                        return true;
                    }
                    if (AttributeRead<char, DestType, Args...>(value, attr, instance, args ...))
                    {
                        return true;
                    }
                    if (AttributeRead<unsigned char, DestType, Args...>(value, attr, instance, args ...))
                    {
                        return true;
                    }
                    if (AttributeRead<short, DestType, Args...>(value, attr, instance, args ...))
                    {
                        return true;
                    }
                    if (AttributeRead<unsigned short, DestType, Args...>(value, attr, instance, args ...))
                    {
                        return true;
                    }
                    if (AttributeRead<int, DestType, Args...>(value, attr, instance, args ...))
                    {
                        return true;
                    }
                    if (AttributeRead<unsigned int, Args..., DestType>(value, attr, instance, args ...))
                    {
                        return true;
                    }
                    if (AttributeRead<V::s64, DestType, Args...>(value, attr, instance, args ...))
                    {
                        return true;
                    }
                    if (AttributeRead<V::u64, DestType, Args...>(value, attr, instance, args ...))
                    {
                        return true;
                    }
                    if (AttributeRead<V::Crc32, DestType, Args...>(value, attr, instance, args ...))
                    {
                        return true;
                    }
                }
                if constexpr (VStd::is_floating_point_v<DestType>)
                {
                    if (AttributeRead<float, DestType>(value, attr, instance, args ...))
                    {
                        return true;
                    }
                    if (AttributeRead<double, DestType>(value, attr, instance, args ...))
                    {
                        return true;
                    }
                }
                if constexpr (VStd::is_same_v<DestType, VStd::string> && (VStd::is_same_v<AttrType, VStd::string>
                    || VStd::is_same_v<AttrType, VStd::string_view>
                    || VStd::is_same_v<AttrType, const char*>))
                {
                    if (AttributeRead<const char*, VStd::string>(value, attr, instance, args...))
                    {
                        return true;
                    }
                    if (AttributeRead<VStd::string_view, VStd::string>(value, attr, instance, args...))
                    {
                        return true;
                    }
                }
                return AttributeRead<AttrType, DestType>(value, attr, instance, args...);
            }
        };
    } // namespace Internal
} // namespace V

#endif // V_FRAMEWORK_CORE_VOBJECT_ATTRIBUTE_READER_H