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


#include "ProjectDefines.h"
#include "EngineSettingsManager.h"

#if defined(CRY_ENABLE_RC_HELPER)

#include <assert.h>                                     // assert()
#include "EngineSettingsBackend.h"

#include "AzCore/PlatformDef.h"
#include "platform.h"

#if defined(AZ_PLATFORM_WINDOWS)
#include "EngineSettingsBackendWin32.h"
#include <AzCore/PlatformIncl.h>
#elif AZ_TRAIT_OS_PLATFORM_APPLE
#include "EngineSettingsBackendApple.h"
#endif


#include <climits>
#include <cstdio>

#define INFOTEXT L"Please specify the directory of your CryENGINE installation (RootPath):"


using namespace SettingsManagerHelpers;


//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
CEngineSettingsManager::CEngineSettingsManager(const wchar_t* moduleName, const wchar_t* iniFileName)
    : m_hWndParent(0)
    , m_backend(NULL)
{
    m_sModuleName.clear();

#if defined(AZ_PLATFORM_WINDOWS)
    m_backend = new CEngineSettingsBackendWin32(this, moduleName);
#elif AZ_TRAIT_OS_PLATFORM_APPLE
    m_backend = new CEngineSettingsBackendApple(this, moduleName);
#endif
    assert(m_backend);

    // std initialization
    RestoreDefaults();

    // try to load content from INI file
    if (moduleName != NULL)
    {
        m_sModuleName = moduleName;

        if (iniFileName == NULL)
        {
            // find INI filename located in module path
            m_sModuleFileName = m_backend->GetModuleFilePath().c_str();
        }
        else
        {
            m_sModuleFileName = iniFileName;
        }

        if (LoadValuesFromConfigFile(m_sModuleFileName.c_str()))
        {
            m_bGetDataFromBackend = false;
            return;
        }
    }

    m_bGetDataFromBackend = true;

    // load basic content from registry
    LoadEngineSettingsFromRegistry();
}

//////////////////////////////////////////////////////////////////////////
CEngineSettingsManager::~CEngineSettingsManager()
{
    delete m_backend, m_backend = NULL;
}

//////////////////////////////////////////////////////////////////////////
void CEngineSettingsManager::RestoreDefaults()
{
    // Engine
    SetKey("ENG_RootPath", L"");

    // RC
    SetKey("RC_ShowWindow", false);
    SetKey("RC_HideCustom", false);
    SetKey("RC_Parameters", L"");
}

//////////////////////////////////////////////////////////////////////////
bool CEngineSettingsManager::GetModuleSpecificStringEntryUtf16(const char* key, SettingsManagerHelpers::CWCharBuffer wbuffer)
{
    if (wbuffer.getSizeInElements() <= 0)
    {
        return false;
    }

    if (!m_bGetDataFromBackend)
    {
        if (!HasKey(key))
        {
            wbuffer[0] = 0;
            return false;
        }
        if (!GetValueByRef(key, wbuffer))
        {
            wbuffer[0] = 0;
            return false;
        }
    }
    else
    {
        assert(m_backend);
        return m_backend->GetModuleSpecificStringEntryUtf16(key, wbuffer);
    }

    return true;
}

//////////////////////////////////////////////////////////////////////////
bool CEngineSettingsManager::GetModuleSpecificStringEntryUtf8(const char* key, SettingsManagerHelpers::CCharBuffer buffer)
{
    if (buffer.getSizeInElements() <= 0)
    {
        return false;
    }

    wchar_t wBuffer[1024];

    if (!GetModuleSpecificStringEntryUtf16(key, SettingsManagerHelpers::CWCharBuffer(wBuffer, sizeof(wBuffer))))
    {
        buffer[0] = 0;
        return false;
    }

    SettingsManagerHelpers::ConvertUtf16ToUtf8(wBuffer, buffer);

    return true;
}


//////////////////////////////////////////////////////////////////////////
bool CEngineSettingsManager::GetModuleSpecificIntEntry(const char* key, int& value)
{
    value = 0;

    if (!m_bGetDataFromBackend)
    {
        if (!HasKey(key))
        {
            return false;
        }
        if (!GetValueByRef(key, value))
        {
            return false;
        }
    }
    else
    {
        assert(m_backend);
        return m_backend->GetModuleSpecificIntEntry(key, value);
    }

    return true;
}

