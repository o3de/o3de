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
#pragma once
// UiClipboard is responsible setting and getting clipboard data for the UI elements in a platform-independent way.

#include <AzCore/std/string/string.h>

class UiClipboard
{
public:
    // UiClipboard strings are UTF8
    static bool SetText(const AZStd::string& text);
    static AZStd::string GetText();
};
