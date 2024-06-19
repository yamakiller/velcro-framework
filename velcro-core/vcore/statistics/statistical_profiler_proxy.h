#ifndef V_FRAMEWORK_CORE_STATISTICS_STATISTICAL_PROFILER_PROXY_H
#define V_FRAMEWORK_CORE_STATISTICS_STATISTICAL_PROFILER_PROXY_H

#include <vcore/interface/interface.h>
#include <vcore/statistics/statistical_profiler.h>
#include <vcore/std/containers/unordered_map.h>
#include <vcore/std/parallel/shared_spin_mutex.h>
#include <vcore/math/crc.h>

namespace V::Statistics
{
    using StatisticalProfilerId = uint32_t;

    class StatisticalProfilerProxy
    {
    public:

        using StatIdType = V::Crc32;
        using StatisticalProfilerType = StatisticalProfiler<StatIdType, VStd::shared_spin_mutex>;

        //! A Convenience class used to measure time performance of scopes of code
        //! with constructor/destructor. Suitable to be used as part of a macro
        //! to facilitate its usage.
        class TimedScope
        {
        public:
            TimedScope() = delete;

            TimedScope(const StatisticalProfilerId profilerId, const StatIdType& statId)
                : m_profilerId(profilerId)
                , m_statId(statId)
            {
                if (!m_profilerProxy)
                {
                    m_profilerProxy = V::Interface<StatisticalProfilerProxy>::Get("StatisticalProfilerProxy");
                    if (!m_profilerProxy)
                    {
                        return;
                    }
                }
                if (!m_profilerProxy->IsProfilerActive(profilerId))
                {
                    return;
                }
                m_startTime = VStd::chrono::high_resolution_clock::now();
            }

            ~TimedScope()
            {
                if (!m_profilerProxy)
                {
                    return;
                }
                VStd::chrono::system_clock::time_point stopTime = VStd::chrono::high_resolution_clock::now();
                VStd::chrono::microseconds duration = stopTime - m_startTime;
                m_profilerProxy->PushSample(m_profilerId, m_statId, static_cast<double>(duration.count()));
            }

            //! Required only for UnitTests
            static void ClearCachedProxy()
            {
                m_profilerProxy = nullptr;
            }

        private:
            static StatisticalProfilerProxy* m_profilerProxy;
            const StatisticalProfilerId m_profilerId;
            const StatIdType& m_statId;
            VStd::chrono::system_clock::time_point m_startTime;
        }; // class TimedScope

        friend class TimedScope;

        StatisticalProfilerProxy()
        {
            V::Interface<StatisticalProfilerProxy>::Register("StatisticalProfilerProxy", this);
        }

        virtual ~StatisticalProfilerProxy()
        {
            V::Interface<StatisticalProfilerProxy>::Unregister("StatisticalProfilerProxy");
        }

        // Note that you have to delete these for safety reasons, you will trip a static_assert if you do not
        StatisticalProfilerProxy(StatisticalProfilerProxy&&) = delete;
        StatisticalProfilerProxy& operator=(StatisticalProfilerProxy&&) = delete;

        void RegisterProfilerId(StatisticalProfilerId id)
        {
            m_profilers.try_emplace(id, ProfilerInfo());
        }

        bool IsProfilerActive(StatisticalProfilerId id) const
        {
            auto iter = m_profilers.find(id);
            return (iter != m_profilers.end()) ? iter->second.m_enabled : false;
        }

        StatisticalProfilerType& GetProfiler(StatisticalProfilerId id)
        {
            auto iter = m_profilers.try_emplace(id, ProfilerInfo()).first;
            return iter->second.m_profiler;
        }

        void ActivateProfiler(StatisticalProfilerId id, bool activate, bool autoCreate = true)
        {
            if (autoCreate)
            {
                auto iter = m_profilers.try_emplace(id, ProfilerInfo()).first;
                iter->second.m_enabled = activate;
            }
            else if (auto iter = m_profilers.find(id); iter != m_profilers.end())
            {
                iter->second.m_enabled = activate;
            }
        }

        void PushSample(StatisticalProfilerId id, const StatIdType& statId, double value)
        {
            if (auto iter = m_profilers.find(id); iter != m_profilers.end())
            {
                iter->second.m_profiler.PushSample(statId, value);
            }
        }

    private:
        struct ProfilerInfo
        {
            StatisticalProfilerType m_profiler;
            bool m_enabled{ false };
        };

        using ProfilerMap = VStd::unordered_map<StatisticalProfilerId, ProfilerInfo>;

        ProfilerMap m_profilers;
    }; // class StatisticalProfilerProxy
}

#endif // V_FRAMEWORK_CORE_STATISTICS_STATISTICAL_PROFILER_PROXY_H