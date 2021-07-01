/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
