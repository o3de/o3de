/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/
// Original file Copyright Crytek GMBH or its affiliates, used under license.

#include "CrySimpleHTTP.hpp"
#include "CrySimpleSock.hpp"
#include "CrySimpleJobCompile.hpp"
#include "CrySimpleServer.hpp"
#include "CrySimpleCache.hpp"

#include <Core/StdTypes.hpp>
#include <Core/Error.hpp>
#include <Core/STLHelper.hpp>
#include <tinyxml/tinyxml.h>

#include <AzCore/Jobs/Job.h>
#include <AzCore/std/algorithm.h>
#include <AzCore/std/string/string.h>

#include <assert.h>
#include <memory>


//////////////////////////////////////////////////////////////////////////
class CHTTPRequest
{
    CCrySimpleSock* m_pSock;
public:
    CHTTPRequest(CCrySimpleSock*    pSock)
        : m_pSock(pSock){}

    ~CHTTPRequest(){delete m_pSock; }

    CCrySimpleSock* Socket(){return m_pSock; }
};

#define HTML_HEADER "HTTP/1.1 200 OK\n\
Server: Shader compile server %s\n\
Content-Length: %zu\n\
Content-Language: de (nach RFC 3282 sowie RFC 1766)\n\
Content-Type: text/html\n\
Connection: close\n\
\n\
<html><title>shader compile server %s</title><body>"

#define TABLE_START "<TABLE BORDER=0 CELLSPACING=0 CELLPADDING=2 WIDTH=640>\n\
<TR bgcolor=lightgrey><TH align=left>Description</TH><TH WIDTH=5></TH><TH>Value</TH><TH>Max</TH>\n\
<TH WIDTH=10>&nbsp;</TH><TH align=center>%</TH></TR>\n"
#define TABLE_INFO  "<TR><TD>%s</TD><TD>&nbsp;</TD><TD align=left>%s</TD><TD align=center></TD><TD>&nbsp;</TD><TD valign=middle>\n\
</TD></TR>\n\
</TD></TR>\n"
#define TABLE_BAR   "<TR><TD>%s</TD><TD>&nbsp;</TD><TD align=center>%d</TD><TD align=center>%d</TD><TD>&nbsp;</TD><TD valign=middle>\n\
<TABLE><TR><TD bgcolor=darkred   style=\"width: %d;\"  ></TD>\n\
<TD><FONT SIZE=1>%d%%</FONT></TD></TR>\n\
</TABLE></TD></TR>\n\
</TD></TR>\n"
#define TABLE_END "</TABLE>"

std::string CreateBar(const std::string& rName, int Value, int Max, int Percentage)
{
    AZStd::string formattedString = AZStd::string::format(TABLE_BAR, rName.c_str(), Value, Max, Percentage, Percentage);
    return formattedString.c_str();
}

std::string CreateInfoText(const std::string& rName, const std::string& rValue)
{
    AZStd::string formattedString = AZStd::string::format(TABLE_INFO, rName.c_str(), rValue.c_str());
    return formattedString.c_str();
}

std::string CreateInfoText(const std::string& rName, int Value)
{
    char Text[64];
    azsprintf(Text, "%d", Value);
    return CreateInfoText(rName, Text);
}

