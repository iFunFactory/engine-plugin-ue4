// Copyright (C) 2013-2015 iFunFactory Inc. All Rights Reserved.
//
// This work is confidential and proprietary to iFunFactory Inc. and
// must not be used, disclosed, copied, or distributed without the prior
// consent of iFunFactory Inc.

#include "funapi_plugin_ue4.h"

#if PLATFORM_WINDOWS
#include "AllowWindowsPlatformTypes.h"
#include <WinSock2.h>
#include <WS2tcpip.h>
#else
#include <arpa/inet.h>
#include <sys/socket.h>
#include <stdint.h>
#include <netdb.h>
#include <errno.h>
#endif
#include "ConnectList.h"
#if PLATFORM_WINDOWS
#include "HideWindowsPlatformTypes.h"
#endif


namespace Fun
{
    ConnectList::ConnectList ()
        : addr_list_index_(0), first_(true)
    {
    }

    void ConnectList::Add (const char* hostname, uint16_t port)
    {
        struct addrinfo *servinfo = getDomainList(hostname);
        if (servinfo == NULL)
            return;

        struct sockaddr_in *h;
        char ip[INET6_ADDRSTRLEN];

        for (struct addrinfo *p = servinfo; p != NULL; p = p->ai_next)
        {
            h = (struct sockaddr_in *)p->ai_addr;
            inet_ntop(AF_INET, &h->sin_addr, ip, INET6_ADDRSTRLEN);  //AF_INET ipv4, AF_INET6 ipv6

            addr_list_.push_back(new HostAddr(ip, port));
        }

        LOG2("[%s] Dns address count : %d", *FString(hostname), addr_list_.size());
    }

    void ConnectList::Add (const char* hostname, uint16_t port, bool https)
    {
        struct addrinfo *servinfo = getDomainList(hostname);
        if (servinfo == NULL)
            return;

        struct sockaddr_in *h;
        char ip[INET6_ADDRSTRLEN];

        for (struct addrinfo *p = servinfo; p != NULL; p = p->ai_next)
        {
            h = (struct sockaddr_in *)p->ai_addr;
            inet_ntop(AF_INET, &h->sin_addr, ip, INET6_ADDRSTRLEN);  //AF_INET ipv4, AF_INET6 ipv6

            addr_list_.push_back(new HostHttp(ip, port, https));
        }

        LOG2("[%s] Dns address count : %d", *FString(hostname), addr_list_.size());
    }

    void ConnectList::Add (const HostList* list)
    {
        if (list == NULL || list->size() <= 0) {
            LOG("ConnectList - Invalid connect list parameter.");
            return;
        }

        addr_list_.insert(addr_list_.end(), list->begin(), list->end());
    }

    void ConnectList::Add (HostAddr* addr)
    {
        addr_list_.push_back(addr);
    }

    void ConnectList::SetFirst()
    {
        addr_list_index_ = 0;
        first_ = true;
    }

    void ConnectList::SetLast()
    {
        addr_list_index_ = addr_list_.size();
    }

    bool ConnectList::IsCurAvailable()
    {
        return addr_list_.size() > 0 && addr_list_index_ < addr_list_.size();
    }

    bool ConnectList::IsNextAvailable()
    {
        return addr_list_.size() > 0 && addr_list_index_ + 1 < addr_list_.size();
    }

    const HostAddr* ConnectList::GetCurAddress()
    {
        if (!IsCurAvailable())
            return NULL;

        return addr_list_[addr_list_index_];
    }

    const HostAddr* ConnectList::GetNextAddress()
    {
        if (first_)
        {
            first_ = false;
            return GetCurAddress();
        }

        if (!IsNextAvailable())
            return NULL;

        ++addr_list_index_;
        return addr_list_[addr_list_index_];
    }

    struct addrinfo* ConnectList::getDomainList (const char* hostname)
    {
        struct addrinfo hints, *servinfo;

        memset(&hints, 0, sizeof hints);
        hints.ai_family = AF_UNSPEC; //AF_INET ipv4, AF_INET6 ipv6
        hints.ai_socktype = 0; //SOCK_STREAM, SOCK_DGRAM
        hints.ai_protocol = IPPROTO_TCP;

        int rv = getaddrinfo(hostname, NULL, &hints, &servinfo);
        if (rv != 0)
        {
            LOG1("ConnectList::getDomainList - %s", *FString(gai_strerror(rv)));
            return NULL;
        }

        return servinfo;
    }

} // namespace Fun