//////////////////////////////////////////////////////////////////////////
bool CEngineSettingsManager::GetModuleSpecificBoolEntry(const char* key, bool& value)
{
    value = false;

    if (!m_bGetDataFromBackend)
    {
        if (!HasKey(key))
        {
            return false;
        }
        if (!GetValueByRef(key, value))
        {
            return false;
        }
    }
    else
    {
        assert(m_backend);
        return m_backend->GetModuleSpecificBoolEntry(key, value);
    }

    return true;
}

//////////////////////////////////////////////////////////////////////////
bool CEngineSettingsManager::SetModuleSpecificStringEntryUtf16(const char* key, const wchar_t* str)
{
    SetKey(key, str);
    if (!m_bGetDataFromBackend)
    {
        return StoreData();
    }

    assert(m_backend);
    return m_backend->SetModuleSpecificStringEntryUtf16(key, str);
}

//////////////////////////////////////////////////////////////////////////
bool CEngineSettingsManager::SetModuleSpecificIntEntry(const char* key, const int& value)
{
    SetKey(key, value);
    if (!m_bGetDataFromBackend)
    {
        return StoreData();
    }

    assert(m_backend);
    return m_backend->SetModuleSpecificIntEntry(key, value);
}

//////////////////////////////////////////////////////////////////////////
bool CEngineSettingsManager::SetModuleSpecificBoolEntry(const char* key, const bool& value)
{
    SetKey(key, value);
    if (!m_bGetDataFromBackend)
    {
        return StoreData();
    }

    assert(m_backend);
    return m_backend->SetModuleSpecificBoolEntry(key, value);
}

//////////////////////////////////////////////////////////////////////////
bool CEngineSettingsManager::SetModuleSpecificStringEntryUtf8(const char* key, const char* str)
{
    wchar_t wbuffer[512];
    SettingsManagerHelpers::ConvertUtf8ToUtf16(str, SettingsManagerHelpers::CWCharBuffer(wbuffer, sizeof(wbuffer)));

    return SetModuleSpecificStringEntryUtf16(key, wbuffer);
}

//////////////////////////////////////////////////////////////////////////
bool CEngineSettingsManager::HasKey(const char* key)
{
    return m_keyValueArray.find(key) != 0;
}

//////////////////////////////////////////////////////////////////////////
void CEngineSettingsManager::SetKey(const char* key, const wchar_t* value)
{
    m_keyValueArray.set(key, value);
}

//////////////////////////////////////////////////////////////////////////
void CEngineSettingsManager::SetKey(const char* key, bool value)
{
    m_keyValueArray.set(key, (value ? L"true" : L"false"));
}

//////////////////////////////////////////////////////////////////////////
void CEngineSettingsManager::SetKey(const char* key, int value)
{
    m_keyValueArray.set(key, std::to_wstring(value).c_str());
}

bool CEngineSettingsManager::GetInstalledBuildRootPathUtf16(const int index, SettingsManagerHelpers::CWCharBuffer name, SettingsManagerHelpers::CWCharBuffer path)
{
    assert(m_backend);
    return m_backend->GetInstalledBuildRootPathUtf16(index, name, path);
}

//////////////////////////////////////////////////////////////////////////
void CEngineSettingsManager::SetParentDialog(size_t window)
{
    m_hWndParent = window;
}


//////////////////////////////////////////////////////////////////////////
bool CEngineSettingsManager::StoreData()
{
    if (m_bGetDataFromBackend)
    {
        bool res = StoreEngineSettingsToRegistry();

        if (!res)
        {
#ifdef AZ_PLATFORM_WINDOWS
            MessageBoxA(reinterpret_cast<HWND>(m_hWndParent), "Could not store data to registry.", "Error", MB_OK | MB_ICONERROR);
#endif
        }
        return res;
    }

    // store data to INI file

    FILE* file;
#ifdef AZ_PLATFORM_WINDOWS
    _wfopen_s(&file, m_sModuleFileName.c_str(), L"wb");
#else
    char fname[MAX_PATH];
    memset(fname, 0, MAX_PATH);
    wcstombs(fname, m_sModuleFileName.c_str(), MAX_PATH);
    file = fopen(fname, "wb");
#endif
    if (file == NULL)
    {
        return false;
    }

    char buffer[2048];

    for (size_t i = 0; i < m_keyValueArray.size(); ++i)
    {
        const SKeyValue& kv = m_keyValueArray[i];

        fprintf_s(file, kv.key.c_str());
        fprintf_s(file, " = ");

        if (kv.value.length() > 0)
        {
            SettingsManagerHelpers::ConvertUtf16ToUtf8(kv.value.c_str(), SettingsManagerHelpers::CCharBuffer(buffer, sizeof(buffer)));
            fprintf_s(file, "%s", buffer);
        }

        fprintf_s(file, "\r\n");
    }

    fclose(file);

    return true;
}


