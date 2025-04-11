/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzToolsFramework/Prefab/Overrides/PrefabOverrideHandler.h>
#include <AzToolsFramework/Prefab/Overrides/PrefabOverridePublicInterface.h>

namespace AzToolsFramework
{
    namespace Prefab
    {
        class InstanceEntityMapperInterface;
        class InstanceToTemplateInterface;
        class PrefabFocusInterface;
        class PrefabSystemComponentInterface;

        class PrefabOverridePublicHandler : public PrefabOverridePublicRequestBus::Handler
        {
        public:
            PrefabOverridePublicHandler();
            virtual ~PrefabOverridePublicHandler();

            static void Reflect(AZ::ReflectContext* context);

        private:
            bool AreOverridesPresent(AZ::EntityId entityId, AZStd::string_view relativePathFromEntity = {}) override;
            bool AreComponentOverridesPresent(AZ::EntityComponentIdPair entityComponentIdPair) override;
            AZStd::optional<OverrideType> GetEntityOverrideType(AZ::EntityId entityId) override;
            AZStd::optional<OverrideType> GetComponentOverrideType(const AZ::EntityComponentIdPair& entityComponentIdPair) override;
            bool RevertOverrides(AZ::EntityId entityId, AZStd::string_view relativePathFromEntity = {}) override;
            bool RevertComponentOverrides(const AZ::EntityComponentIdPair& entityComponentIdPair) override;
            bool ApplyComponentOverrides(const AZ::EntityComponentIdPair& entityComponentIdPair) override;
            AZStd::pair<AZ::Dom::Path, LinkId> GetEntityPathAndLinkIdFromFocusedPrefab(AZ::EntityId entityId);
            AZStd::pair<AZ::Dom::Path, LinkId> GetComponentPathAndLinkIdFromFocusedPrefab(const AZ::EntityComponentIdPair& entityComponentIdPair);

            PrefabOverrideHandler m_prefabOverrideHandler;

            InstanceEntityMapperInterface* m_instanceEntityMapperInterface = nullptr;
            InstanceToTemplateInterface* m_instanceToTemplateInterface = nullptr;
            PrefabFocusInterface* m_prefabFocusInterface = nullptr;
            PrefabSystemComponentInterface* m_prefabSystemComponentInterface = nullptr;
        };
    } // namespace Prefab
} // namespace AzToolsFramework
