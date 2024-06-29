#include <vcore/socket/vsocket.h>

namespace V
{
    namespace VSock
    {
        V::s32 TranslateOSError(V::s32 oserror)
        {
            V::s32 error;

            #define TRANSLATE(_from, _to) case (_from): error = static_cast<V::s32>(_to); break;

            switch (oserror)
            {
                TRANSLATE(0, VSockError::eASE_NO_ERROR);
                TRANSLATE(WSAEACCES, VSockError::eASE_EACCES);
                TRANSLATE(WSAEADDRINUSE, VSockError::eASE_EADDRINUSE);
                TRANSLATE(WSAEADDRNOTAVAIL, VSockError::eASE_EADDRNOTAVAIL);
                TRANSLATE(WSAEAFNOSUPPORT, VSockError::eASE_EAFNOSUPPORT);
                TRANSLATE(WSAEALREADY, VSockError::eASE_EALREADY);
                TRANSLATE(WSAEBADF, VSockError::eASE_EBADF);
                TRANSLATE(WSAECONNABORTED, VSockError::eASE_ECONNABORTED);
                TRANSLATE(WSAECONNREFUSED, VSockError::eASE_ECONNREFUSED);
                TRANSLATE(WSAECONNRESET, VSockError::eASE_ECONNRESET);
                TRANSLATE(WSAEFAULT, VSockError::eASE_EFAULT);
                TRANSLATE(WSAEHOSTDOWN, VSockError::eASE_EHOSTDOWN);
                TRANSLATE(WSAEINPROGRESS, VSockError::eASE_EINPROGRESS);
                TRANSLATE(WSAEINTR, VSockError::eASE_EINTR);
                TRANSLATE(WSAEINVAL, VSockError::eASE_EINVAL);
                TRANSLATE(WSAEISCONN, VSockError::eASE_EISCONN);
                TRANSLATE(WSAEMFILE, VSockError::eASE_EMFILE);
                TRANSLATE(WSAEMSGSIZE, VSockError::eASE_EMSGSIZE);
                TRANSLATE(WSAENETUNREACH, VSockError::eASE_ENETUNREACH);
                TRANSLATE(WSAENOBUFS, VSockError::eASE_ENOBUFS);
                TRANSLATE(WSAENOPROTOOPT, VSockError::eASE_ENOPROTOOPT);
                TRANSLATE(WSAENOTCONN, VSockError::eASE_ENOTCONN);
                TRANSLATE(WSAEOPNOTSUPP, VSockError::eASE_EOPNOTSUPP);
                TRANSLATE(WSAEPROTONOSUPPORT, VSockError::eASE_EAFNOSUPPORT);
                TRANSLATE(WSAETIMEDOUT, VSockError::eASE_ETIMEDOUT);
                TRANSLATE(WSAETOOMANYREFS, VSockError::eASE_ETOOMANYREFS);
                TRANSLATE(WSAEWOULDBLOCK, VSockError::eASE_EWOULDBLOCK);

                // No equivalent in the posix api
                TRANSLATE(WSANOTINITIALISED, VSockError::eASE_ENOTINITIALISED);

            default:
                V_TracePrintf("VSock", "AzSocket could not translate OS error code %x, treating as miscellaneous.\n", oserror);
                error = static_cast<V::s32>(VSockError::eASE_MISC_ERROR);
                break;
            }

#undef TRANSLATE

            return error;
        }

        V::s32 TranslateSocketOption(VSocketOption opt)
        {
            V::s32 value;

#define TRANSLATE(_from, _to) case (_from): value = (_to); break;

            switch (opt)
            {
                TRANSLATE(VSocketOption::REUSEADDR, SO_REUSEADDR);
                TRANSLATE(VSocketOption::KEEPALIVE, SO_KEEPALIVE);
                TRANSLATE(VSocketOption::LINGER, SO_LINGER);

            default:
                V_TracePrintf("VSock", "VSocket option %x not yet supported", opt);
                value = 0;
                break;
            }

#undef TRANSLATE

            return value;
        }

        VSOCKET HandleInvalidSocket(SOCKET sock)
        {
            VSOCKET azsock = static_cast<VSOCKET>(sock);
            if (sock == INVALID_SOCKET)
            {
                azsock = TranslateOSError(WSAGetLastError());
            }
            return azsock;
        }


        V::s32 HandleSocketError(V::s32 socketError)
        {
            if (socketError == SOCKET_ERROR)
            {
                socketError = TranslateOSError(WSAGetLastError());
            }
            return socketError;
        }

