#ifndef V_FRAMEWORK_CORE_STATISTICS_NAMED_RUNNING_STATISTIC_H
#define V_FRAMEWORK_CORE_STATISTICS_NAMED_RUNNING_STATISTIC_H

#include <core/std/string/string.h>
#include <core/statistics/running_statistic.h>

namespace V
{
    namespace Statistics
    {
        /**
         * @brief A convenient class to assign name and units to a RunningStatistic
         *
         * Also provides convenient methods to format the statistics.
         */
        class NamedRunningStatistic : public RunningStatistic
        {
        public:
            NamedRunningStatistic(const VStd::string& name = "Unnamed", const VStd::string& units = "")
                : m_name(name), m_units(units)
            {
            }

            void UpdateName(const VStd::string& name)
            {
                m_name = name;
            }

            void UpdateUnits(const VStd::string& units)
            {
                m_units = units;
            }

            virtual ~NamedRunningStatistic() = default;

            const VStd::string& GetName() const { return m_name; }
            
            const VStd::string& GetUnits() const { return m_units; }
            
            VStd::string GetFormatted() const
            {
                return VStd::string::format("Name=\"%s\", Units=\"%s\", numSamples=%llu, avg=%f, min=%f, max=%f, stdev=%f",
                    m_name.c_str(), m_units.c_str(), GetNumSamples(), GetAverage(), GetMinimum(), GetMaximum(), GetStdev());
            }

            static const char * GetCsvHeader()
            {
                return "Name, Units, numSamples, avg, min, max, stdev";
            }

            VStd::string GetCsvFormatted() const
            {
                return VStd::string::format("\"%s\", \"%s\", %llu, %f, %f, %f, %f",
                    m_name.c_str(), m_units.c_str(), GetNumSamples(), GetAverage(), GetMinimum(), GetMaximum(), GetStdev());
            }

        private:
            VStd::string m_name;
            VStd::string m_units;
        }; //class NamedRunningStatistic
    }//namespace Statistics
}//namespace V

#endif // V_FRAMEWORK_CORE_STATISTICS_NAMED_RUNNING_STATISTIC_H