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
        // Reading the presets here rather than in the constructor: this hook runs once the
        // application is up, so the settings registry knows which gems are active and the project
        // path resolves.
        //
        // This is the earliest of the three hooks this class handles (menu registration, then
        // action registration, then binding), and it is also the first thing that needs the preset
        // list - there is one submenu per category. Loading here rather than letting the lazy load
        // inside EntityPresets::All() do it keeps the disk read at a point we chose.
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
