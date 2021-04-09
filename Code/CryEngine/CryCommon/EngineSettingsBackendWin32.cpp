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


#include "EngineSettingsBackendWin32.h"

#ifdef CRY_ENABLE_RC_HELPER

#include "AzCore/PlatformDef.h"

#ifdef AZ_PLATFORM_WINDOWS

#include "EngineSettingsManager.h"

#include "platform.h"
#include <windows.h>

#define REG_SOFTWARE            L"Software\\"
#define REG_COMPANY_NAME        L"Amazon\\"
#define REG_PRODUCT_NAME        L"Lumberyard\\"
#define REG_SETTING             L"Settings\\"
#define REG_BASE_SETTING_KEY  REG_SOFTWARE REG_COMPANY_NAME REG_PRODUCT_NAME REG_SETTING

EXTERN_C IMAGE_DOS_HEADER __ImageBase;

using namespace SettingsManagerHelpers;

static bool g_bWindowQuit;
static CEngineSettingsManager* g_pThis = 0;
static const unsigned int IDC_hEditRootPath = 100;
static const unsigned int IDC_hBtnBrowse = 101;

namespace
{
    class RegKey
    {
    public:
        RegKey(const wchar_t* key, bool writeable);
        ~RegKey();
        void* pKey;
    };

    RegKey::RegKey(const wchar_t* key, bool writeable)
    {
        HKEY  hKey;
        LONG result;
        if (writeable)
        {
            result = RegCreateKeyExW(HKEY_CURRENT_USER, key, 0, 0, 0, KEY_WRITE, 0, &hKey, 0);
        }
        else
        {
            result = RegOpenKeyExW(HKEY_CURRENT_USER, key, 0, KEY_READ, &hKey);
        }
        pKey = hKey;
    }

    RegKey::~RegKey()
    {
        RegCloseKey((HKEY)pKey);
    }
}

CEngineSettingsBackendWin32::CEngineSettingsBackendWin32(CEngineSettingsManager* parent, const wchar_t* moduleName)
    : CEngineSettingsBackend(parent, moduleName)
{
}

std::wstring CEngineSettingsBackendWin32::GetModuleFilePath() const
{
    wchar_t szFilename[_MAX_PATH];
    GetModuleFileNameW((HINSTANCE)&__ImageBase, szFilename, _MAX_PATH);
    wchar_t drive[_MAX_DRIVE];
    wchar_t dir[_MAX_DIR];
    wchar_t fname[_MAX_FNAME];
    wchar_t ext[1] = L"";
    _wsplitpath_s(szFilename, drive, dir, fname, ext);
    _wmakepath_s(szFilename, drive, dir, fname, L"ini");
    return szFilename;
}

bool CEngineSettingsBackendWin32::GetModuleSpecificStringEntryUtf16(const char* key, CWCharBuffer wbuffer)
{
    CFixedString<wchar_t, 256> s = REG_BASE_SETTING_KEY;
    s.append(moduleName().c_str());
    RegKey superKey(s.c_str(), false);
    if (!superKey.pKey)
    {
        wbuffer[0] = 0;
        return false;
    }
    if (!GetRegValue(superKey.pKey, key, wbuffer))
    {
        wbuffer[0] = 0;
        return false;
    }

    return true;
}

bool CEngineSettingsBackendWin32::GetModuleSpecificIntEntry(const char* key, int& value)
{
    CFixedString<wchar_t, 256> s = REG_BASE_SETTING_KEY;
    s.append(moduleName().c_str());
    RegKey superKey(s.c_str(), false);
    if (!superKey.pKey)
    {
        return false;
    }
    if (!GetRegValue(superKey.pKey, key, value))
    {
        value = 0;
        return false;
    }

    return true;
}

bool CEngineSettingsBackendWin32::GetModuleSpecificBoolEntry(const char* key, bool& value)
{
    CFixedString<wchar_t, 256> s = REG_BASE_SETTING_KEY;
    s.append(moduleName().c_str());
    RegKey superKey(s.c_str(), false);
    if (!superKey.pKey)
    {
        return false;
    }
    if (!GetRegValue(superKey.pKey, key, value))
    {
        value = false;
        return false;
    }

    return true;
}

bool CEngineSettingsBackendWin32::SetModuleSpecificStringEntryUtf16(const char* key, const wchar_t* str)
{
    CFixedString<wchar_t, 256> s = REG_BASE_SETTING_KEY;
    s.append(moduleName().c_str());
    RegKey superKey(s.c_str(), true);
    if (superKey.pKey)
    {
        return SetRegValue(superKey.pKey, key, str);
    }
    return false;
}

bool CEngineSettingsBackendWin32::SetModuleSpecificIntEntry(const char* key, const int& value)
{
    CFixedString<wchar_t, 256> s = REG_BASE_SETTING_KEY;
    s.append(moduleName().c_str());
    RegKey superKey(s.c_str(), true);
    if (superKey.pKey)
    {
        return SetRegValue(superKey.pKey, key, value);
    }
    return false;
}

