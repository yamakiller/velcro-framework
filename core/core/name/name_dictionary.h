#ifndef V_FRAMEWORK_CORE_NAME_NAME_DICTIONARY_H
#define V_FRAMEWORK_CORE_NAME_NAME_DICTIONARY_H


#include <core/std/containers/unordered_map.h>
#include <core/std/string/string.h>
#include <core/std/string/string_view.h>
#include <core/std/parallel/shared_mutex.h>
#include <core/memory/memory.h>
#include <core/memory/osallocator.h>
#include <core/name/name.h>



namespace V {
    class Module;

    namespace Internal
    {
        class NameData;
    };
    
    //! Maintains a list of unique strings for Name objects.
    //! The main benefit of the Name system is very fast string equality comparison, because every
    //! unique name has a unique ID. The NameDictionary's purpose is to guarantee name IDs do not 
    //! collide. It also saves memory by removing duplicate strings.
    //!
    //! Benchmarks have shown that creating a new Name object can be quite slow when the name doesn't 
    //! already exist in the NameDictionary, but is comparable to creating an VStd::string for names 
    //! that already exist.
    class NameDictionary final
    {
        V_CLASS_ALLOCATOR(NameDictionary, V::OSAllocator, 0);

        friend Module;
        friend Name;
        friend Internal::NameData;
        
    public:

        static void Create();

        static void Destroy();
        static bool IsReady();
        static NameDictionary& Instance();

        //! Makes a Name from the provided raw string. If an entry already exists in the dictionary, it is shared.
        //! Otherwise, it is added to the internal dictionary.
        //! 
        //! @param name The name to resolve against the dictionary.
        //! @return A Name instance holding a dictionary entry associated with the provided raw string.
        Name MakeName(VStd::string_view name);

        //! Search for an existing name in the dictionary by hash.
        //! @param hash The key by which to search for the name.
        //! @return A Name instance. If the hash was not found, the Name will be empty.
        Name FindName(Name::Hash hash) const;

    private:
        NameDictionary();
        ~NameDictionary();

        void ReportStats() const;

        //////////////////////////////////////////////////////////////////////////
        // Private API for NameData

        // Attempts to release the name from the dictionary, but checks to make sure
        // a reference wasn't taken by another thread.
        void TryReleaseName(Name::Hash hash);
        
        //////////////////////////////////////////////////////////////////////////

        // Calculates a hash for the provided name string.
        // Does not attempt to resolve hash collisions; that is handled elsewhere.
        Name::Hash CalcHash(VStd::string_view name);
                
        VStd::unordered_map<Name::Hash, Internal::NameData*> m_dictionary;
        mutable VStd::shared_mutex m_sharedMutex;
    };
}

#endif // V_FRAMEWORK_CORE_NAME_NAME_DICTIONARY_H