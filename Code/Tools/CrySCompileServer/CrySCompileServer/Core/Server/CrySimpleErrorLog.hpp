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

#ifndef __CRYSIMPLEERRORLOG__
#define __CRYSIMPLEERRORLOG__

#include "CrySimpleMutex.hpp"

#include <vector>
#include <list>

class ICryError;
typedef std::list<ICryError*> tdErrorList;

class CCrySimpleErrorLog
{
    CCrySimpleMutex                         m_LogMutex;       // protects both below variables
    tdErrorList                                     m_Log;            // error log
    AZ::u64                 m_lastErrorTime;  // last time an error came in (we mail out a little after we've stopped receiving errors)

    void                                                Init();
    void                                                SendMail();
    CCrySimpleErrorLog();
public:

    bool                                                Add(ICryError* err);
    void                        Tick();

    static CCrySimpleErrorLog&  Instance();
};

#endif
