/* Copyright (c) Contributors to the Open 3D Engine Project
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#pragma once

#include <AzToolsFramework/Prefab/PrefabSystemComponentInterface.h>
#include <gmock/gmock.h>

namespace PrefabDependencyViewer
{
    using PrefabSystemComponentInterface = AzToolsFramework::Prefab::PrefabSystemComponentInterface;
    using TemplateReference              = AzToolsFramework::Prefab::TemplateReference;
    using TemplateId                     = AzToolsFramework::Prefab::TemplateId;
    using LinkReference                  = AzToolsFramework::Prefab::LinkReference;
    using LinkId                         = AzToolsFramework::Prefab::LinkId;
    using PrefabDom                      = AzToolsFramework::Prefab::PrefabDom;
    using PrefabDomValue                 = AzToolsFramework::Prefab::PrefabDomValue;
    using InstanceOptionalReference      = AzToolsFramework::Prefab::InstanceOptionalReference;
    using InstanceAlias                  = AzToolsFramework::Prefab::InstanceAlias;
    using PrefabDomConstReference        = AzToolsFramework::Prefab::PrefabDomConstReference;
    using Instance                       = AzToolsFramework::Prefab::Instance;

    class MockPrefabSystemComponent : public PrefabSystemComponentInterface
    {
    public:
        AZ_CLASS_ALLOCATOR(MockPrefabSystemComponent, AZ::SystemAllocator, 0);
        MOCK_METHOD1(FindTemplate, TemplateReference(const TemplateId&));
        MOCK_METHOD1(FindLink, LinkReference(const LinkId&));

        MOCK_METHOD2(AddTemplate, TemplateId(const AZ::IO::Path&, PrefabDom));
        MOCK_METHOD1(RemoveTemplate, void(const TemplateId&));
        MOCK_METHOD0(RemoveAllTemplates, void());

        MOCK_METHOD4(AddLink, LinkId(const TemplateId&,
                                     const TemplateId&,
                                     PrefabDomValue::MemberIterator&,
                                     InstanceOptionalReference));

        MOCK_METHOD5(CreateLink2, LinkId(const TemplateId&,
                                        const TemplateId&,
                                        const InstanceAlias&,
                                        const PrefabDomConstReference,
                                        const LinkId&));

        // Mock Method don't come with a default value
        LinkId CreateLink(
            const TemplateId& linkTargetId,
            const TemplateId& linkSourceId,
            const InstanceAlias& instanceAlias,
            const PrefabDomConstReference linkPatches,
            const LinkId& linkId = AzToolsFramework::Prefab::InvalidLinkId)
        {
            return CreateLink2(linkTargetId, linkSourceId, instanceAlias, linkPatches, linkId);
        }

        MOCK_METHOD1(RemoveLink, void(const LinkId&));
        MOCK_CONST_METHOD1(GetTemplateIdFromFilePath, TemplateId(AZ::IO::PathView));
        MOCK_METHOD1(IsTemplateDirty, bool(const TemplateId&));
        MOCK_METHOD2(SetTemplateDirtyFlag, void(const TemplateId&, bool));

        MOCK_METHOD1(FindTemplateDom, PrefabDom&(TemplateId));
        MOCK_METHOD2(UpdatePrefabTemplate, void(TemplateId, const PrefabDom&));
        MOCK_METHOD2(PropagateTemplateChanges, void(TemplateId, InstanceOptionalReference));

        void PropagateTemplateChanges2(TemplateId templateId, InstanceOptionalReference instanceToExclude = AZStd::nullopt) {
            return PropagateTemplateChanges(templateId, instanceToExclude);
        }

        MOCK_METHOD1(InstantiatePrefab, AZStd::unique_ptr<Instance>(AZ::IO::PathView));
        MOCK_METHOD1(InstantiatePrefab, AZStd::unique_ptr<Instance>(const TemplateId&));
        MOCK_METHOD5(CreatePrefab2, AZStd::unique_ptr<Instance>(
                                        const AZStd::vector<AZ::Entity*>&,
                                        AZStd::vector<AZStd::unique_ptr<Instance>>&&,
                                        AZ::IO::PathView,
                                        AZStd::unique_ptr<AZ::Entity>,
                                        bool));

        AZStd::unique_ptr<Instance> CreatePrefab(
            const AZStd::vector<AZ::Entity*>& entities,
            AZStd::vector<AZStd::unique_ptr<Instance>>&& instancesToConsume,
            AZ::IO::PathView filePath,
            AZStd::unique_ptr<AZ::Entity> containerEntity = nullptr,
            bool ShouldCreateLinks = true)
        {
            // Check whether this would destroy the pointer in a higher scope.
            return CreatePrefab2(entities, AZStd::move(instancesToConsume),
                filePath, std::move(containerEntity), ShouldCreateLinks);
        }

    };
} // namespace PrefabDependencyViewer