//////////////////////////////////////////////////////////////////////////
bool CEngineSettingsManager::LoadValuesFromConfigFile(const wchar_t* szFileName)
{
    m_keyValueArray.clear();

    // read file to memory

    FILE* file;
#ifdef AZ_PLATFORM_WINDOWS
    _wfopen_s(&file, szFileName, L"rb");
#else
    char fname[MAX_PATH];
    memset(fname, 0, MAX_PATH);
    wcstombs(fname, szFileName, MAX_PATH);
    file = fopen(fname, "rb");
#endif
    if (file == NULL)
    {
        return false;
    }

    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    fseek(file, 0, SEEK_SET);
    char* data = new char[size + 1];
    fread_s(data, size, 1, size, file);
    fclose(file);

    wchar_t wBuffer[1024];

    // parse file for root path

    int start = 0, end = 0;
    while (end < size)
    {
        while (end < size && data[end] != '\n')
        {
            end++;
        }

        memcpy(data, &data[start], end - start);
        data[end - start] = 0;
        start = end = end + 1;

        CFixedString<char, 2048> line(data);
        size_t equalsOfs;
        for (equalsOfs = 0; equalsOfs < line.length(); ++equalsOfs)
        {
            if (line[equalsOfs] == '=')
            {
                break;
            }
        }
        if (equalsOfs < line.length())
        {
            CFixedString<char, 256> key;
            CFixedString<wchar_t, 1024> value;

            key.appendAscii(line.c_str(), equalsOfs);
            key.trim();

            SettingsManagerHelpers::ConvertUtf8ToUtf16(line.c_str() + equalsOfs + 1, SettingsManagerHelpers::CWCharBuffer(wBuffer, sizeof(wBuffer)));
            value.append(wBuffer);
            value.trim();

            m_keyValueArray.set(key.c_str(), value.c_str());
        }
    }
    delete[] data;

    return true;
}

//////////////////////////////////////////////////////////////////////////
bool CEngineSettingsManager::StoreEngineSettingsToRegistry()
{
    assert(m_backend);
    return m_backend->StoreEngineSettingsToRegistry();
}

//////////////////////////////////////////////////////////////////////////
void CEngineSettingsManager::LoadEngineSettingsFromRegistry()
{
    assert(m_backend);
    m_backend->LoadEngineSettingsFromRegistry();
}

//////////////////////////////////////////////////////////////////////////
bool CEngineSettingsManager::GetValueByRef(const char* key, SettingsManagerHelpers::CWCharBuffer wbuffer) const
{
    if (wbuffer.getSizeInElements() <= 0)
    {
        return false;
    }

    const SKeyValue* p = m_keyValueArray.find(key);
    if (!p || (p->value.length() + 1) > wbuffer.getSizeInElements())
    {
        wbuffer[0] = 0;
        return false;
    }
    azwcscpy(wbuffer.getPtr(), wbuffer.getSizeInElements(), p->value.c_str());
    return true;
}

//////////////////////////////////////////////////////////////////////////
bool CEngineSettingsManager::GetValueByRef(const char* key, bool& value) const
{
    wchar_t buffer[100];
    if (!GetValueByRef(key, SettingsManagerHelpers::CWCharBuffer(buffer, sizeof(buffer))))
    {
        return false;
    }
    value = (wcscmp(buffer, L"true") == 0);
    return true;
}

//////////////////////////////////////////////////////////////////////////
bool CEngineSettingsManager::GetValueByRef(const char* key, int& value) const
{
    wchar_t buffer[100];
    if (!GetValueByRef(key, SettingsManagerHelpers::CWCharBuffer(buffer, sizeof(buffer))))
    {
        return false;
    }
    value = wcstol(buffer, 0, 10);
    return true;
}

#endif //(CRY_ENABLE_RC_HELPER)