        const char* GetStringForError(V::s32 errorNumber)
        {
            VSockError errorCode = VSockError(errorNumber);

#define CASE_RETSTRING(errorEnum) case errorEnum: { return #errorEnum; }

            switch (errorCode)
            {
                CASE_RETSTRING(VSockError::eASE_NO_ERROR);
                CASE_RETSTRING(VSockError::eASE_SOCKET_INVALID);
                CASE_RETSTRING(VSockError::eASE_EACCES);
                CASE_RETSTRING(VSockError::eASE_EADDRINUSE);
                CASE_RETSTRING(VSockError::eASE_EADDRNOTAVAIL);
                CASE_RETSTRING(VSockError::eASE_EAFNOSUPPORT);
                CASE_RETSTRING(VSockError::eASE_EALREADY);
                CASE_RETSTRING(VSockError::eASE_EBADF);
                CASE_RETSTRING(VSockError::eASE_ECONNABORTED);
                CASE_RETSTRING(VSockError::eASE_ECONNREFUSED);
                CASE_RETSTRING(VSockError::eASE_ECONNRESET);
                CASE_RETSTRING(VSockError::eASE_EFAULT);
                CASE_RETSTRING(VSockError::eASE_EHOSTDOWN);
                CASE_RETSTRING(VSockError::eASE_EINPROGRESS);
                CASE_RETSTRING(VSockError::eASE_EINTR);
                CASE_RETSTRING(VSockError::eASE_EINVAL);
                CASE_RETSTRING(VSockError::eASE_EISCONN);
                CASE_RETSTRING(VSockError::eASE_EMFILE);
                CASE_RETSTRING(VSockError::eASE_EMSGSIZE);
                CASE_RETSTRING(VSockError::eASE_ENETUNREACH);
                CASE_RETSTRING(VSockError::eASE_ENOBUFS);
                CASE_RETSTRING(VSockError::eASE_ENOPROTOOPT);
                CASE_RETSTRING(VSockError::eASE_ENOTCONN);
                CASE_RETSTRING(VSockError::eASE_ENOTINITIALISED);
                CASE_RETSTRING(VSockError::eASE_EOPNOTSUPP);
                CASE_RETSTRING(VSockError::eASE_EPIPE);
                CASE_RETSTRING(VSockError::eASE_EPROTONOSUPPORT);
                CASE_RETSTRING(VSockError::eASE_ETIMEDOUT);
                CASE_RETSTRING(VSockError::eASE_ETOOMANYREFS);
                CASE_RETSTRING(VSockError::eASE_EWOULDBLOCK);
                CASE_RETSTRING(VSockError::eASE_EWOULDBLOCK_CONN);
                CASE_RETSTRING(VSockError::eASE_MISC_ERROR);
            }

#undef CASE_RETSTRING

            return "(invalid)";
        }

        V::u32 HostToNetLong(V::u32 hstLong)
        {
            return htonl(hstLong);
        }

        V::u32 NetToHostLong(V::u32 netLong)
        {
            return ntohl(netLong);
        }

        V::u16 HostToNetShort(V::u16 hstShort)
        {
            return htons(hstShort);
        }

        V::u16 NetToHostShort(V::u16 netShort)
        {
            return ntohs(netShort);
        }

        V::s32 GetHostName(VStd::string& hostname)
        {
            V::s32 result = 0;
            hostname.clear();

            char name[256];
            result = HandleSocketError(gethostname(name, V_ARRAY_SIZE(name)));
            if (result == static_cast<V::s32>(VSockError::eASE_NO_ERROR))
            {
                hostname = name;
            }
            return result;
        }

        VSOCKET Socket()
        {
            return Socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        }

        VSOCKET Socket(V::s32 af, V::s32 type, V::s32 protocol)
        {
            return HandleInvalidSocket(socket(af, type, protocol));
        }

        V::s32 SetSockOpt(VSOCKET sock, V::s32 level, V::s32 optname, const char* optval, V::s32 optlen)
        {
            V::s32 length(optlen);
            return HandleSocketError(setsockopt(sock, level, optname, optval, length));
        }

        V::s32 SetSocketOption(VSOCKET sock, VSocketOption opt, bool enable)
        {
            V::u32 val = enable ? 1 : 0;
            return SetSockOpt(sock, SOL_SOCKET, TranslateSocketOption(opt), reinterpret_cast<const char*>(&val), sizeof(val));
        }

        V::s32 EnableTCPNoDelay(VSOCKET sock, bool enable)
        {
            V::u32 val = enable ? 1 : 0;
            return SetSockOpt(sock, IPPROTO_TCP, TCP_NODELAY, reinterpret_cast<const char*>(&val), sizeof(val));
        }

        V::s32 SetSocketBlockingMode(VSOCKET sock, bool blocking)
        {
            u_long val = blocking ? 0 : 1;
            return HandleSocketError(ioctlsocket(sock, FIONBIO, &val));
        }

        V::s32 CloseSocket(VSOCKET sock)
        {
            return HandleSocketError(closesocket(sock));
        }

        V::s32 Shutdown(VSOCKET sock, V::s32 how)
        {
            return HandleSocketError(shutdown(sock, how));
        }

