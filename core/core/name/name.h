#ifndef V_FRAMEWORK_CORE_NAME_NAME_H
#define V_FRAMEWORK_CORE_NAME_NAME_H

#include <core/name/internal/name_data.h>


namespace V
{
    class NameDictionary;

    //! The Name class provides very fast string equality comparison, so that names can be used as IDs without sacrificing performance.
    //! It is a smart pointer to a NameData held in a NameDictionary, where names are tracked, de-duplicated, and ref-counted.
    //!
    //! Creating a Name object with a value that doesn't exist in the dictionary is very slow.
    //! Creating a Name object with a value that already exists in the dictionary is similar to creating VStd::string.
    //! Copy-constructing a Name object is very fast.
    //! Equality-comparison of two Name objects is very fast.
    //!
    //! The dictionary must be initialized before Name objects are created.
    //! A Name instance must not be statically declared.
    class Name
    {
        friend NameDictionary;
    public:
        using Hash = Internal::NameData::Hash;

        V_CLASS_ALLOCATOR(Name, V::SystemAllocator, 0);

        Name();
        Name(const Name& name);
        Name(Name&& name);
        Name& operator=(const Name&);
        Name& operator=(Name&&);


        //! Creates an instance of a name from a string. 
        //! The name string is used as a key to lookup an entry in the dictionary, and is not 
        //! internally held after the call.
        explicit Name(VStd::string_view name);

        //! Creates an instance of a name from a hash.
        //! The hash will be used to find an existing name in the dictionary. If there is no
        //! name with this hash, the resulting name will be empty.
        explicit Name(Hash hash);
        
        //! Assigns a new name.  
        //! The name string is used as a key to lookup an entry in the dictionary, and is not 
        //! internally held after the call.
        Name& operator=(VStd::string_view name);

        //! Returns the name's string value.
        //! This is always null-terminated.
        //! This will always point to a string in memory (i.e. it will return "" instead of null).
        VStd::string_view GetStringView() const;
        const char* GetCStr() const;

        bool IsEmpty() const;

        bool operator==(const Name& other) const
        {
            return GetHash() == other.GetHash();
        }

        bool operator!=(const Name& other) const
        {
            return GetHash() != other.GetHash();
        }

        // We delete these operators because using Name in ordered containers is not supported. 
        // The point of Name is for fast equality comparison and lookup. Unordered containers should be used instead.
        friend bool operator<(const Name& lhs, const Name& rhs) = delete;
        friend bool operator<=(const Name& lhs, const Name& rhs) = delete;
        friend bool operator>(const Name& lhs, const Name& rhs) = delete;
        friend bool operator>=(const Name& lhs, const Name& rhs) = delete;
        
        //! Returns the string's hash that is used as the key in the NameDictionary.
        Hash GetHash() const
        {
            return m_hash;
        }

    private:
        
        // Assigns a new name.  
        // The name string is used as a key to lookup an entry in the dictionary, and is not 
        // internally held after the call.
        // This is needed for reflection into behavior context.
        void SetName(VStd::string_view name);
        
        void SetEmptyString();
        
        // This constructor is used by NameDictionary to construct from a dictionary-held NameData instance.
        Name(Internal::NameData* nameData);
    private:
        VStd::string_view m_view;
        Hash m_hash;
        //! Pointer to NameData in the NameDictionary. This holds both the hash and string pair.
        VStd::intrusive_ptr<Internal::NameData> m_data;
    };

} // namespace V

namespace VStd
{
    template <typename T>
    struct hash;

    // hashing support for STL containers
    template <>
    struct hash<V::Name>
    {
        V::Name::Hash operator()(const V::Name& value) const
        {
            return value.GetHash();
        }
    };
}

#endif // V_FRAMEWORK_CORE_NAME_NAME_H