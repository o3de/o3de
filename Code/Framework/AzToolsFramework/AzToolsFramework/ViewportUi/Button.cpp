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
