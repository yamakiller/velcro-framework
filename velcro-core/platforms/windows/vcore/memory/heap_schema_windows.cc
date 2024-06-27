#include <vcore/platform_incl.h>

namespace V {
    namespace Platform {
        size_t GetHeapCapacity() {
            SYSTEM_INFO si;
            GetSystemInfo(&si);
            return (char*)si.lpMaximumApplicationAddress - (char*)si.lpMinimumApplicationAddress;
        }
    }
}