/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
// UiClipboard is responsible setting and getting clipboard data for the UI elements in a platform-independent way.

#include "UiClipboard.h"

bool UiClipboard::SetText([[maybe_unused]] const AZStd::string& text)
{
    return false;
}

AZStd::string UiClipboard::GetText()
{
    return {};
}
