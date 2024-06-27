#include <vcore/utilitys/utility.h>
#include <vcore/platform_incl.h>
#include <vcore/std/string/conversions.h>

#include <stdlib.h>

namespace V::Utilitys {

    V::IO::FixedMaxPathString GetHomeDirectory()
    {
        // TODO: 读取环境变量
        char userProfileBuffer[V::IO::MaxPathLength]{};
        size_t variableSize = 0;
        auto err = getenv_s(&variableSize, userProfileBuffer, V::IO::MaxPathLength, "USERPROFILE");
        if (!err)
        {
            V::IO::FixedMaxPath path{ userProfileBuffer };
            return path.Native();
        }

        return {};
    }
}