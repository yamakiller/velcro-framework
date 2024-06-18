#ifndef V_FRAMEWORK_CORE_DETECTOR_DEFAULT_STRING_POOL_H
#define V_FRAMEWORK_CORE_DETECTOR_DEFAULT_STRING_POOL_H


#include <core/detector/stream.h>

#include <core/std/containers/unordered_map.h>
#include <core/std/containers/unordered_set.h>
#include <core/math/crc.h>

namespace V
{
    namespace Debug
    {
        template<class Key, class Mapped>
        struct unordered_map
        {
            typedef VStd::unordered_map<Key, Mapped, VStd::hash<Key>, VStd::equal_to<Key>, OSStdAllocator> type;
        };

        template<class Key>
        struct unordered_set
        {
            typedef VStd::unordered_set<Key, VStd::hash<Key>, VStd::equal_to<Key>, OSStdAllocator> type;
        };

        /**
         * Default implementation of a string pool.
         */
        class DetectorDefaultStringPool
            : public DetectorStringPool
        {
        public:
            virtual ~DetectorDefaultStringPool()
            {
                Reset();
            }

            typedef unordered_map<V::u32, const char*>::type CrcToStringMapType;
            typedef unordered_set<const char*>::type OwnedStringsMapType;

            /**
             * Add a copy of the string to the pool. If we return true the string was added otherwise it was already in the bool.
             * In both cases the crc32 of the string and the pointer to the shared copy is returned (optional for poolStringAddress)!
             */
            virtual bool    InsertCopy(const char* string, unsigned int length, V::u32& crc32, const char** poolStringAddress = nullptr)
            {
                crc32 = V::Crc32(string, length);
                CrcToStringMapType::pair_iter_bool insertIt = m_crcToStringMap.insert_key(crc32);
                if (insertIt.second)
                {
                    char* newString = reinterpret_cast<char*>(vmalloc(length + 1, 1, V::OSAllocator));
                    memcpy(newString, string, length);
                    newString[length] = '\0'; // terminate
                    m_ownedStrings.insert(newString);
                    insertIt.first->second = newString;
                }
                if (poolStringAddress)
                {
                    *poolStringAddress = insertIt.first->second;
                }
                return insertIt.second;
            }

            /**
             * Same as the InsertCopy above without actually coping the string into the pool. The pool assumes that
             * none of the strings added to the pool will be deleted.
             */
            virtual bool    Insert(const char* string, unsigned int length, V::u32& crc32)
            {
                crc32 = V::Crc32(string, length);
                return m_crcToStringMap.insert(VStd::make_pair(crc32, string)).second;
            }

            /// Finds a string in the pool by crc32.
            virtual const char* Find(V::u32 crc32)
            {
                CrcToStringMapType::iterator it = m_crcToStringMap.find(crc32);
                if (it != m_crcToStringMap.end())
                {
                    return it->second;
                }
                return NULL;
            }

            virtual void    Erase(V::u32 crc32)
            {
                CrcToStringMapType::iterator it = m_crcToStringMap.find(crc32);
                if (it != m_crcToStringMap.end())
                {
                    OwnedStringsMapType::iterator ownerIt = m_ownedStrings.find(it->second);
                    if (ownerIt != m_ownedStrings.end())
                    {
                        vfree(const_cast<char*>(it->second), V::OSAllocator);
                        m_ownedStrings.erase(ownerIt);
                    }
                    m_crcToStringMap.erase(it);
                }
            }

            virtual void    Reset()
            {
                for (OwnedStringsMapType::iterator it = m_ownedStrings.begin(); it != m_ownedStrings.end(); ++it)
                {
                    vfree(const_cast<char*>(*it), V::OSAllocator);
                }
                m_crcToStringMap.clear();
                m_ownedStrings.clear();
            }
        protected:
            CrcToStringMapType  m_crcToStringMap;
            OwnedStringsMapType m_ownedStrings;
        };
    } // namespace Debug
} // namespace V


#endif // V_FRAMEWORK_CORE_DETECTOR_DEFAULT_STRING_POOL_H