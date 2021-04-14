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

#include "CrySimpleErrorLog.hpp"
#include "CrySimpleServer.hpp"
#include "CrySimpleJob.hpp"

#include <Core/Common.h>
#include <Core/StdTypes.hpp>
#include <Core/Error.hpp>
#include <Core/STLHelper.hpp>
#include <Core/Mailer.h>
#include <tinyxml/tinyxml.h>

#include <AzCore/std/time.h>
#include <AzCore/IO/SystemFile.h>

#include <string>
#include <map>
#include <algorithm>

#ifdef _MSC_VER
#include <process.h>
#include <direct.h>
#endif
#ifdef UNIX
#include <pthread.h>
#endif

static unsigned int volatile g_bSendingMail = false;
static unsigned int volatile g_nMailNum = 0;

CCrySimpleErrorLog& CCrySimpleErrorLog::Instance()
{
    static CCrySimpleErrorLog g_Cache;
    return g_Cache;
}

CCrySimpleErrorLog::CCrySimpleErrorLog()
{
    m_lastErrorTime = 0;
}

void CCrySimpleErrorLog::Init()
{
}

bool CCrySimpleErrorLog::Add(ICryError* err)
{
    CCrySimpleMutexAutoLock Lock(m_LogMutex);

    if (m_Log.size() > 150)
    {
        // too many, just throw this error away
        return false; // no ownership of this error
    }

    m_Log.push_back(err);

    m_lastErrorTime = AZStd::GetTimeUTCMilliSecond();

    return true; // take ownership of this error
}

inline bool CmpError(ICryError* a, ICryError* b)
{
    return a->Compare(b);
}

void CCrySimpleErrorLog::SendMail()
{
    CSMTPMailer::tstrcol Rcpt;

    std::string mailBody;

    tdEntryVec RcptVec;
    CSTLHelper::Tokenize(RcptVec, SEnviropment::Instance().m_FailEMail, ";");

    for (size_t i = 0; i < RcptVec.size(); i++)
    {
        Rcpt.insert(RcptVec[i]);
    }

    tdErrorList tempLog;
    {
        CCrySimpleMutexAutoLock Lock(m_LogMutex);
        m_Log.swap(tempLog);
    }
#if defined(_MSC_VER)
    {
        char compName[256];
        DWORD size = ARRAYSIZE(compName);

        typedef BOOL (WINAPI * FP_GetComputerNameExA)(COMPUTER_NAME_FORMAT, LPSTR, LPDWORD);
        FP_GetComputerNameExA pGetComputerNameExA = (FP_GetComputerNameExA) GetProcAddress(LoadLibrary("kernel32.dll"), "GetComputerNameExA");

        if (pGetComputerNameExA)
        {
            pGetComputerNameExA(ComputerNamePhysicalDnsFullyQualified, compName, &size);
        }
        else
        {
            GetComputerName(compName, &size);
        }

        mailBody += std::string("Report sent from ") + compName + "...\n\n";
    }
#endif
    {
        bool dedupe = SEnviropment::Instance().m_DedupeErrors;

        std::vector<ICryError*> errors;

        if (!dedupe)
        {
            for (tdErrorList::const_iterator it = tempLog.begin(); it != tempLog.end(); ++it)
            {
                errors.push_back(*it);
            }
        }
        else
        {
            std::map<tdHash, ICryError*> uniqErrors;
            for (tdErrorList::const_iterator it = tempLog.begin(); it != tempLog.end(); ++it)
            {
                ICryError* err = *it;

                tdHash hash = err->Hash();

                std::map<tdHash, ICryError*>::iterator uniq = uniqErrors.find(hash);
                if (uniq != uniqErrors.end())
                {
                    uniq->second->AddDuplicate(err);
                    delete err;
                }
                else
                {
                    uniqErrors[hash] = err;
                }
            }

            for (std::map<tdHash, ICryError*>::iterator it = uniqErrors.begin(); it != uniqErrors.end(); ++it)
            {
                errors.push_back(it->second);
            }
        }

        std::string body = mailBody;
        CSMTPMailer::tstrcol cc;
        CSMTPMailer::tattachlist Attachment;

        std::sort(errors.begin(), errors.end(), CmpError);

        int a = 0;
        for (uint32_t i = 0; i < errors.size(); i++)
        {
            ICryError* err = errors[i];

            err->SetUniqueID(a + 1);

            // doesn't have to be related to any job/error,
            // we just use it to differentiate "1-IlluminationPS.txt" from "1-IlluminationPS.txt"
            long req = CCrySimpleJob::GlobalRequestNumber();

            if (err->HasFile())
            {
                char Filename[1024];
                azsprintf(Filename, "%d-req%ld-%s", a + 1, req, err->GetFilename().c_str());

                char DispFilename[1024];
                azsprintf(DispFilename, "%d-%s", a + 1, err->GetFilename().c_str());

                std::string sErrorFile = (SEnviropment::Instance().m_ErrorPath / Filename).c_str();

                std::vector<uint8_t> bytes;
                std::string text = err->GetFileContents();
                bytes.resize(text.size() + 1);
                std::copy(text.begin(), text.end(), bytes.begin());
                while (bytes.size() && bytes[bytes.size() - 1] == 0)
                {
                    bytes.pop_back();
                }

                CrySimple_SECURE_START

                CSTLHelper::ToFile(sErrorFile, bytes);

                Attachment.push_back(CSMTPMailer::tattachment(DispFilename, sErrorFile));

                CrySimple_SECURE_END
            }

            body += std::string("=============================================================\n");
            body += err->GetErrorDetails(ICryError::OUTPUT_EMAIL) + "\n";

            err->AddCCs(cc);
            a++;

            if (i == errors.size() - 1 || !err->CanMerge(errors[i + 1]))
            {
                CSMTPMailer::tstrcol bcc;

                CSMTPMailer mail("", "", SEnviropment::Instance().m_MailServer);
                mail.Send(SEnviropment::Instance().m_FailEMail, Rcpt, cc, bcc, err->GetErrorName(), body, Attachment);

                a = 0;
                body = mailBody;
                cc.clear();

                for (CSMTPMailer::tattachlist::iterator attach = Attachment.begin(); attach != Attachment.end(); ++attach)
                {
                    AZ::IO::SystemFile::Delete(attach->second.c_str());
                }

                Attachment.clear();
            }

            delete err;
        }
    }

    g_bSendingMail = false;
}

//////////////////////////////////////////////////////////////////////////
void CCrySimpleErrorLog::Tick()
{
    if (SEnviropment::Instance().m_MailInterval == 0)
    {
        return;
    }

    AZ::u64 lastError = 0;
    bool forceFlush = false;

    {
        CCrySimpleMutexAutoLock Lock(m_LogMutex);

        if (m_Log.size() == 0)
        {
            return;
        }

        // log has gotten pretty big, force a flush to avoid losing any errors
        if (m_Log.size() > 100)
        {
            forceFlush = true;
        }

        lastError = m_lastErrorTime;
    }

    AZ::u64 t = AZStd::GetTimeUTCMilliSecond();
    if (forceFlush || t < lastError || (t - lastError) > SEnviropment::Instance().m_MailInterval * 1000)
    {
        if (!g_bSendingMail)
        {
            g_bSendingMail = true;
            g_nMailNum++;
            logmessage("Sending Errors Mail %d\n", g_nMailNum);
            CCrySimpleErrorLog::Instance().SendMail();
        }
    }
}