class HttpProcessRequestJob
    : public AZ::Job
{
public:
    HttpProcessRequestJob(CHTTPRequest* request)
        : AZ::Job(true, nullptr)
        , m_request(request) { }

protected:
    void Process() override
    {
#if defined(AZ_PLATFORM_WINDOWS)
        FILETIME IdleTime0, IdleTime1;
        FILETIME KernelTime0, KernelTime1;
        FILETIME UserTime0, UserTime1;

        int Ret0 = GetSystemTimes(&IdleTime0, &KernelTime0, &UserTime0);
        Sleep(100);
        int Ret1 = GetSystemTimes(&IdleTime1, &KernelTime1, &UserTime1);
        const int Idle      =   IdleTime1.dwLowDateTime - IdleTime0.dwLowDateTime;
        const int Kernel    =   KernelTime1.dwLowDateTime - KernelTime0.dwLowDateTime;
        const int User      =   UserTime1.dwLowDateTime - UserTime0.dwLowDateTime;
        //const int Idle        =   IdleTime1.dwHighDateTime-IdleTime0.dwHighDateTime;
        //const int Kernel  =   KernelTime1.dwHighDateTime-KernelTime0.dwHighDateTime;
        //const int User        =   UserTime1.dwHighDateTime-UserTime0.dwHighDateTime;
        const int Total =   Kernel + User;
#else
        int Ret0 = 0;
        int Ret1 = 0;
        int Total = 0;
        int Idle = 0;
#endif

        std::string Ret = TABLE_START;

        Ret += CreateInfoText("<b>Load</b>:", "");
        if (Ret0 && Ret1 && Total)
        {
            Ret += CreateBar("CPU-Usage", Total - Idle, Total, 100 - Idle * 100 / Total);
        }

        Ret += CreateBar("CompileTasks", CCrySimpleJobCompile::GlobalCompileTasks(),
                CCrySimpleJobCompile::GlobalCompileTasksMax(),
                CCrySimpleJobCompile::GlobalCompileTasksMax() ?
                CCrySimpleJobCompile::GlobalCompileTasks() * 100 /
                CCrySimpleJobCompile::GlobalCompileTasksMax() : 0);


        Ret += CreateInfoText("<b>Setup</b>:", "");
        Ret += CreateInfoText("Root", SEnviropment::Instance().m_Root);
        Ret += CreateInfoText("CompilerPath", SEnviropment::Instance().m_CompilerPath);
        Ret += CreateInfoText("CachePath", SEnviropment::Instance().m_CachePath);
        Ret += CreateInfoText("TempPath", SEnviropment::Instance().m_TempPath);
        Ret += CreateInfoText("ErrorPath", SEnviropment::Instance().m_ErrorPath);
        Ret += CreateInfoText("ShaderPath", SEnviropment::Instance().m_ShaderPath);
        Ret += CreateInfoText("FailEMail", SEnviropment::Instance().m_FailEMail);
        Ret += CreateInfoText("MailServer", SEnviropment::Instance().m_MailServer);
        Ret += CreateInfoText("port", SEnviropment::Instance().m_port);
        Ret += CreateInfoText("MailInterval", SEnviropment::Instance().m_MailInterval);
        Ret += CreateInfoText("Caching", SEnviropment::Instance().m_Caching ? "Enabled" : "Disabled");
        Ret += CreateInfoText("FallbackServer", SEnviropment::Instance().m_FallbackServer == "" ? "None" : SEnviropment::Instance().m_FallbackServer);
        Ret += CreateInfoText("FallbackTreshold", static_cast<int>(SEnviropment::Instance().m_FallbackTreshold));
        Ret += CreateInfoText("DumpShaders", static_cast<int>(SEnviropment::Instance().m_DumpShaders));

        Ret += CreateInfoText("<b>Cache</b>:", "");
        Ret += CreateInfoText("Entries", CCrySimpleCache::Instance().EntryCount());
        Ret += CreateBar("Hits", CCrySimpleCache::Instance().Hit(),
                CCrySimpleCache::Instance().Hit() + CCrySimpleCache::Instance().Miss(),
                CCrySimpleCache::Instance().Hit() * 100 / AZStd::GetMax(1, (CCrySimpleCache::Instance().Hit() + CCrySimpleCache::Instance().Miss())));
        Ret += CreateInfoText("Pending Entries", static_cast<int>(CCrySimpleCache::Instance().PendingCacheEntries().size()));



        Ret += TABLE_END;

        Ret += "</Body></hmtl>";
        char Text[sizeof(HTML_HEADER) + 1024];
        azsprintf(Text, HTML_HEADER, __DATE__, Ret.size(), __DATE__);
        Ret = std::string(Text) + Ret;
        m_request->Socket()->Send(Ret);
    }

private:
    std::unique_ptr<CHTTPRequest> m_request;
};

class HttpServerJob
    : public AZ::Job
{
public:
    HttpServerJob(CCrySimpleHTTP* simpleHttp)
        : AZ::Job(true, nullptr)
        , m_simpleHttp(simpleHttp) { }

protected:
    void Process()
    {
        m_simpleHttp->Run();
    }
private:
    CCrySimpleHTTP* m_simpleHttp;
};

//////////////////////////////////////////////////////////////////////////
CCrySimpleHTTP::CCrySimpleHTTP()
    : m_pServerSocket(0)
{
    CrySimple_SECURE_START

        Init();

    CrySimple_SECURE_END
}

void CCrySimpleHTTP::Init()
{
    m_pServerSocket = new CCrySimpleSock(61480, SEnviropment::Instance().m_WhitelistAddresses);   //http
    m_pServerSocket->Listen();
    HttpServerJob* serverJob = new HttpServerJob(this);
    serverJob->Start();
}

void CCrySimpleHTTP::Run()
{
    while (1)
    {
        // New client message, receive new client socket connection.
        CCrySimpleSock* newClientSocket = m_pServerSocket->Accept();
        if(!newClientSocket)
        {
            continue;
        }

        // HTTP Request Data for new job
        CHTTPRequest* pData = new CHTTPRequest(newClientSocket);

        HttpProcessRequestJob* requestJob = new HttpProcessRequestJob(pData);
        requestJob->Start();
    }
}


