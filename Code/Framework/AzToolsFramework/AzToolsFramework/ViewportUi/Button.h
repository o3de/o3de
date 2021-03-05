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

#include <AzToolsFramework/ViewportUi/Cluster.h>
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
            Deselected
        };

        explicit Button(AZStd::string icon, ButtonId buttonId);
        ~Button() = default;

        AZStd::string m_icon; //!< The icon for this button, string path to an image.
        State m_state = State::Deselected; 
        ButtonId m_buttonId; 
    };
} // namespace AzToolsFramework::ViewportUi::Internal
