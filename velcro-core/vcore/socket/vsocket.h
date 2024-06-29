#ifndef V_FRAMEWORK_CORE_SOCKET_VSOCKET_H
#define V_FRAMEWORK_CORE_SOCKET_VSOCKET_H

#include <vcore/socket/vsocket_platform.h>
#include <vcore/platform_incl.h>

#include <vcore/socket/vsocket_fwd.h>
#include <vcore/std/string/string.h>

typedef fd_set  VFD_SET;
typedef timeval VTIMEVAL;

namespace V
{
    namespace VSock
    {
        //! All socket errors should be -ve since a 0 or +ve result indicates success
        enum class VSockError : V::s32
        {
            eASE_NO_ERROR           = 0,        // No error reported

            eASE_SOCKET_INVALID     = -1,       // Invalid socket

            eASE_EACCES             = -2,       // Tried to establish a connection to an invalid address (such as a broadcast address)
            eASE_EADDRINUSE         = -3,       // Specified address already in use
            eASE_EADDRNOTAVAIL      = -4,       // Invalid address was specified
            eASE_EAFNOSUPPORT       = -5,       // Invalid socket type (or invalid protocol family)
            eASE_EALREADY           = -6,       // Connection is being established (if the function is called again in non-blocking state)
            eASE_EBADF              = -7,       // Invalid socket number specified
            eASE_ECONNABORTED       = -8,       // Connection was aborted
            eASE_ECONNREFUSED       = -9,       // Connection refused by destination
            eASE_ECONNRESET         = -10,       // Connection was reset (TCP only)
            eASE_EFAULT             = -11,      // Invalid socket number specified
            eASE_EHOSTDOWN          = -12,      // Other end is down and unreachable
            eASE_EINPROGRESS        = -13,      // Action is already in progress (when non-blocking)
            eASE_EINTR              = -14,      // A blocking socket call was cancelled
            eASE_EINVAL             = -15,      // Invalid argument or function call
            eASE_EISCONN            = -16,      // Specified connection is already established
            eASE_EMFILE             = -17,      // No more socket descriptors available
            eASE_EMSGSIZE           = -18,      // Message size is too large
            eASE_ENETUNREACH        = -19,      // Destination is unreachable
            eASE_ENOBUFS            = -20,      // Insufficient working memory
            eASE_ENOPROTOOPT        = -21,      // Invalid combination of 'level' and 'optname'
            eASE_ENOTCONN           = -22,      // Specified connection is not established
            eASE_ENOTINITIALISED    = -23,      // Socket layer not initialised (e.g. need to call WSAStartup() on Windows)
            eASE_EOPNOTSUPP         = -24,      // Socket type cannot accept connections
            eASE_EPIPE              = -25,      // The writing side of the socket has already been closed
            eASE_EPROTONOSUPPORT    = -26,      // Invalid protocol family
            eASE_ETIMEDOUT          = -27,      // TCP resend timeout occurred
            eASE_ETOOMANYREFS       = -28,      // Too many multicast addresses specified
            eASE_EWOULDBLOCK        = -29,      // Time out occurred when attempting to perform action
            eASE_EWOULDBLOCK_CONN   = -30,      // Only applicable to connect() - a non-blocking connection has been attempted and is in progress

            eASE_MISC_ERROR         = -1000
        };

        enum class VSocketOption : V::s32
        {
            REUSEADDR               = 0x04,
            KEEPALIVE               = 0x08,
            LINGER                  = 0x20,
        };

        const char* GetStringForError(V::s32 errorNumber);

        V::u32 HostToNetLong(V::u32 hstLong);
        V::u32 NetToHostLong(V::u32 netLong);
        V::u16 HostToNetShort(V::u16 hstShort);
        V::u16 NetToHostShort(V::u16 netShort);
        V::s32 GetHostName(VStd::string& hostname);

        VSOCKET Socket();
        VSOCKET Socket(V::s32 af, V::s32 type, V::s32 protocol);
        V::s32 SetSockOpt(VSOCKET sock, V::s32 level, V::s32 optname, const char* optval, V::s32 optlen);
        V::s32 SetSocketOption(VSOCKET sock, VSocketOption opt, bool enable);
        V::s32 EnableTCPNoDelay(VSOCKET sock, bool enable);
        V::s32 SetSocketBlockingMode(VSOCKET sock, bool blocking);
        V::s32 CloseSocket(VSOCKET sock);
        V::s32 Shutdown(VSOCKET sock, V::s32 how);
        V::s32 GetSockName(VSOCKET sock, VSocketAddress& addr);
        V::s32 Connect(VSOCKET sock, const VSocketAddress& addr);
        V::s32 Listen(VSOCKET sock, V::s32 backlog);
        VSOCKET Accept(VSOCKET sock, VSocketAddress& addr);
        V::s32 Send(VSOCKET sock, const char* buf, V::s32 len, V::s32 flags);
        V::s32 Recv(VSOCKET sock, char* buf, V::s32 len, V::s32 flags);
        V::s32 Bind(VSOCKET sock, const VSocketAddress& addr);

        V::s32 Select(VSOCKET sock, VFD_SET* readfds, VFD_SET* writefds, VFD_SET* exceptfds, VTIMEVAL* timeout);
        V::s32 IsRecvPending(VSOCKET sock, VTIMEVAL* timeout);
        V::s32 WaitForWritableSocket(VSOCKET sock, VTIMEVAL* timeout);

        //! Required to call startup before using most other socket calls
        //!  This really only is necessary on windows, but may be relevant for other platforms soon
        V::s32 Startup();

        //! Required to call cleanup exactly as many times as startup
        V::s32 Cleanup();

        V_FORCE_INLINE bool IsAzSocketValid(const VSOCKET& socket)
        {
            return socket >= static_cast<V::s32>(VSockError::eASE_NO_ERROR);
        }

        V_FORCE_INLINE bool SocketErrorOccured(V::s32 error)
        {
            return error < static_cast<V::s32>(VSockError::eASE_NO_ERROR);
        }

        bool ResolveAddress(const VStd::string& ip, V::u16 port, VSOCKADDR_IN& sockAddr);

        class VSocketAddress
        {
        public:
            VSocketAddress();

            VSocketAddress& operator=(const VSOCKADDR& addr);

            bool operator==(const VSocketAddress& rhs) const;
            V_FORCE_INLINE bool operator!=(const VSocketAddress& rhs) const
            {
                return !(*this == rhs);
            }

            const VSOCKADDR* GetTargetAddress() const;

            VStd::string GetIP() const;
            VStd::string GetAddress() const;

            V::u16 GetAddrPort() const;
            void SetAddrPort(V::u16 port);

            bool SetAddress(const VStd::string& ip, V::u16 port);
            bool SetAddress(V::u32 ip, V::u16 port);

        protected:
            void Reset();

        private:
            VSOCKADDR_IN m_sockAddr;
        };
    } // namespace VSock
} // namespace V


#endif // V_FRAMEWORK_CORE_SOCKET_VSOCKET_H