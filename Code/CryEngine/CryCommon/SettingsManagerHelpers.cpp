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

#if defined(CRY_ENABLE_RC_HELPER)

#include "SettingsManagerHelpers.h"
#include "EngineSettingsManager.h"

#include <codecvt>
#include <locale>
#include <string>

#include <AzCore/std/string/string_view.h>
#include <AzCore/Component/ComponentApplicationBus.h>

#if defined(AZ_PLATFORM_WINDOWS)
#include <windows.h>
#include <shellapi.h> //ShellExecuteW()
#pragma comment(lib, "Shell32.lib")
#endif
#if AZ_TRAIT_OS_PLATFORM_APPLE
#include "AppleSpecific.h"
#endif

bool SettingsManagerHelpers::Utf16ContainsAsciiOnly(const wchar_t* wstr)
{
    while (*wstr)
    {
        if (*wstr > 127 || *wstr < 0)
        {
            return false;
        }
        ++wstr;
    }
    return true;
}


void SettingsManagerHelpers::ConvertUtf16ToUtf8(const wchar_t* src, CCharBuffer dst)
{
    if (dst.getSizeInElements() <= 0)
    {
        return;
    }

    if (src[0] == 0)
    {
        dst[0] = 0;
    }
    else
    {
        const std::codecvt<wchar_t, char, std::mbstate_t>& utf8Utf16Facet = std::use_facet<std::codecvt<wchar_t, char, std::mbstate_t>>(std::locale());
        std::mbstate_t mb{};
        const wchar_t* from_next;
        char* to_next;
        std::codecvt_base::result result = utf8Utf16Facet.out(mb, src, src + wcslen(src), from_next, dst.getPtr(), dst.getPtr() + dst.getSizeInElements(), to_next);
        if (result != std::codecvt_base::ok)
        {
            dst[0] = 0;
        }
        else
        {
            to_next = 0;
        }
    }
}


void SettingsManagerHelpers::ConvertUtf8ToUtf16(const char* src, CWCharBuffer dst)
{
    if (dst.getSizeInElements() <= 0)
    {
        return;
    }

    if (src[0] == 0)
    {
        dst[0] = 0;
    }
    else
    {
        const std::codecvt<wchar_t, char, std::mbstate_t>& utf8Utf16Facet = std::use_facet<std::codecvt<wchar_t, char, std::mbstate_t>>(std::locale());
        std::mbstate_t mb{};
        const char* from_next;
        wchar_t* to_next;
        std::codecvt_base::result result = utf8Utf16Facet.in(mb, src, src + strlen(src), from_next, dst.getPtr(), dst.getPtr() + dst.getSizeInElements(), to_next);
        if (result != std::codecvt_base::ok)
        {
            dst[0] = 0;
        }
        else
        {
            to_next = 0;
        }
    }
}


void SettingsManagerHelpers::GetAsciiFilename(const wchar_t* wfilename, CCharBuffer buffer)
{
    if (buffer.getSizeInElements() <= 0)
    {
        return;
    }

    if (wfilename[0] == 0)
    {
        buffer[0] = 0;
        return;
    }

    if (Utf16ContainsAsciiOnly(wfilename))
    {
        ConvertUtf16ToUtf8(wfilename, buffer);
        return;
    }

#if defined(AZ_PLATFORM_WINDOWS)
    // The path is non-ASCII unicode, so let's resort to short filenames (they are always ASCII-only, I hope)
    wchar_t shortW[MAX_PATH];
    const int bufferCharCount = sizeof(shortW) / sizeof(shortW[0]);
    const int charCount = GetShortPathNameW(wfilename, shortW, bufferCharCount);
    if (charCount <= 0 || charCount >= bufferCharCount)
    {
        buffer[0] = 0;
        return;
    }

    shortW[charCount] = 0;
    if (!Utf16ContainsAsciiOnly(shortW))
    {
        buffer[0] = 0;
        return;
    }

    ConvertUtf16ToUtf8(shortW, buffer);
#else
    buffer[0] = 0;
#endif
}


//////////////////////////////////////////////////////////////////////////
CSettingsManagerTools::CSettingsManagerTools(const wchar_t* szModuleName)
{
    m_pSettingsManager = new CEngineSettingsManager(szModuleName);
}

//////////////////////////////////////////////////////////////////////////
CSettingsManagerTools::~CSettingsManagerTools()
{
    delete m_pSettingsManager;
}

//////////////////////////////////////////////////////////////////////////
bool CSettingsManagerTools::GetInstalledBuildPathUtf16(const int index, SettingsManagerHelpers::CWCharBuffer name, SettingsManagerHelpers::CWCharBuffer path)
{
    return m_pSettingsManager->GetInstalledBuildRootPathUtf16(index, name, path);
}


bool CSettingsManagerTools::GetInstalledBuildPathAscii(const int index, SettingsManagerHelpers::CCharBuffer name, SettingsManagerHelpers::CCharBuffer path)
{
    wchar_t wName[MAX_PATH];
    wchar_t wPath[MAX_PATH];
    if (GetInstalledBuildPathUtf16(index, SettingsManagerHelpers::CWCharBuffer(wName, sizeof(wName)), SettingsManagerHelpers::CWCharBuffer(wPath, sizeof(wPath))))
    {
        SettingsManagerHelpers::GetAsciiFilename(wName, name);
        SettingsManagerHelpers::GetAsciiFilename(wPath, path);
        return true;
    }
    return false;
}



