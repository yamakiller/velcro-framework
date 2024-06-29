#include <vcore/socket/vsocket.h>

namespace V
{
    namespace VSock
    {
        VSocketAddress::VSocketAddress()
        {
            Reset();
        }

        VSocketAddress& VSocketAddress::operator=(const VSOCKADDR& addr)
        {
            m_sockAddr = *reinterpret_cast<const VSOCKADDR_IN*>(&addr);
            return *this;
        }

        bool VSocketAddress::operator==(const VSocketAddress& rhs) const
        {
            return m_sockAddr.sin_family == rhs.m_sockAddr.sin_family
                && m_sockAddr.sin_addr.s_addr == rhs.m_sockAddr.sin_addr.s_addr
                && m_sockAddr.sin_port == rhs.m_sockAddr.sin_port;
        }

        const VSOCKADDR* VSocketAddress::GetTargetAddress() const
        {
            return reinterpret_cast<const VSOCKADDR*>(&m_sockAddr);
        }

        VStd::string VSocketAddress::GetIP() const
        {
            char ip[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, const_cast<in_addr*>(&m_sockAddr.sin_addr), ip, V_ARRAY_SIZE(ip));
            return VStd::string(ip);
        }

        VStd::string VSocketAddress::GetAddress() const
        {
            char ip[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, const_cast<in_addr*>(&m_sockAddr.sin_addr), ip, V_ARRAY_SIZE(ip));
            return VStd::string::format("%s:%d", ip, V::VSock::NetToHostShort(m_sockAddr.sin_port));
        }

        V::u16 VSocketAddress::GetAddrPort() const
        {
            return V::VSock::NetToHostShort(m_sockAddr.sin_port);
        }

        void VSocketAddress::SetAddrPort(V::u16 port)
        {
            m_sockAddr.sin_port = V::VSock::HostToNetShort(port);
        }

        bool VSocketAddress::SetAddress(const VStd::string& ip, V::u16 port)
        {
            V_Assert(!ip.empty(), "Invalid address string!");
            Reset();
            return V::VSock::ResolveAddress(ip, port, m_sockAddr);
        }

        bool VSocketAddress::SetAddress(V::u32 ip, V::u16 port)
        {
            Reset();
            m_sockAddr.sin_addr.s_addr = V::VSock::HostToNetLong(ip);
            m_sockAddr.sin_port = V::VSock::HostToNetShort(port);
            return true;
        }

        void VSocketAddress::Reset()
        {
            memset(&m_sockAddr, 0, sizeof(m_sockAddr));
            m_sockAddr.sin_family = AF_INET;
            m_sockAddr.sin_addr.s_addr = INADDR_ANY;
        }
    } // namespace VSock
} // namespace V
