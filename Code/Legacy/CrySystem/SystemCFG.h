/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#ifndef CRYINCLUDE_CRYSYSTEM_SYSTEMCFG_H
#define CRYINCLUDE_CRYSYSTEM_SYSTEMCFG_H

#pragma once

#include <math.h>
#include <map>

typedef AZStd::string SysConfigKey;
typedef AZStd::string SysConfigValue;

//////////////////////////////////////////////////////////////////////////
class CSystemConfiguration
{
public:
    CSystemConfiguration(const AZStd::string& strSysConfigFilePath, CSystem* pSystem, ILoadConfigurationEntrySink* pSink, bool warnIfMissing = true);
    ~CSystemConfiguration();

    bool IsError() const { return m_bError; }

private: // ----------------------------------------

    // Returns:
    //   success
    bool ParseSystemConfig();

    CSystem*                           m_pSystem;
    AZStd::string                      m_strSysConfigFilePath;
    bool                               m_bError;
    ILoadConfigurationEntrySink*       m_pSink;  // never 0
    bool m_warnIfMissing;
};


#endif // CRYINCLUDE_CRYSYSTEM_SYSTEMCFG_H
