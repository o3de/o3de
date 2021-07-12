/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "AzToolsFramework_precompiled.h"

#include <AzToolsFramework/ViewportUi/Button.h>

namespace AzToolsFramework::ViewportUi::Internal
{
    Button::Button(AZStd::string icon, ButtonId buttonId)
        : m_icon(AZStd::move(icon))
        , m_buttonId(buttonId)
    {
    }

    Button::Button(AZStd::string icon, AZStd::string name, ButtonId buttonId)
        : m_icon(AZStd::move(icon))
        , m_name(AZStd::move(name))
        , m_buttonId(buttonId)
    {
    }
} // namespace AzToolsFramework::ViewportUi::Internal
