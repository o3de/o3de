/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzToolsFramework/AzToolsFrameworkAPI.h>

#include <AzCore/Memory/SystemAllocator.h>

#include <AzToolsFramework/ActionManager/ActionManagerRegistrationNotificationBus.h>

namespace AzToolsFramework
{
    //! Registers the entity presets' menus and actions with the Action Manager.
    //!
    //! Like Prefab::PrefabIntegrationManager, the editor creates one at startup and destroys it on
    //! shutdown.
    class AZTF_API EntityPresetsIntegrationManager final
        : private ActionManagerRegistrationNotificationBus::Handler
    {
    public:
        AZ_CLASS_ALLOCATOR(EntityPresetsIntegrationManager, AZ::SystemAllocator);

        EntityPresetsIntegrationManager();
        ~EntityPresetsIntegrationManager();

    private:
        // ActionManagerRegistrationNotificationBus overrides ...
        void OnMenuRegistrationHook() override;
        void OnActionRegistrationHook() override;
        void OnMenuBindingHook() override;
    };
} // namespace AzToolsFramework
