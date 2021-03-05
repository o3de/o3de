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

#ifndef __CRYSIMPLEJOB__
#define __CRYSIMPLEJOB__

#include <Core/Common.h>
#include <string>
#include <vector>
#include <string>
#include <AzCore/std/parallel/atomic.h>

class TiXmlElement;

enum ECrySimpleJobState
{
    ECSJS_NONE,
    ECSJS_DONE                  =   1,                  //this is checked on client side, don't change!
    ECSJS_JOBNOTFOUND,
    ECSJS_CACHEHIT,
    ECSJS_ERROR,
    ECSJS_ERROR_COMPILE = 5,                    //this is checked on client side, don't change!
    ECSJS_ERROR_COMPRESS,
    ECSJS_ERROR_FILEIO,
    ECSJS_ERROR_INVALID_PROFILE,
    ECSJS_ERROR_INVALID_PROJECT,
    ECSJS_ERROR_INVALID_PLATFORM,
    ECSJS_ERROR_INVALID_PROGRAM,
    ECSJS_ERROR_INVALID_ENTRY,
    ECSJS_ERROR_INVALID_COMPILEFLAGS,
    ECSJS_ERROR_INVALID_COMPILER,
    ECSJS_ERROR_INVALID_LANGUAGE,
    ECSJS_ERROR_INVALID_SHADERREQUESTLINE,
    ECSJS_ERROR_INVALID_SHADERLIST,
};

class CCrySimpleJob
{
    ECrySimpleJobState      m_State;
    uint32_t                            m_RequestIP;
    static AZStd::atomic_long m_GlobalRequestNumber;
    

protected:
    virtual bool                    ExecuteCommand(const std::string& rCmd, std::string& outError);
public:
    CCrySimpleJob(uint32_t requestIP);
    virtual                             ~CCrySimpleJob();

    virtual bool                    Execute(const TiXmlElement* pElement) = 0;

    void                                    State(ECrySimpleJobState S)
    {
        if (m_State < ECSJS_ERROR || S >= ECSJS_ERROR)
        {
            m_State = S;
        }
    }
    ECrySimpleJobState      State() const { return m_State; }
    const uint32_t&             RequestIP() const { return m_RequestIP; }
    static long GlobalRequestNumber() { return m_GlobalRequestNumber; }
};

#endif
