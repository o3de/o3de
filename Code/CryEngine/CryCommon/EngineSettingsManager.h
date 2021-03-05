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

#ifndef CRYINCLUDE_CRYCOMMON_ENGINESETTINGSMANAGER_H
#define CRYINCLUDE_CRYCOMMON_ENGINESETTINGSMANAGER_H
#pragma once

#include "ProjectDefines.h"

#if defined(CRY_ENABLE_RC_HELPER)

#include "SettingsManagerHelpers.h"

class CEngineSettingsBackend;

//////////////////////////////////////////////////////////////////////////
// Manages storage and loading of all information for tools and CryENGINE, by either registry or an INI file.
// Information can be read and set by key-to-value functions.
// Specific information can be set by a dialog application called by this class.
// If the engine root path is not found, a fall-back dialog is opened.
class CEngineSettingsManager
{
public:
    // prepares CEngineSettingsManager to get requested information either from registry or an INI file,
    // if existent as a file with name an directory equal to the module, or from registry.
    CEngineSettingsManager(const wchar_t* moduleName = NULL, const wchar_t* iniFileName = NULL);
    ~CEngineSettingsManager();

    void RestoreDefaults();

    // stores/loads user specific information for modules to/from registry or INI file
    bool GetModuleSpecificStringEntryUtf16(const char* key, SettingsManagerHelpers::CWCharBuffer wbuffer);
    bool GetModuleSpecificStringEntryUtf8(const char* key, SettingsManagerHelpers::CCharBuffer buffer);
    bool GetModuleSpecificIntEntry(const char* key, int& value);
    bool GetModuleSpecificBoolEntry(const char* key, bool& value);

    bool SetModuleSpecificStringEntryUtf16(const char* key, const wchar_t* str);
    bool SetModuleSpecificStringEntryUtf8(const char* key, const char* str);
    bool SetModuleSpecificIntEntry(const char* key, const int& value);
    bool SetModuleSpecificBoolEntry(const char* key, const bool& value);

    bool GetValueByRef(const char* key, SettingsManagerHelpers::CWCharBuffer wbuffer) const;
    bool GetValueByRef(const char* key, bool& value) const;
    bool GetValueByRef(const char* key, int& value) const;

    void SetKey(const char* key, const wchar_t* value);
    void SetKey(const char* key, bool value);
    void SetKey(const char* key, int value);

    bool StoreData();

    bool GetInstalledBuildRootPathUtf16(const int index, SettingsManagerHelpers::CWCharBuffer name, SettingsManagerHelpers::CWCharBuffer path);

    void SetParentDialog(size_t window);

private:
    bool HasKey(const char* key);

    void LoadEngineSettingsFromRegistry();
    bool StoreEngineSettingsToRegistry();

    // parses a file and stores all flags in a private key-value-map
    bool LoadValuesFromConfigFile(const wchar_t* szFileName);

private:
    CEngineSettingsBackend *m_backend;

    SettingsManagerHelpers::CFixedString<wchar_t, 256> m_sModuleName;         // name to store key-value pairs of modules in (registry) or to identify INI file
    SettingsManagerHelpers::CFixedString<wchar_t, 256> m_sModuleFileName;     // used in case of data being loaded from INI file
    bool m_bGetDataFromBackend;
    SettingsManagerHelpers::CKeyValueArray<30> m_keyValueArray;

    void* m_hBtnBrowse;
    size_t m_hWndParent;
};

#endif // CRY_ENABLE_RC_HELPER

#endif // CRYINCLUDE_CRYCOMMON_ENGINESETTINGSMANAGER_H