bool CEngineSettingsBackendWin32::SetModuleSpecificBoolEntry(const char* key, const bool& value)
{
    CFixedString<wchar_t, 256> s = REG_BASE_SETTING_KEY;
    s.append(moduleName().c_str());
    RegKey superKey(s.c_str(), true);
    if (superKey.pKey)
    {
        return SetRegValue(superKey.pKey, key, value);
    }
    return false;
}

bool CEngineSettingsBackendWin32::GetInstalledBuildRootPathUtf16(const int index, CWCharBuffer name, CWCharBuffer path)
{
    RegKey key(REG_BASE_SETTING_KEY L"LumberyardExport\\ProjectBuilds", false);
    if (key.pKey)
    {
        DWORD type;
        DWORD nameSizeInBytes = DWORD(name.getSizeInBytes());
        DWORD pathSizeInBytes = DWORD(path.getSizeInBytes());
        LONG result = RegEnumValueW((HKEY)key.pKey, index, name.getPtr(), &nameSizeInBytes, NULL, &type, (BYTE*)path.getPtr(), &pathSizeInBytes);
        if (result == ERROR_SUCCESS)
        {
            return true;
        }
    }
    return false;
}

bool CEngineSettingsBackendWin32::StoreEngineSettingsToRegistry()
{
    // make sure the path in registry exists
    {
        RegKey key0(REG_SOFTWARE REG_COMPANY_NAME, true);
        if (!key0.pKey)
        {
            RegKey software(REG_SOFTWARE, true);
            HKEY hKey;
            RegCreateKeyW((HKEY)software.pKey, REG_COMPANY_NAME, &hKey);
            if (!hKey)
            {
                return false;
            }
        }

        RegKey key1(REG_SOFTWARE REG_COMPANY_NAME REG_PRODUCT_NAME, true);
        if (!key1.pKey)
        {
            RegKey softwareCompany(REG_SOFTWARE REG_COMPANY_NAME, true);
            HKEY hKey;
            RegCreateKeyW((HKEY)softwareCompany.pKey, REG_COMPANY_NAME, &hKey);
            if (!hKey)
            {
                return false;
            }
        }

        RegKey key2(REG_BASE_SETTING_KEY, true);
        if (!key2.pKey)
        {
            RegKey softwareCompanyProduct(REG_SOFTWARE REG_COMPANY_NAME REG_PRODUCT_NAME, true);
            HKEY hKey;
            RegCreateKeyW((HKEY)key2.pKey, REG_SETTING, &hKey);
            if (!hKey)
            {
                return false;
            }
        }
    }

    bool bRet = true;

    RegKey key(REG_BASE_SETTING_KEY, true);
    if (!key.pKey)
    {
        bRet = false;
    }
    else
    {
        wchar_t buffer[1024];

        // ResourceCompiler Specific

        if (parent()->GetValueByRef("RC_ShowWindow", SettingsManagerHelpers::CWCharBuffer(buffer, sizeof(buffer))))
        {
            const bool b = wcscmp(buffer, L"true") == 0;
            SetRegValue(key.pKey, "RC_ShowWindow", b);
        }

        if (parent()->GetValueByRef("RC_HideCustom", SettingsManagerHelpers::CWCharBuffer(buffer, sizeof(buffer))))
        {
            const bool b = wcscmp(buffer, L"true") == 0;
            SetRegValue(key.pKey, "RC_HideCustom", b);
        }

        if (parent()->GetValueByRef("RC_Parameters", SettingsManagerHelpers::CWCharBuffer(buffer, sizeof(buffer))))
        {
            SetRegValue(key.pKey, "RC_Parameters", buffer);
        }

        if (parent()->GetValueByRef("RC_EnableSourceControl", SettingsManagerHelpers::CWCharBuffer(buffer, sizeof(buffer))))
        {
            const bool b = wcscmp(buffer, L"true") == 0;
            SetRegValue(key.pKey, "RC_EnableSourceControl", b);
        }
    }

    return bRet;
}

