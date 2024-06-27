#include <vcore/name/internal/name_data.h>
#include <vcore/name/name_dictionary.h>

namespace V
{
    namespace Internal
    {
        NameData::NameData(VStd::string&& name, Hash hash)
            : m_name{VStd::move(name)}
            , m_hash{hash}
        {}

        VStd::string_view NameData::GetName() const
        {
            return m_name;
        }

        NameData::Hash NameData::GetHash() const
        {
            return m_hash;
        }

        void NameData::add_ref()
        {
            V_Assert(m_useCount >= 0, "NameData has been deleted");
            ++m_useCount;
        }

        void NameData::release()
        {
            // this could be released after we decrement the counter, therefore we will
            // base the release on the hash which is stable
            Hash hash = m_hash;
            V_Assert(m_useCount > 0, "m_useCount is already 0!");
            if (m_useCount.fetch_sub(1) == 1)
            {
                V::NameDictionary::Instance().TryReleaseName(hash);
            }
        }
    }
}