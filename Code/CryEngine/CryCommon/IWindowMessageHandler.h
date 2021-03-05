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

#ifndef _CRY_WINDOW_MESSAGE_HANDLER_H_
#define _CRY_WINDOW_MESSAGE_HANDLER_H_
#include "platform.h"
#if defined(WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

// Summary:
//      Window message handler for Windows OS
struct IWindowMessageHandler
{
    // Summary:
    //      The low-level pre-process message handler for Windows
    //      This is called before TranslateMessage/DispatchMessage (which will eventually end up in the HandleMessage handler)
    //      Typically, do not implement this function.
    virtual void PreprocessMessage([[maybe_unused]] HWND hWnd, [[maybe_unused]] UINT uMsg, [[maybe_unused]] WPARAM wParam, [[maybe_unused]] LPARAM lParam) {}

    // Summary:
    //      The low-level window message handler for Windows
    //      The return value specifies if the implementation wants to modify the message handling result
    //      When returning true, the desired result value should be written through the pResult pointer
    //      When returning false, the value stored through pResult (if any) is ignored
    //      If more than one implementation write different results, the behavior is undefined
    //      If none of the implementations write any result, the default OS result will be used instead
    //      In general, return false if the handler doesn't care about the message, or only uses it for informational purposes
    virtual bool HandleMessage([[maybe_unused]] HWND hWnd, [[maybe_unused]] UINT uMsg, [[maybe_unused]] WPARAM wParam, [[maybe_unused]] LPARAM lParam, [[maybe_unused]] LRESULT* pResult) { return false; }
};
#else
// Summary:
//      Dummy window message handler
//      This is used for platforms that don't use window message handlers
struct IWindowMessageHandler
{
};
#endif
#endif
