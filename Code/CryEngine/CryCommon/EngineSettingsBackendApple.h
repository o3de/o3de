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
#ifndef CRYINCLUDE_CRYCOMMON_ENGINESETTINGSBACKENDAPPLE_H
#define CRYINCLUDE_CRYCOMMON_ENGINESETTINGSBACKENDAPPLE_H
#pragma once

#include "EngineSettingsBackend.h"

#ifdef CRY_ENABLE_RC_HELPER

class CEngineSettingsManager;
class SimpleRegistry;

class CEngineSettingsBackendApple : public CEngineSettingsBackend
{
public:
    CEngineSettingsBackendApple(CEngineSettingsManager* parent, const wchar_t* moduleName = NULL);
    ~CEngineSettingsBackendApple();

    std::wstring GetModuleFilePath() const override;

    bool GetModuleSpecificStringEntryUtf16(const char* key, SettingsManagerHelpers::CWCharBuffer wbuffer) override;
    bool GetModuleSpecificIntEntry(const char* key, int& value) override;
    bool GetModuleSpecificBoolEntry(const char* key, bool& value) override;

    bool SetModuleSpecificStringEntryUtf16(const char* key, const wchar_t* str) override;
    bool SetModuleSpecificIntEntry(const char* key, const int& value) override;
    bool SetModuleSpecificBoolEntry(const char* key, const bool& value) override;

    bool GetInstalledBuildRootPathUtf16(const int index, SettingsManagerHelpers::CWCharBuffer name, SettingsManagerHelpers::CWCharBuffer path) override;

    void LoadEngineSettingsFromRegistry() override;
    bool StoreEngineSettingsToRegistry() override;

private:
    SimpleRegistry *m_registry;
    std::string m_registryFilePath;
};

#endif // CRY_ENABLE_RC_HELPER

#endif // CRYINCLUDE_CRYCOMMON_ENGINESETTINGSBACKENDAPPLE_H
