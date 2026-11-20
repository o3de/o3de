/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzToolsFramework/UI/EntityPresets/EntityPresetsIntegrationManager.h>

#include <AzToolsFramework/UI/EntityPresets/EntityPresetMenu.h>
#include <AzToolsFramework/Entity/EntityPresets/EntityPresets.h>

namespace AzToolsFramework
{
    EntityPresetsIntegrationManager::EntityPresetsIntegrationManager()
    {
        ActionManagerRegistrationNotificationBus::Handler::BusConnect();
    }

    EntityPresetsIntegrationManager::~EntityPresetsIntegrationManager()
    {
        ActionManagerRegistrationNotificationBus::Handler::BusDisconnect();
    }

    void EntityPresetsIntegrationManager::OnMenuRegistrationHook()
    {
        // Loaded here rather than in the constructor: by now the active gems and project path are
        // known. This is the first of the three hooks, and the first to need the preset list.
        EntityPresets::Reload();

        EntityPresetMenu::RegisterMenus();
    }

    void EntityPresetsIntegrationManager::OnActionRegistrationHook()
    {
        EntityPresetMenu::RegisterActions();
    }

    void EntityPresetsIntegrationManager::OnMenuBindingHook()
    {
        EntityPresetMenu::BindMenus();
    }
} // namespace AzToolsFramework
