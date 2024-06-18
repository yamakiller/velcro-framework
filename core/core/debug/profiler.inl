#include <core/interface/interface.h>

namespace V::Debug
{
    template<typename... T>
    void ProfileScope::BeginRegion(
        [[maybe_unused]] Budget* budget, [[maybe_unused]] const char* eventName, [[maybe_unused]] T const&... args)
    {
        if (!budget)
        {
            return;
        }
#if !defined(_RELEASE)
        // TODO: Verification that the supplied system name corresponds to a known budget
#if defined(USE_PIX)
        PIXBeginEvent(PIX_COLOR_INDEX(budget->Crc() & 0xff), eventName, args...);
#endif
        budget->BeginProfileRegion();

        if (auto profiler = V::Interface<Profiler>::Get(); profiler)
        {
            profiler->BeginRegion(budget, eventName);
        }
#endif
    }

    inline void ProfileScope::EndRegion([[maybe_unused]] Budget* budget)
    {
        if (!budget)
        {
            return;
        }
#if !defined(_RELEASE)
        budget->EndProfileRegion();
#if defined(USE_PIX)
        PIXEndEvent();
#endif
        if (auto profiler = V::Interface<Profiler>::Get("Profiler"); profiler)
        {
            profiler->EndRegion(budget);
        }
#endif
    }

    template<typename... T>
    ProfileScope::ProfileScope(Budget* budget, char const* eventName, T const&... args)
        : m_budget{ budget }
    {
        BeginRegion(budget, eventName, args...);
    }

    inline ProfileScope::~ProfileScope()
    {
        EndRegion(m_budget);
    }

} // namespace V::Debug