void CEngineSettingsBackendWin32::LoadEngineSettingsFromRegistry()
{
    wchar_t buffer[1024];

    bool bResult;

    // Engine Specific (Deprecated value)
    RegKey key(REG_BASE_SETTING_KEY, false);
    if (key.pKey)
    {
        if (GetRegValue(key.pKey, "RootPath", SettingsManagerHelpers::CWCharBuffer(buffer, sizeof(buffer))))
        {
            parent()->SetKey("ENG_RootPath", buffer);
        }

        // Engine Specific
        if (GetRegValue(key.pKey, "ENG_RootPath", SettingsManagerHelpers::CWCharBuffer(buffer, sizeof(buffer))))
        {
            parent()->SetKey("ENG_RootPath", buffer);
        }

        // ResourceCompiler Specific
        if (GetRegValue(key.pKey, "RC_ShowWindow", bResult))
        {
            parent()->SetKey("RC_ShowWindow", bResult);
        }
        if (GetRegValue(key.pKey, "RC_HideCustom", bResult))
        {
            parent()->SetKey("RC_HideCustom", bResult);
        }
        if (GetRegValue(key.pKey, "RC_Parameters", SettingsManagerHelpers::CWCharBuffer(buffer, sizeof(buffer))))
        {
            parent()->SetKey("RC_Parameters", buffer);
        }
        if (GetRegValue(key.pKey, "RC_EnableSourceControl", bResult))
        {
            parent()->SetKey("RC_EnableSourceControl", bResult);
        }
    }
}

bool CEngineSettingsBackendWin32::SetRegValue(void* key, const char* valueName, const wchar_t* value)
{
    CFixedString<wchar_t, 256> name;
    name.appendAscii(valueName);

    size_t const sizeInBytes = (wcslen(value) + 1) * sizeof(value[0]);
    return (ERROR_SUCCESS == RegSetValueExW((HKEY)key, name.c_str(), 0, REG_SZ, (BYTE*)value, DWORD(sizeInBytes)));
}

bool CEngineSettingsBackendWin32::SetRegValue(void* key, const char* valueName, bool value)
{
    CFixedString<wchar_t, 256> name;
    name.appendAscii(valueName);

    DWORD dwVal = value;
    return (ERROR_SUCCESS == RegSetValueExW((HKEY)key, name.c_str(), 0, REG_DWORD, (BYTE*)&dwVal, sizeof(dwVal)));
}

bool CEngineSettingsBackendWin32::SetRegValue(void* key, const char* valueName, int value)
{
    CFixedString<wchar_t, 256> name;
    name.appendAscii(valueName);

    DWORD dwVal = value;
    return (ERROR_SUCCESS == RegSetValueExW((HKEY)key, name.c_str(), 0, REG_DWORD, (BYTE*)&dwVal, sizeof(dwVal)));
}

bool CEngineSettingsBackendWin32::GetRegValue(void* key, const char* valueName, CWCharBuffer wbuffer)
{
    if (wbuffer.getSizeInElements() <= 0)
    {
        return false;
    }

    CFixedString<wchar_t, 256> name;
    name.appendAscii(valueName);

    DWORD type;
    DWORD sizeInBytes = DWORD(wbuffer.getSizeInBytes());
    if (ERROR_SUCCESS != RegQueryValueExW((HKEY)key, name.c_str(), NULL, &type, (BYTE*)wbuffer.getPtr(), &sizeInBytes))
    {
        wbuffer[0] = 0;
        return false;
    }

    const size_t sizeInElements = sizeInBytes / sizeof(wbuffer[0]);
    if (sizeInElements > wbuffer.getSizeInElements()) // paranoid check
    {
        wbuffer[0] = 0;
        return false;
    }

    // According to MSDN documentation for RegQueryValueEx(), strings returned by the function
    // are not zero-terminated sometimes, so we need to terminate them by ourselves.
    if (wbuffer[sizeInElements - 1] != 0)
    {
        if (sizeInElements >= wbuffer.getSizeInElements())
        {
            // No space left to put terminating zero character
            wbuffer[0] = 0;
            return false;
        }
        wbuffer[sizeInElements] = 0;
    }

    return true;
}

bool CEngineSettingsBackendWin32::GetRegValue(void* key, const char* valueName, bool& value)
{
    CFixedString<wchar_t, 256> name;
    name.appendAscii(valueName);

    // Open the appropriate registry key
    DWORD type, dwVal = 0, size = sizeof(dwVal);
    bool res = (ERROR_SUCCESS == RegQueryValueExW((HKEY)key, name.c_str(), NULL, &type, (BYTE*)&dwVal, &size));
    if (res)
    {
        value = (dwVal != 0);
    }
    else
    {
        wchar_t buffer[100];
        res = GetRegValue(key, valueName, CWCharBuffer(buffer, sizeof(buffer)));
        if (res)
        {
            value = (wcscmp(buffer, L"true") == 0);
        }
    }
    return res;
}

bool CEngineSettingsBackendWin32::GetRegValue(void* key, const char* valueName, int& value)
{
    CFixedString<wchar_t, 256> name;
    name.appendAscii(valueName);

    // Open the appropriate registry key
    DWORD type, dwVal = 0, size = sizeof(dwVal);

    bool res = (ERROR_SUCCESS == RegQueryValueExW((HKEY)key, name.c_str(), NULL, &type, (BYTE*)&dwVal, &size));
    if (res)
    {
        value = dwVal;
    }

    return res;
}

#endif // AZ_PLATFORM_WINDOWS
#endif // CRY_ENABLE_RC_HELPER
