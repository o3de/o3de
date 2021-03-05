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
#include <AzCore/std/containers/vector.h>

#import <AppKit/NSAlert.h>
#import <AppKit/NSPanel.h>

namespace {
    Alert::Buttons fromCocoaResponse(NSModalResponse response, const AZStd::vector<Alert::Buttons>& ids)
    {
        return ids.at(response -NSAlertFirstButtonReturn);
    }
}

Alert::Buttons Alert::ShowMessage(const char* message, int buttons)
{
    return ShowMessage("", message, buttons);
}

Alert::Buttons Alert::ShowMessage(const char* title, const char* message, int buttons)
{
    // macOS alert dialogs don't show title
    AZ_UNUSED(title);

    AZStd::vector<Alert::Buttons> ids;
    NSAlert* alert = [[NSAlert alloc] init];

    // Ensure the alert stay on top of everything by default
    NSPanel* panel = static_cast<NSPanel*>([alert window]);
    panel.floatingPanel = YES;

    [alert setMessageText:[NSString stringWithUTF8String:message]];

    if (buttons & Alert::Ok)
    {
        [alert addButtonWithTitle:@"Ok"];
        ids.push_back(Alert::Ok);
    }
    if (buttons & Alert::No)
    {
        [alert addButtonWithTitle:@"No"];
        ids.push_back(Alert::No);
    }
    if (buttons & Alert::Yes)
    {
        [alert addButtonWithTitle:@"Yes"];
        ids.push_back(Alert::Yes);
    }
    if (buttons & Alert::YesNo)
    {
        {
            [alert addButtonWithTitle:@"No"];
            ids.push_back(Alert::No);
        }
        {
            [alert addButtonWithTitle:@"Yes"];
            ids.push_back(Alert::Yes);
        }
    }

    const NSModalResponse result = [alert runModal];
    return fromCocoaResponse(result, ids);
}

