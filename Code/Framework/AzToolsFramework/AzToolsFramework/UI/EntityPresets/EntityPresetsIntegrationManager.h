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
    //! Owns the entity preset feature's presence in the editor.
    //!
    //! Entity presets are ready-made entities - "give me a spot light", "give me a PostFX volume
    //! with exposure control on it" - built in one click instead of create-entity then hunt for the
    //! components then set the two properties that make it useful. The presets themselves are data
    //! (see EntityPresets); this class is only what puts them in front of the user.
    //!
    //! Modelled on Prefab::PrefabIntegrationManager: it exists so the feature has one owner with a
    //! definite lifetime that connects to the action registration hooks, rather than free functions
    //! somebody has to remember to call. Instantiate it once, from the editor's own integration
    //! layer, and destroy it on shutdown.
    class AZTF_API EntityPresetsIntegrationManager final
        : private ActionManagerRegistrationNotificationBus::Handler
    {
    public:
        AZ_CLASS_ALLOCATOR(EntityPresetsIntegrationManager, AZ::SystemAllocator);

        EntityPresetsIntegrationManager();
        ~EntityPresetsIntegrationManager();

    private:
        // ActionManagerRegistrationNotificationBus overrides ...
        // Called in this order: menus are registered, then actions, then the two are bound.
        void OnMenuRegistrationHook() override;
        void OnActionRegistrationHook() override;
        void OnMenuBindingHook() override;
    };
} // namespace AzToolsFramework
