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

#ifndef CRYINCLUDE_CRYCOMMON_PROFILELOG_H
#define CRYINCLUDE_CRYCOMMON_PROFILELOG_H
#pragma once


#include <ISystem.h> // <> required for Interfuscator
#include <ITimer.h> // <> required for Interfuscator

struct ILogElement
{
    virtual ~ILogElement(){}
    virtual ILogElement*    Log         (const char* name, const char* message) = 0;
    virtual ILogElement*    SetTime (float time)                                                        = 0;
    virtual void                    Flush       (stack_string& indent)                                  = 0;
};

struct IProfileLogSystem
{
    virtual ~IProfileLogSystem(){}
    virtual ILogElement*    Log         (const char* name, const char* msg)     = 0;
    virtual void                    SetTime (ILogElement* pElement, float time)     = 0;
    virtual void                    Release ()                                                                      =   0;
};

struct SHierProfileLogItem
{
    SHierProfileLogItem(const char* name, const char* msg, int inbDoLog)
        : m_pLogElement(NULL)
        , m_bDoLog(inbDoLog)
    {
        if (m_bDoLog)
        {
            m_pLogElement = gEnv->pProfileLogSystem->Log(name, msg);
            m_startTime = gEnv->pTimer->GetAsyncTime();
        }
    }
    ~SHierProfileLogItem()
    {
        if (m_bDoLog)
        {
            CTimeValue endTime = gEnv->pTimer->GetAsyncTime();
            gEnv->pProfileLogSystem->SetTime(m_pLogElement, (endTime - m_startTime).GetMilliSeconds());
        }
    }

private:
    int                     m_bDoLog;
    CTimeValue      m_startTime;
    ILogElement*    m_pLogElement;
};

#define HPROFILE_BEGIN(msg1, msg2, doLog) { SHierProfileLogItem __hier_profile_uniq_var_in_this_scope__(msg1, msg2, doLog);
#define HPROFILE_END() }

#define HPROFILE(msg1, msg2, doLog) SHierProfileLogItem __hier_profile_uniq_var_in_this_scope__(msg1, msg2, doLog);

#endif // CRYINCLUDE_CRYCOMMON_PROFILELOG_H
