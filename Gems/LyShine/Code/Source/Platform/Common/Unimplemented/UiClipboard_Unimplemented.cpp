/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
// UiClipboard is responsible setting and getting clipboard data for the UI elements in a platform-independent way.

#include "LyShine_precompiled.h"
#include "UiClipboard.h"
#include <StringUtils.h>

bool UiClipboard::SetText(const AZStd::string& text)
{
    AZ_UNUSED(text);
    return false;
}

AZStd::string UiClipboard::GetText()
{
    AZStd::string outText;
    return outText;
}
