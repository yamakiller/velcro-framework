#include <vcore/name/name.h>
#include <vcore/name/name_dictionary.h>

namespace V
{
    Name::Name()
    {
        SetEmptyString();
    }

    Name::Name(VStd::string_view name)
    {
        SetName(name);
    }

    Name::Name(Hash hash)
    {
        *this = NameDictionary::Instance().FindName(hash);
    }

    Name::Name(Internal::NameData* data)
        : m_data{data}
        , m_view{data->GetName()}
        , m_hash{data->GetHash()}
    {}

    Name::Name(const Name& rhs)
    {
        *this = rhs;
    }

    Name& Name::operator=(const Name& rhs)
    {
        m_data = rhs.m_data;
        m_hash = rhs.m_hash;
        if (!rhs.m_view.empty())
        {
            m_view = rhs.m_view;
        }
        else
        {
            SetEmptyString();
        }
        return *this;
    }

    Name& Name::operator=(Name&& rhs)
    {
        if (rhs.m_view.empty())
        {
            // In this case we can't actually copy the values from rhs
            // because rhs.m_view points to the address of rhs.m_data.
            // Instead we just use SetEmptyString() to make our m_view
            // point to *our* m_data.
            m_data = nullptr;
            m_hash = 0;
            SetEmptyString();
        }
        else
        {
            m_data = VStd::move(rhs.m_data);
            m_view = rhs.m_view;
            m_hash = rhs.m_hash;
            rhs.SetEmptyString();
        }

        return *this;
    }

    Name& Name::operator=(VStd::string_view name)
    {
        SetName(name);
        return *this;
    }

    Name::Name(Name&& rhs)
    {
        *this = VStd::move(rhs);
    }

    void Name::SetEmptyString()
    {
        /**
         * When an VStd::string_view references an VStd::string, it sees the null-terminator and assigns m_begin / m_end
         * to '\0'. This means calling string_view::data() won't return nullptr. This is important in cases where data is fed
         * to C functions which call strlen.
         *
         * In order to make the 'empty' string non-null, the string_view is assigned to the memory location of m_data, which
         * is always a null pointer when the string is empty. This address is reset any time a move / copy occurs. This is safer
         * than using a value in the string table of the module, since a module shutdown would de-allocate that memory.
         */

        V_Assert(!m_data.get(), "Data pointer is not null.");
        m_view = reinterpret_cast<const char*>(&m_data);
        m_hash = 0;
    }

    void Name::SetName(VStd::string_view name)
    {
        if (!name.empty())
        {
            V_Assert(NameDictionary::IsReady(), "Attempted to initialize name '%.*s' before the NameDictionary is ready.", V_STRING_ARG(name));

            *this = VStd::move(NameDictionary::Instance().MakeName(name));
        }
        else
        {
            *this = Name();
        }
    }
    
    VStd::string_view Name::GetStringView() const
    {
        return m_view;
    }

    const char* Name::GetCStr() const
    {
        return m_view.data();
    }

    bool Name::IsEmpty() const
    {
        return m_view.empty();
    }
} // namespace V