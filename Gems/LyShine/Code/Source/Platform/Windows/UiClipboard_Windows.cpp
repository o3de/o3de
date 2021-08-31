/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
// UiClipboard is responsible setting and getting clipboard data for the UI elements in a platform-independent way.

#include "UiClipboard.h"
#include <AzCore/PlatformIncl.h>
#include <AzCore/std/string/conversions.h>

bool UiClipboard::SetText(const AZStd::string& text)
{
    bool success = false;
    if (OpenClipboard(nullptr))
    {
        if (EmptyClipboard())
        {
            if (text.length() > 0)
            {
                AZStd::wstring wstr;
                AZStd::to_wstring(wstr, text.c_str());
                const SIZE_T buffSize = (wstr.size() + 1) * sizeof(WCHAR);
                if (HGLOBAL hBuffer = GlobalAlloc(GMEM_MOVEABLE, buffSize))
                {
                    auto buffer = static_cast<WCHAR*>(GlobalLock(hBuffer));
                    memcpy_s(buffer, buffSize, wstr.data(), wstr.size() * sizeof(wchar_t));
                    buffer[wstr.size()] = WCHAR(0);
                    GlobalUnlock(hBuffer);
                    SetClipboardData(CF_UNICODETEXT, hBuffer); // Clipboard now owns the hBuffer
                    success = true;
                }
            }
        }
        CloseClipboard();
    }
    return success;
}

AZStd::string UiClipboard::GetText()
{
    AZStd::string outText;
    if (OpenClipboard(nullptr))
    {
        if (HANDLE hText = GetClipboardData(CF_UNICODETEXT))
        {
            const WCHAR* text = static_cast<const WCHAR*>(GlobalLock(hText));
            AZStd::to_string(outText, text);
            GlobalUnlock(hText);
        }
        CloseClipboard();
    }
    return outText;
}
