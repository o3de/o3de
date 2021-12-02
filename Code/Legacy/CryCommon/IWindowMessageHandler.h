/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#ifndef _CRY_WINDOW_MESSAGE_HANDLER_H_
#define _CRY_WINDOW_MESSAGE_HANDLER_H_

#if defined(WIN32)
#include "platform.h"
#include <AzCore/PlatformIncl.h>

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
