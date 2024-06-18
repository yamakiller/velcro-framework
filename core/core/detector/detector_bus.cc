#include <core/detector/detector_bus.h>

#include <core/detector/detector.h>
#include <core/module/environment.h>
#include <core/std/parallel/mutex.h>

namespace V
{
    namespace Debug
    {
        //////////////////////////////////////////////////////////////////////////
        // Globals
        // We need to synchronize all detector evens, so we have proper order, and access to the data
        // We use a global mutex which should be used for all detector operations.
        // The mutex is held in an environment variable so it works across DLLs.
        EnvironmentVariable<VStd::recursive_mutex> _detectorGlobalMutex;
        //////////////////////////////////////////////////////////////////////////

        
        //=========================================================================
        // lock
        //=========================================================================
        void DetectorEventBusMutex::lock()
        {
            GetMutex().lock();
        }

        //=========================================================================
        // try_lock
        //=========================================================================
        bool DetectorEventBusMutex::try_lock()
        {
            return GetMutex().try_lock();
        }

        //=========================================================================
        // unlock
        //=========================================================================
        void DetectorEventBusMutex::unlock()
        {
            GetMutex().unlock();
        }

        //=========================================================================
        // unlock
        //=========================================================================
        VStd::recursive_mutex& DetectorEventBusMutex::GetMutex()
        {
            if (!_detectorGlobalMutex)
            {
                _detectorGlobalMutex = Environment::CreateVariable<VStd::recursive_mutex>(V_FUNCTION_SIGNATURE);
            }
            return *_detectorGlobalMutex;
        }
    } // namespace Debug
} // namespace V