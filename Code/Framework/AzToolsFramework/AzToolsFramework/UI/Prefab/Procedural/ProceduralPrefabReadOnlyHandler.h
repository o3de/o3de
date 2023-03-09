/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/RTTI/RTTI.h>

#include <AzToolsFramework/Entity/ReadOnly/ReadOnlyEntityBus.h>
#include <AzToolsFramework/Prefab/PrefabFocusNotificationBus.h>

namespace AzToolsFramework
{
    namespace Prefab
    {
        class PrefabFocusPublicInterface;
        class PrefabPublicInterface;

        //! Ensures entities in a procedural prefab are correctly reported as read-only.
        class ProceduralPrefabReadOnlyHandler
            : public ReadOnlyEntityQueryRequestBus::Handler
            , public PrefabFocusNotificationBus::Handler
        {
        public:
            AZ_CLASS_ALLOCATOR(ProceduralPrefabReadOnlyHandler, AZ::SystemAllocator);
            AZ_RTTI(ProceduralPrefabReadOnlyHandler, "{A2D72461-8CA3-45EE-81D2-4976BC0B6AE9}");

            ProceduralPrefabReadOnlyHandler();
            ~ProceduralPrefabReadOnlyHandler() override;

            // PrefabFocusNotificationBus overrides ...
            void OnPrefabEditScopeChanged() override;

            // ReadOnlyEntityQueryRequestBus overrides ...
            void IsReadOnly(const AZ::EntityId& entityId, bool& isReadOnly) override;

        private:
            AzFramework::EntityContextId m_editorEntityContextId = AzFramework::EntityContextId::CreateNull();
            PrefabPublicInterface* m_prefabPublicInterface = nullptr;
            PrefabFocusPublicInterface* m_prefabFocusPublicInterface = nullptr;
        };

    } // namespace Prefab
} // namespace AzToolsFramework
