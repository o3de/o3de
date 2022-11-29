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
#include <AzToolsFramework/Prefab/Instance/InstanceDomGenerator.h>
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
            AZ_RTTI(InstanceToTemplatePropagator, "{526F7B55-84F9-4EA9-8180-19C5DBCD0103}", InstanceToTemplateInterface);
            AZ_CLASS_ALLOCATOR(InstanceToTemplatePropagator, AZ::SystemAllocator, 0);

            void RegisterInstanceToTemplateInterface();
            void UnregisterInstanceToTemplateInterface();

            bool GenerateDomForEntity(PrefabDom& generatedEntityDom, const AZ::Entity& entity) override;
            bool GenerateDomForInstance(PrefabDom& generatedInstanceDom, const Prefab::Instance& instance) override;
            bool GeneratePatch(PrefabDom& generatedPatch, const PrefabDomValue& initialState, const PrefabDomValue& modifiedState) override;
            bool GeneratePatchForLink(PrefabDom& generatedPatch, const PrefabDom& initialState,
                const PrefabDom& modifiedState, LinkId linkId) override;
            bool PatchEntityInTemplate(PrefabDom& providedPatch, AZ::EntityId entityId) override;

            AZStd::string GenerateEntityAliasPath(AZ::EntityId entityId) override;

            AZ::Dom::Path GenerateEntityPathFromFocusedPrefab(AZ::EntityId entityId) override;

            void AppendEntityAliasToPatchPaths(PrefabDom& providedPatch, AZ::EntityId entityId, const AZStd::string& prefix = "") override;
            void AppendEntityAliasPathToPatchPaths(PrefabDom& providedPatch, const AZStd::string& entityAliasPath) override;

            InstanceOptionalReference GetTopMostInstanceInHierarchy(AZ::EntityId entityId) override;

            bool PatchTemplate(PrefabDomValue& providedPatch, TemplateId templateId, InstanceOptionalConstReference instanceToExclude = AZStd::nullopt) override;

            void ApplyPatchesToInstance(const AZ::EntityId& entityId, PrefabDom& patches, const Instance& instanceToAddPatches) override;

        private:
            InstanceEntityMapperInterface* m_instanceEntityMapperInterface = nullptr;
            PrefabSystemComponentInterface* m_prefabSystemComponentInterface = nullptr;
            InstanceDomGenerator m_instanceDomGenerator;
            static AzFramework::EntityContextId s_editorEntityContextId;
        };
    }
}
