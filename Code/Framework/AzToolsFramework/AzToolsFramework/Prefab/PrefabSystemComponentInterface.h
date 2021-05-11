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

#include <AzCore/Interface/Interface.h>
#include <AzToolsFramework/Prefab/Instance/Instance.h>
#include <AzToolsFramework/Prefab/Link/Link.h>
#include <AzToolsFramework/Prefab/PrefabIdTypes.h>
#include <AzToolsFramework/Prefab/Template/Template.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>

namespace AzToolsFramework
{
    namespace Prefab
    {
        /*!
         * PrefabSystemComponentInterface
         * Interface serviced by the Prefab System Component.
         */
        class PrefabSystemComponentInterface
        {
        public:
            AZ_RTTI(PrefabSystemComponentInterface, "{8E95A029-67F9-4F74-895F-DDBFE29516A0}");

            virtual TemplateReference FindTemplate(const TemplateId& id) = 0;
            virtual LinkReference FindLink(const LinkId& id) = 0;

            virtual TemplateId AddTemplate(const AZ::IO::Path& filePath, PrefabDom prefabDom) = 0;
            virtual void RemoveTemplate(const TemplateId& templateId) = 0;
            virtual void RemoveAllTemplates() = 0;

            virtual LinkId AddLink(const TemplateId& sourceTemplateId, const TemplateId& targetTemplateId,
                PrefabDomValue::MemberIterator& instanceIterator, InstanceOptionalReference instance) = 0;

            //creates a new Link
            virtual LinkId CreateLink(const TemplateId& linkTargetId, const TemplateId& linkSourceId,
                const InstanceAlias& instanceAlias, const PrefabDomReference linkPatch,
                const LinkId& linkId = InvalidLinkId) = 0;

            virtual void RemoveLink(const LinkId& linkId) = 0;

            virtual TemplateId GetTemplateIdFromFilePath(AZ::IO::PathView filePath) const = 0;

            virtual bool IsTemplateDirty(const TemplateId& templateId) = 0;
            virtual void SetTemplateDirtyFlag(const TemplateId& templateId, bool dirty) = 0;

            virtual PrefabDom& FindTemplateDom(TemplateId templateId) = 0;
            virtual void UpdatePrefabTemplate(TemplateId templateId, const PrefabDom& updatedDom) = 0;
            virtual void PropagateTemplateChanges(TemplateId templateId, InstanceOptionalReference instanceToExclude = AZStd::nullopt) = 0;

            virtual AZStd::unique_ptr<Instance> InstantiatePrefab(AZ::IO::PathView filePath) = 0;
            virtual AZStd::unique_ptr<Instance> InstantiatePrefab(const TemplateId& templateId) = 0;
            virtual AZStd::unique_ptr<Instance> CreatePrefab(const AZStd::vector<AZ::Entity*>& entities,
                AZStd::vector<AZStd::unique_ptr<Instance>>&& instancesToConsume, AZ::IO::PathView filePath,
                AZStd::unique_ptr<AZ::Entity> containerEntity = nullptr, bool ShouldCreateLinks = true) = 0;
        };


    } // namespace Prefab
} // namespace AzToolsFramework

