#ifndef V_FRAMEWORK_CORE_STATISTICS_STATISTICS_MANAGER_H
#define V_FRAMEWORK_CORE_STATISTICS_STATISTICS_MANAGER_H

#include <core/std/containers/vector.h>
#include <core/std/containers/unordered_map.h>
#include <core/statistics/named_running_statistic.h>

namespace V
{
    namespace Statistics
    {
        /**
         * @brief A Collection of Running Statistics, addressable by a hashable
         *        class/primitive. e.g. V::Crc32, int, VStd::string, etc.
         *
         */
        template <class StatIdType = VStd::string>
        class StatisticsManager
        {
        public:
            StatisticsManager() = default;

            StatisticsManager(const StatisticsManager& other)
            {
                m_statistics.reserve(other.m_statistics.size());
                for (auto const& it : other.m_statistics)
                {
                    const StatIdType& statId = it.first;
                    const NamedRunningStatistic* stat = it.second;
                    m_statistics[statId] = new NamedRunningStatistic(*stat);
                }
            }

            virtual ~StatisticsManager()
            {
                Clear();
            }

            bool ContainsStatistic(const StatIdType& statId) const
            {
                auto iterator = m_statistics.find(statId);
                return iterator != m_statistics.end();
            }

            V::u32 GetCount() const
            {
                return static_cast<V::u32>(m_statistics.size());
            }

            void GetAllStatistics(VStd::vector<NamedRunningStatistic*>& vector)
            {
                for (auto const& it : m_statistics)
                {
                    NamedRunningStatistic* stat = it.second;
                    vector.push_back(stat);
                }
            }

            //! Helper method to apply units to statistics with empty units string.
            V::u32 ApplyUnits(const VStd::string& units)
            {
                V::u32 updatedCount = 0;
                for (auto& it : m_statistics)
                {
                    NamedRunningStatistic* stat = it.second;
                    if (stat->GetUnits().empty())
                    {
                        stat->UpdateUnits(units);
                        updatedCount++;
                    }
                }
                return updatedCount;
            }

            void Clear()
            {
                for (auto& it : m_statistics)
                {
                    NamedRunningStatistic* stat = it.second;
                    delete stat;
                }
                m_statistics.clear();
            }

            /**
             * Returns nullptr if a statistic with such name doesn't exist,
             * otherwise returns a pointer to the statistic.
             */
            NamedRunningStatistic* GetStatistic(const StatIdType& statId)
            {
                auto iterator = m_statistics.find(statId);
                if (iterator == m_statistics.end())
                {
                    return nullptr;
                }
                return iterator->second;
            }

            //! Returns false if a NamedRunningStatistic with such id already exists.
            NamedRunningStatistic* AddStatistic(const StatIdType& statId, const bool failIfExist = true)
            {
                if (failIfExist)
                {
                    NamedRunningStatistic* prevStat = GetStatistic(statId);
                    if (prevStat)
                    {
                        return nullptr;
                    }
                }
                NamedRunningStatistic* stat = new NamedRunningStatistic();
                m_statistics[statId] = stat;
                return stat;
            }

            //! Returns false if a NamedRunningStatistic with such id already exists.
            NamedRunningStatistic* AddStatistic(const StatIdType& statId, const VStd::string& name, const VStd::string& units, const bool failIfExist = true)
            {
                if (failIfExist)
                {
                    NamedRunningStatistic* prevStat = GetStatistic(statId);
                    if (prevStat)
                    {
                        return nullptr;
                    }
                }
                NamedRunningStatistic* stat = new NamedRunningStatistic(name, units);
                m_statistics[statId] = stat;
                return stat;
            }

            virtual void RemoveStatistic(const StatIdType& statId)
            {
                auto iterator = m_statistics.find(statId);
                if (iterator == m_statistics.end())
                {
                    return;
                }
                NamedRunningStatistic* prevStat = iterator->second;
                delete prevStat;
                m_statistics.erase(iterator);
            }

            void ResetStatistic(const StatIdType& statId)
            {
                NamedRunningStatistic* stat = GetStatistic(statId);
                if (!stat)
                {
                    return;
                }
                stat->Reset();
            }

            void ResetAllStatistics()
            {
                for (auto& it : m_statistics)
                {
                    NamedRunningStatistic* stat = it.second;
                    stat->Reset();
                }
            }

            void PushSampleForStatistic(const StatIdType& statId, double value)
            {
                NamedRunningStatistic* stat = GetStatistic(statId);
                if (!stat)
                {
                    return;
                }
                stat->PushSample(value);
            }

            //! Expensive function because it does a reverse lookup
            bool GetStatId(NamedRunningStatistic* searchStat, StatIdType& statIdOut) const
            {
                for (auto& it : m_statistics)
                {
                    NamedRunningStatistic* stat = it.second;
                    if (stat == searchStat)
                    {
                        statIdOut = it.first;
                        return true;
                    }
                }
                return false;
            }


        private:
            ///Key: StatIdType, Value: NamedRunningStatistic*
            VStd::unordered_map<StatIdType, NamedRunningStatistic*> m_statistics;
        };//class StatisticsManager
    }//namespace Statistics
}//namespace V


#endif // V_FRAMEWORK_CORE_STATISTICS_STATISTICS_MANAGER_H