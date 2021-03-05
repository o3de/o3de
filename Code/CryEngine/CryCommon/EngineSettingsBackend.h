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
#ifndef CRYINCLUDE_CRYCOMMON_ENGINESETTINGSBACKEND_H
#define CRYINCLUDE_CRYCOMMON_ENGINESETTINGSBACKEND_H
#pragma once

#include "ProjectDefines.h"

#ifdef CRY_ENABLE_RC_HELPER

#include "SettingsManagerHelpers.h"

#include <string>

class CEngineSettingsManager;

class CEngineSettingsBackend
{
public:
    CEngineSettingsBackend(CEngineSettingsManager* parent, const wchar_t* moduleName = NULL);
    virtual ~CEngineSettingsBackend();

    virtual std::wstring GetModuleFilePath() const = 0;

    virtual bool GetModuleSpecificStringEntryUtf16(const char* key, SettingsManagerHelpers::CWCharBuffer wbuffer) = 0;
    virtual bool GetModuleSpecificIntEntry(const char* key, int& value) = 0;
    virtual bool GetModuleSpecificBoolEntry(const char* key, bool& value) = 0;

    virtual bool SetModuleSpecificStringEntryUtf16(const char* key, const wchar_t* str) = 0;
    virtual bool SetModuleSpecificIntEntry(const char* key, const int& value) = 0;
    virtual bool SetModuleSpecificBoolEntry(const char* key, const bool& value) = 0;

    virtual bool GetInstalledBuildRootPathUtf16(const int index, SettingsManagerHelpers::CWCharBuffer name, SettingsManagerHelpers::CWCharBuffer path) = 0;

    virtual void LoadEngineSettingsFromRegistry() = 0;
    virtual bool StoreEngineSettingsToRegistry() = 0;

protected:
    CEngineSettingsManager* parent() const
    {
        return m_parent;
    }

    const std::wstring& moduleName() const
    {
        return m_moduleName;
    }

private:
    std::wstring m_moduleName;
    CEngineSettingsManager* m_parent;
};

#endif // CRY_ENABLE_RC_HELPER

#endif // CRYINCLUDE_CRYCOMMON_ENGINESETTINGSBACKEND_H
