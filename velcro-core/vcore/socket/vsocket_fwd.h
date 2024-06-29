#ifndef V_FRAMEWORK_CORE_SOCKET_VSOCKET_FWD_H
#define V_FRAMEWORK_CORE_SOCKET_VSOCKET_FWD_H

#include <vcore/base.h>

#define SOCKET_ERROR (-1)
#define V_SOCKET_INVALID (-1)

#include <vcore/socket/vsocket_fwd_platform.h>

// Type wrappers for sockets
typedef sockaddr    VSOCKADDR;
typedef sockaddr_in VSOCKADDR_IN;
typedef V::s32      VSOCKET;

namespace V
{
    namespace VSock
    {
        enum class VSockError :    V::s32;
        enum class VSocketOption : V::s32;
        class VSocketAddress;
    } // namespace VSock
} // namespace V

#endif // V_FRAMEWORK_CORE_SOCKET_VSOCKET_FWD_H