//////////////////////////////////////////////////////////////////////////
static bool FileExists(const wchar_t* filename)
{
#if defined(AZ_PLATFORM_WINDOWS)
    const DWORD dwAttrib = GetFileAttributesW(filename);
    return dwAttrib != INVALID_FILE_ATTRIBUTES && !(dwAttrib & FILE_ATTRIBUTE_DIRECTORY);
#else
    char utf8Filename[MAX_PATH];
    SettingsManagerHelpers::ConvertUtf16ToUtf8(filename, SettingsManagerHelpers::CCharBuffer(utf8Filename, sizeof(utf8Filename)));
    struct stat buffer;
    return (stat(utf8Filename, &buffer) == 0);
#endif
}


//////////////////////////////////////////////////////////////////////////
void CSettingsManagerTools::GetEditorExecutable(SettingsManagerHelpers::CWCharBuffer wbuffer)
{
    if (wbuffer.getSizeInElements() <= 0)
    {
        return;
    }

    AZStd::string_view exePath;
    AZ::ComponentApplicationBus::BroadcastResult(exePath, &AZ::ComponentApplicationRequests::GetExecutableFolder);

    SettingsManagerHelpers::CFixedString<wchar_t, 1024> editorExe;
    editorExe = wbuffer.getPtr();
    editorExe.appendAscii(exePath.data(), exePath.size());

    if (editorExe.length() <= 0)
    {
        wbuffer[0] = 0;
        return;
    }

    bool bFound = false;
    if (Is64bitWindows())
    {
        const size_t len = editorExe.length();
        editorExe.appendAscii("/Editor.exe");
        bFound = FileExists(editorExe.c_str());
        if (!bFound)
        {
            editorExe.setLength(len);
        }
    }

    const size_t sizeToCopy = (editorExe.length() + 1) * sizeof(wbuffer[0]);
    if (!bFound || sizeToCopy > wbuffer.getSizeInBytes())
    {
        wbuffer[0] = 0;
    }
    else
    {
        memcpy(wbuffer.getPtr(), editorExe.c_str(), sizeToCopy);
    }
}


//////////////////////////////////////////////////////////////////////////
bool CSettingsManagerTools::CallEditor(void** pEditorWindow, [[maybe_unused]] void* hParent, const char* pWindowName, const char* pFlag)
{
#if !defined(AZ_PLATFORM_WINDOWS)
    AZ_Assert(false, "CSettingsManagerTools::CallEditor is not supported on this platform!");
    return false;
#else
    HWND window = ::FindWindowA(NULL, pWindowName);
    if (window)
    {
        *pEditorWindow = window;
        return true;
    }
    else
    {
        *pEditorWindow = 0;

        wchar_t buffer[512] = { L'\0' };
        GetEditorExecutable(SettingsManagerHelpers::CWCharBuffer(buffer, sizeof(buffer)));

        SettingsManagerHelpers::CFixedString<wchar_t, 256> wFlags;
        SettingsManagerHelpers::ConvertUtf8ToUtf16(pFlag, wFlags.getBuffer());
        wFlags.setLength(wcslen(wFlags.c_str()));

        if (buffer[0] != '\0')
        {
            INT_PTR hIns = (INT_PTR)ShellExecuteW(NULL, L"open", buffer, wFlags.c_str(), NULL, SW_SHOWNORMAL);
            if (hIns > 32)
            {
                return true;
            }
            else
            {
                MessageBoxA(0, "Editor.exe was not found.\n\nPlease verify CryENGINE root path.", "Error", MB_ICONERROR | MB_OK);
            }
        }
    }

    return false;
#endif
}

//////////////////////////////////////////////////////////////////////////
// Modified version of
// http://msdn.microsoft.com/en-us/library/windows/desktop/ms684139(v=vs.85).aspx
bool CSettingsManagerTools::Is64bitWindows()
{
#if defined(_WIN64)
    // 64-bit programs run only on 64-bit Windows
    return true;
#elif !defined(AZ_PLATFORM_WINDOWS)
    return false;
#else
    // 32-bit programs run on both 32-bit and 64-bit Windows
    static bool bWin64 = false;
    static bool bOnce = true;
    if (bOnce)
    {
        typedef BOOL (WINAPI * LPFN_ISWOW64PROCESS)(HANDLE, PBOOL);
        LPFN_ISWOW64PROCESS fnIsWow64Process = (LPFN_ISWOW64PROCESS)GetProcAddress(GetModuleHandleA("kernel32"), "IsWow64Process");
        if (fnIsWow64Process != NULL)
        {
            BOOL itIsWow64Process = FALSE;
            if (fnIsWow64Process(GetCurrentProcess(), &itIsWow64Process))
            {
                bWin64 = (itIsWow64Process == TRUE);
            }
        }
        bOnce = false;
    }
    return bWin64;
#endif
}

#endif // #if defined(CRY_ENABLE_RC_HELPER)

// eof
