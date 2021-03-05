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

#ifndef CRYINCLUDE_CRYSYSTEM_SYSTEMCFG_H
#define CRYINCLUDE_CRYSYSTEM_SYSTEMCFG_H

#pragma once

#include <math.h>
#include <map>

typedef string SysConfigKey;
typedef string SysConfigValue;

//////////////////////////////////////////////////////////////////////////
class CSystemConfiguration
{
public:
    CSystemConfiguration(const string& strSysConfigFilePath, CSystem* pSystem, ILoadConfigurationEntrySink* pSink, bool warnIfMissing = true);
    ~CSystemConfiguration();

    string RemoveWhiteSpaces(string& s)
    {
        s.Trim();
        return s;
    }

    bool IsError() const { return m_bError; }

private: // ----------------------------------------

    // Returns:
    //   success
    bool ParseSystemConfig();

    CSystem*                                               m_pSystem;
    string                                                  m_strSysConfigFilePath;
    bool                                                        m_bError;
    ILoadConfigurationEntrySink*       m_pSink;                                         // never 0
    bool m_warnIfMissing;
};


#endif // CRYINCLUDE_CRYSYSTEM_SYSTEMCFG_H
