// Copyright (C) 2013-2015 iFunFactory Inc. All Rights Reserved.
//
// This work is confidential and proprietary to iFunFactory Inc. and
// must not be used, disclosed, copied, or distributed without the prior
// consent of iFunFactory Inc.

#pragma once

#if PLATFORM_WINDOWS
#include <ws2def.h>
#endif

#include <vector>


namespace Fun
{
    class HostAddr
    {
    public:
        HostAddr (const char* host, uint16_t port)
        {
            this->host = host;
            this->port = port;
        }

        string host;
        uint16_t port;
    };

    class HostHttp : public HostAddr
    {
    public:
        HostHttp(const char* host, uint16_t port, bool https = false)
            : HostAddr(host, port)
        {
            this->https = https;
        }

        bool https;
    };


    class ConnectList
    {
    private:
        typedef std::vector<HostAddr*> HostList;

    public:
        ConnectList();

        void Add(const char* hostname, uint16_t port);
        void Add(const char* hostname, uint16_t port, bool https);
        void Add(const HostList* list);
        void Add(HostAddr* addr);
        
        void SetFirst();
        void SetLast();
        bool IsCurAvailable();
        bool IsNextAvailable();

        const HostAddr* GetCurAddress();
        const HostAddr* GetNextAddress();

    private:
        struct addrinfo* getDomainList(const char* hostname);

    private:
        HostList addr_list_;
        int addr_list_index_;
        bool first_;
    };

} // namespace Fun
