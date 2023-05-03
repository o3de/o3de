/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzToolsFramework/ViewportUi/ViewportUiRequestBus.h>

namespace AzToolsFramework::ViewportUi::Internal
{
    //! Data class for holding button settings.
    class Button
    {
    public:
        enum class State
        {
            Selected,
            Deselected,
            Disabled
        };

        Button(AZStd::string icon, ButtonId buttonId);
        Button(AZStd::string icon, AZStd::string name, ButtonId buttonId);
        ~Button() = default;

        AZStd::string m_icon; //!< The icon for this button, string path to an image.
        AZStd::string m_name; //!< The name displayed as a label next to the button's icon.
        State m_state = State::Deselected;
        ButtonId m_buttonId;
    };
} // namespace AzToolsFramework::ViewportUi::Internal
