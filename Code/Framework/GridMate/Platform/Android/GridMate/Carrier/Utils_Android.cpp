/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <GridMate_Traits_Platform.h>
#include <GridMate/Carrier/Utils.h>
#include <GridMate/Carrier/Driver.h>

#include <netinet/in.h>
#include <sys/socket.h>
#include <linux/rtnetlink.h>
#include <arpa/inet.h>
#include <unistd.h>

namespace GridMate
{
    string Utils::GetMachineAddress(int familyType)
    {
        string machineName;
        struct RTMRequest
        {
            nlmsghdr m_msghdr;
            ifaddrmsg m_msg;
        } rtmRequest;

        int sock = socket(PF_NETLINK, SOCK_DGRAM, NETLINK_ROUTE);
        int family = familyType == Driver::BSD_AF_INET ? AF_INET : AF_INET6;

        memset(&rtmRequest, 0, sizeof(rtmRequest));
        rtmRequest.m_msghdr.nlmsg_len = NLMSG_LENGTH(sizeof(ifaddrmsg));
        rtmRequest.m_msghdr.nlmsg_flags = NLM_F_REQUEST | NLM_F_ROOT;
        rtmRequest.m_msghdr.nlmsg_type = RTM_GETADDR;
        rtmRequest.m_msg.ifa_family = family;

        rtattr* rtaAttr = reinterpret_cast<rtattr*>(((char*)&rtmRequest) + NLMSG_ALIGN(rtmRequest.m_msghdr.nlmsg_len));
        rtaAttr->rta_len = RTA_LENGTH(familyType == Driver::BSD_AF_INET ? 4 : 16);

        int res = send(sock, &rtmRequest, rtmRequest.m_msghdr.nlmsg_len, 0);
        if (res < 0)
        {
            close(sock);
            AZ_Warning("GridMate", false, "Failed to send rtm request");
            return "";
        }

        const unsigned int k_bufSize = 4096;
        char* buf = static_cast<char*>(azmalloc(k_bufSize, 1, GridMateAllocatorMP));
        res = recv(sock, buf, k_bufSize, 0);
        if (res > 0 && static_cast<unsigned int>(res) < k_bufSize)
        {
            for (nlmsghdr* nlmp = reinterpret_cast<nlmsghdr*>(buf); static_cast<unsigned int>(res) > sizeof(*nlmp); )
            {
                int len = nlmp->nlmsg_len;
                int reqLen = len - sizeof(*nlmp);
                if (reqLen < 0 || len > res || !NLMSG_OK(nlmp, res))
                {
                    AZ_Warning("GridMate", false, "Failed to dispatch rtnetlink request");
                    break;
                }

                ifaddrmsg* rtmp = reinterpret_cast<ifaddrmsg*>(NLMSG_DATA(nlmp));
                rtattr* rtatp = reinterpret_cast<rtattr*>(IFA_RTA(rtmp));

                char address[INET6_ADDRSTRLEN] = { 0 };
                bool isLoopback = false;
                string devname;
                for (int rtattrlen = IFA_PAYLOAD(nlmp); RTA_OK(rtatp, rtattrlen); rtatp = RTA_NEXT(rtatp, rtattrlen))
                {
                    if (rtatp->rta_type == IFA_ADDRESS)
                    {
                        in_addr* addr = reinterpret_cast<in_addr*>(RTA_DATA(rtatp));
                        inet_ntop(family, addr, address, sizeof(address));

                        static const in6_addr loopbackAddr6[] = { IN6ADDR_LOOPBACK_INIT };
                        isLoopback |= family == AF_INET && *reinterpret_cast<AZ::u32*>(addr) == 0x0100007F;
                        isLoopback |= family == AF_INET6 && !memcmp(addr, loopbackAddr6, sizeof(loopbackAddr6));
                    }

                    if (rtatp->rta_type == IFA_LABEL)
                    {
                        devname = reinterpret_cast<const char*>RTA_DATA(rtatp);
                    }
                }

                if (!isLoopback
                    && *address
                    && (!strcmp(devname.c_str(), "eth0") || !strcmp(devname.c_str(), "wlan0")))
                {
                    machineName = address;
                    break;
                }

                res -= NLMSG_ALIGN(len);
                nlmp = reinterpret_cast<nlmsghdr*>(((char*)nlmp + NLMSG_ALIGN(len)));
            }
        }

        azfree(buf, GridMateAllocatorMP, k_bufSize);
        close(sock);
        return machineName;
    }

    const char* GridMate::Utils::GetBroadcastAddress(int familyType)
    {
        if (familyType == Driver::BSD_AF_INET6)
        {
            return "FF02::1";
        }
        else if (familyType == Driver::BSD_AF_INET)
        {
            return "255.255.255.255";
        }
        else
        {
            return "";
        }
    }
}

