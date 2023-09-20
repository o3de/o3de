/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzToolsFramework/Editor/ActionManagerUtils.h>

#include <AzCore/Settings/SettingsRegistry.h>
#include <AzToolsFramework/ActionManager/HotKey/HotKeyManagerInterface.h>
#include <AzToolsFramework/ActionManager/HotKey/HotKeyWidgetRegistrationInterface.h>

#include <QWidget>

namespace AzToolsFramework
{
    bool IsNewActionManagerEnabled()
    {
        // New Action Manager system is always enabled.
        // This helper will be removed once the legacy system is completely removed.
        return true;
    }

    void AssignWidgetToActionContextHelper(const AZStd::string& actionContextIdentifier, QWidget* widget)
    {
        if (auto hotKeyWidgetRegistrationInterface = AZ::Interface<HotKeyWidgetRegistrationInterface>::Get())
        {
            hotKeyWidgetRegistrationInterface->AssignWidgetToActionContext(actionContextIdentifier, widget);
        }
    }

    void RemoveWidgetFromActionContextHelper(const AZStd::string& actionContextIdentifier, QWidget* widget)
    {
        if (auto hotKeyManagerInterface = AZ::Interface<AzToolsFramework::HotKeyManagerInterface>::Get())
        {
            hotKeyManagerInterface->RemoveWidgetFromActionContext(actionContextIdentifier, widget);
        }
    }

} // namespace AzToolsFramework
