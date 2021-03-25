/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
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
            void PatchEntityInTemplate(PrefabDomValue& providedPatch, const AZ::EntityId& entityId) override;
            void PatchEntityInTemplate(PrefabDomValue& providedPatch, const EntityAlias& entityAlias, const TemplateId& templateId) override;

            InstanceOptionalReference GetTopMostInstanceInHierarchy(AZ::EntityId entityId);

            void PatchTemplate(PrefabDomValue& providedPatch, const AzToolsFramework::Prefab::TemplateId& templateId) override;

            void ApplyPatchesToInstance(const AZ::EntityId& entityId, PrefabDom& patches, const Instance& instanceToAddPatches) override;

        private:
            void AddPatchesToLink(PrefabDom& patches, Link& link);
            InstanceEntityMapperInterface* m_instanceEntityMapperInterface;
            PrefabSystemComponentInterface* m_prefabSystemComponentInterface;
        };
    }
}
