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

#include <Alert.h>
#include <Windows.h>
#include <cassert>

namespace {
    UINT toWinButtons(int buttons)
    {
        UINT result = 0;

        if (buttons & Alert::Ok)
        {
            result |= MB_OK;
        }
        if (buttons & Alert::YesNo)
        {
            result |= MB_YESNO;
        }

        return result;
    }

    Alert::Buttons fromWinId(UINT id)
    {
        switch (id)
        {
            case IDOK:
                return Alert::Ok;
            case IDYES:
                return Alert::Yes;
            case IDNO:
                return Alert::No;
            default:
                assert(false);
        }

        // Shouldn't reach here
        return Alert::No;
    }
}

Alert::Buttons Alert::ShowMessage(const char* message, int buttons)
{
    return ShowMessage("", message, buttons);
}

Alert::Buttons Alert::ShowMessage(const char* title, const char* message, int buttons)
{
    const UINT result = MessageBox(GetDesktopWindow(), message, title, toWinButtons(buttons));
    return fromWinId(result);
}
