/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzToolsFramework/ActionManager/ActionManager.h>

namespace AzToolsFramework
{
    void ActionManager::RegisterActionContext(
        [[maybe_unused]] QWidget* parentWidget,
        [[maybe_unused]] AZStd::string_view identifier,
        [[maybe_unused]] AZStd::string_view name,
        [[maybe_unused]] AZStd::string_view parentIdentifier)
    {

    }

    void ActionManager::RegisterAction(
        [[maybe_unused]] AZStd::string_view contextIdentifier,
        [[maybe_unused]] AZStd::string_view identifier,
        [[maybe_unused]] AZStd::string_view name,
        [[maybe_unused]] AZStd::string_view description,
        [[maybe_unused]] AZStd::string_view category,
        [[maybe_unused]] AZStd::string_view iconPath,
        [[maybe_unused]] AZStd::function<void()> handler)
    {

    }

} // namespace AzToolsFramework