        V::s32 GetSockName(VSOCKET sock, VSocketAddress& addr)
        {
            VSOCKADDR sAddr;
            V::s32 sAddrLen = sizeof(VSOCKADDR);
            memset(&sAddr, 0, sAddrLen);
            V::s32 result = HandleSocketError(getsockname(sock, &sAddr, &sAddrLen));
            addr = sAddr;
            return result;
        }

        V::s32 Connect(VSOCKET sock, const VSocketAddress& addr)
        {
            V::s32 err = HandleSocketError(connect(sock, addr.GetTargetAddress(), sizeof(VSOCKADDR_IN)));
            if (err == static_cast<V::s32>(VSockError::eASE_EWOULDBLOCK))
            {
                err = static_cast<V::s32>(VSockError::eASE_EWOULDBLOCK_CONN);
            }
            return err;
        }

        V::s32 Listen(VSOCKET sock, V::s32 backlog)
        {
            return HandleSocketError(listen(sock, backlog));
        }

        VSOCKET Accept(VSOCKET sock, VSocketAddress& addr)
        {
            VSOCKADDR sAddr;
            V::s32 sAddrLen = sizeof(VSOCKADDR);
            memset(&sAddr, 0, sAddrLen);
            VSOCKET outSock = HandleInvalidSocket(accept(sock, &sAddr, &sAddrLen));
            addr = sAddr;
            return outSock;
        }

        V::s32 Send(VSOCKET sock, const char* buf, V::s32 len, V::s32 flags)
        {
            return HandleSocketError(send(sock, buf, len, flags));
        }

        V::s32 Recv(VSOCKET sock, char* buf, V::s32 len, V::s32 flags)
        {
            return HandleSocketError(recv(sock, buf, len, flags));
        }

        V::s32 Bind(VSOCKET sock, const VSocketAddress& addr)
        {
            return HandleSocketError(bind(sock, addr.GetTargetAddress(), sizeof(VSOCKADDR_IN)));
        }

        V::s32 Select(VSOCKET sock, VFD_SET* readfdsock, VFD_SET* writefdsock, VFD_SET* exceptfdsock, VTIMEVAL* timeout)
        {
            return HandleSocketError(::select(sock + 1, readfdsock, writefdsock, exceptfdsock, timeout));
        }

        V::s32 IsRecvPending(VSOCKET sock, VTIMEVAL* timeout)
        {
            VFD_SET readSet;
            FD_ZERO(&readSet);
            FD_SET(sock, &readSet);

            V::s32 ret = Select(sock, &readSet, nullptr, nullptr, timeout);
            if (ret >= 0)
            {
                ret = FD_ISSET(sock, &readSet);
                if (ret != 0)
                {
                    ret = 1;
                }
            }

            return ret;
        }

        V::s32 WaitForWritableSocket(VSOCKET sock, VTIMEVAL* timeout)
        {
            VFD_SET writeSet;
            FD_ZERO(&writeSet);
            FD_SET(sock, &writeSet);

            V::s32 ret = Select(sock, nullptr, &writeSet, nullptr, timeout);
            if (ret >= 0)
            {
                ret = FD_ISSET(sock, &writeSet);
                if (ret != 0)
                {
                    ret = 1;
                }
            }

            return ret;
        }

        V::s32 Startup()
        {
            WSAData wsaData;
            return TranslateOSError(WSAStartup(MAKEWORD(2, 2), &wsaData));
        }

        V::s32 Cleanup()
        {
            return TranslateOSError(WSACleanup());
        }

        bool ResolveAddress(const VStd::string& ip, V::u16 port, VSOCKADDR_IN& socketAddress)
        {
            bool foundAddr = false;

            addrinfo hints;
            memset(&hints, 0, sizeof(addrinfo));
            addrinfo* addrInfo;
            hints.ai_family = AF_INET;
            hints.ai_flags = AI_CANONNAME;
            char strPort[8];
            v_snprintf(strPort, V_ARRAY_SIZE(strPort), "%d", port);

            const char* address = ip.c_str();

            if (address && strlen(address) == 0) // getaddrinfo doesn't accept empty string
            {
                address = nullptr;
            }

            V::s32 err = HandleSocketError(getaddrinfo(address, strPort, &hints, &addrInfo));
            if (err == 0) // eASE_NO_ERROR
            {
                if (addrInfo->ai_family == AF_INET)
                {
                    socketAddress = *reinterpret_cast<const VSOCKADDR_IN*>(addrInfo->ai_addr);
                    foundAddr = true;
                }

                freeaddrinfo(addrInfo);
            }
            else
            {
                V_Assert(false, "VSocketAddress could not resolve address %s with port %d. (reason - %s)", ip.c_str(), port, GetStringForError(err));
            }

            return foundAddr;
        }
    };
}

/**
* Windows platform-specific net modules
*/
#pragma comment(lib, WIN_SOCK_LIB_TO_LINK)
