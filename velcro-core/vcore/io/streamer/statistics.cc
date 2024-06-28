#include <vcore/IO/streamer/statistics.h>
#include <vcore/debug/profiler.h>

namespace V
{
    namespace IO
    {
        Statistic Statistic::CreateFloat(VStd::string_view owner, VStd::string_view name, double value)
        {
            Statistic result;
            result.m_owner = owner;
            result.m_name = name;
            result.m_value.m_floatingPoint = value;
            result.m_type = Type::FloatingPoint;
            return result;
        }

        Statistic Statistic::CreateInteger(VStd::string_view owner, VStd::string_view name, s64 value)
        {
            Statistic result;
            result.m_owner = owner;
            result.m_name = name;
            result.m_value.m_integer = value;
            result.m_type = Type::Integer;
            return result;
        }

        Statistic Statistic::CreatePercentage(VStd::string_view owner, VStd::string_view name, double value)
        {
            Statistic result;
            result.m_owner = owner;
            result.m_name = name;
            result.m_value.m_floatingPoint = value;
            result.m_type = Type::Percentage;
            return result;
        }

        void Statistic::PlotImmediate(
            [[maybe_unused]] VStd::string_view owner,
            [[maybe_unused]] VStd::string_view name,
            [[maybe_unused]] double value)
        {
            V_PROFILE_DATAPOINT(AzCore, value,
                "Streamer/%.*s/%.*s (Raw)",
                v_numeric_cast<int>(owner.size()), owner.data(),
                v_numeric_cast<int>(name.size()), name.data());
        }

        Statistic::Statistic(const Statistic& rhs)
            : m_owner(rhs.m_owner)
            , m_name(rhs.m_name)
            , m_type(rhs.m_type)
        {
            memcpy(&m_value, &rhs.m_value, sizeof(m_value));
        }

        Statistic::Statistic(Statistic&& rhs)
            : m_owner(VStd::move(rhs.m_owner))
            , m_name(VStd::move(rhs.m_name))
            , m_type(rhs.m_type)
        {
            memcpy(&m_value, &rhs.m_value, sizeof(m_value));
        }

        Statistic& Statistic::operator=(const Statistic& rhs)
        {
            if (this != &rhs)
            {
                m_owner = rhs.m_owner;
                m_name = rhs.m_name;
                m_type = rhs.m_type;
                memcpy(&m_value, &rhs.m_value, sizeof(m_value));
            }
            return *this;
        }

        Statistic& Statistic::operator=(Statistic&& rhs)
        {
            if (this != &rhs)
            {
                m_owner = VStd::move(rhs.m_owner);
                m_name = VStd::move(rhs.m_name);
                m_type = rhs.m_type;
                memcpy(&m_value, &rhs.m_value, sizeof(m_value));
            }
            return *this;
        }

        VStd::string_view Statistic::GetOwner() const
        {
            return m_owner;
        }

        VStd::string_view Statistic::GetName() const
        {
            return m_name;
        }

        Statistic::Type Statistic::GetType() const
        {
            return m_type;
        }

        double Statistic::GetFloatValue() const
        {
            V_Assert(m_type == Type::FloatingPoint, "Trying to get a floating point value from a statistic that doesn't store a floating point value.");
            return m_value.m_floatingPoint;
        }

        s64 Statistic::GetIntegerValue() const
        {
            V_Assert(m_type == Type::Integer, "Trying to get a integer value from a statistic that doesn't store a integer value.");
            return m_value.m_integer;
        }

        double Statistic::GetPercentage() const
        {
            V_Assert(m_type == Type::Percentage, "Trying to get a percentage value from a statistic that doesn't store a percentage value.");
            return m_value.m_floatingPoint * 100.0;
        }
    } // namespace IO
} // namespace V
