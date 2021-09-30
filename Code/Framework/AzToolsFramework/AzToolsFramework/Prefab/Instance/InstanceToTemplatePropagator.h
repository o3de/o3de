/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once
#include <AzToolsFramework/Prefab/Instance/InstanceToTemplateInterface.h>
#include <AzToolsFramework/Prefab/Instance/InstanceEntityMapperInterface.h>
#include <AzToolsFramework/Prefab/PrefabSystemComponentInterface.h>

namespace AzToolsFramework
{
    namespace Prefab
    {
        class InstanceToTemplatePropagator
            : private InstanceToTemplateInterface
        {
        public:
            void RegisterInstanceToTemplateInterface();
            void UnregisterInstanceToTemplateInterface();

            bool GenerateDomForEntity(PrefabDom& generatedEntityDom, const AZ::Entity& entity) override;
            bool GenerateDomForInstance(PrefabDom& generatedInstanceDom, const Prefab::Instance& instance) override;
            bool GeneratePatch(PrefabDom& generatedPatch, const PrefabDom& initialState, const PrefabDom& modifiedState) override;
            bool GeneratePatchForLink(PrefabDom& generatedPatch, const PrefabDom& initialState,
                const PrefabDom& modifiedState, LinkId linkId) override;
            bool PatchEntityInTemplate(PrefabDom& providedPatch, AZ::EntityId entityId) override;

            void AppendEntityAliasToPatchPaths(PrefabDom& providedPatch, const AZ::EntityId& entityId) override;

            InstanceOptionalReference GetTopMostInstanceInHierarchy(AZ::EntityId entityId) override;

            bool PatchTemplate(PrefabDomValue& providedPatch, TemplateId templateId, bool immediate = false, InstanceOptionalReference instanceToExclude = AZStd::nullopt) override;

            void ApplyPatchesToInstance(const AZ::EntityId& entityId, PrefabDom& patches, const Instance& instanceToAddPatches) override;

            void AddPatchesToLink(const PrefabDom& patches, Link& link);

        private:

            InstanceEntityMapperInterface* m_instanceEntityMapperInterface;
            PrefabSystemComponentInterface* m_prefabSystemComponentInterface;
        };
    }
}
