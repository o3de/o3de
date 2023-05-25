/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/Outcome/Outcome.h>
#include <AzToolsFramework/Entity/EntityTypes.h>
#include <AzToolsFramework/Prefab/Instance/Instance.h>
#include <AzToolsFramework/Prefab/Spawnable/ComponentRequirementsValidator.h>
#include <AzToolsFramework/Prefab/Spawnable/EditorOnlyEntityHandler/EditorOnlyEntityHandler.h>
#include <AzToolsFramework/Prefab/Spawnable/EditorOnlyEntityHandler/UiEditorOnlyEntityHandler.h>
#include <AzToolsFramework/Prefab/Spawnable/EditorOnlyEntityHandler/WorldEditorOnlyEntityHandler.h>
#include <AzToolsFramework/Prefab/Spawnable/PrefabProcessor.h>

namespace AZ
{
    class ReflectContext;
}

namespace AzToolsFramework::Components
{
    class EditorComponentBase;
}

namespace AzToolsFramework::Prefab::PrefabConversionUtils
{
    class EditorInfoRemover
        : public PrefabProcessor
    {
    public:
        AZ_CLASS_ALLOCATOR(EditorInfoRemover, AZ::SystemAllocator);
        AZ_RTTI(AzToolsFramework::Prefab::PrefabConversionUtils::EditorInfoRemover,
            "{50B48C7E-C9DE-48DE-8438-1A186A8EEAC8}", PrefabProcessor);

        ~EditorInfoRemover() override;

        void Process(PrefabProcessorContext& prefabProcessorContext) override;

        using RemoveEditorInfoResult = AZ::Outcome<void, AZStd::string>;
        RemoveEditorInfoResult RemoveEditorInfo(
            PrefabDocument& prefab,
            AZ::SerializeContext* serializeContext,
            PrefabProcessorContext& prefabProcessorContext);

        static void Reflect(AZ::ReflectContext* context);

     protected:
        static void GetEntitiesFromInstance(AzToolsFramework::Prefab::Instance& instance, EntityList& hierarchyEntities);

        static bool ReadComponentAttribute(
            AZ::Component* component,
            AZ::Edit::Attribute* attribute,
            AZStd::vector<AZ::Crc32>& attributeTags);

        void SetEditorOnlyEntityHandlerFromCandidates(const EntityList& entities);

        bool HasValidEditorOnlyHandler() const;

        void ClearEditorOnlyEntityIds();

        void AddEntityIdIfEditorOnly(AZ::Entity* entity);

        using RemoveEditorOnlyEntitiesResult = AZ::Outcome<void, AZStd::string>;
        RemoveEditorOnlyEntitiesResult RemoveEditorOnlyEntities(EntityList& entities);

        using ExportEntityResult = AZ::Outcome<AZStd::unique_ptr<AZ::Entity>, AZStd::string>;
        ExportEntityResult ExportEntity(AZ::Entity* sourceEntity, PrefabProcessorContext& context);

        using ResolveExportedComponentResult = AZ::Outcome<AZ::ExportedComponent, AZStd::string>;
        ResolveExportedComponentResult ResolveExportedComponent(
            AZ::ExportedComponent& component, PrefabProcessorContext& prefabProcessorContext);

        using ShouldExportResult = AZ::Outcome<bool, AZStd::string>;
        ShouldExportResult ShouldExportComponent(
            AZ::Component* component,
            PrefabProcessorContext& prefabProcessorContext) const;

        using BuildGameEntityResult = AZ::Outcome<void, AZStd::string>;
        BuildGameEntityResult BuildGameEntity(
            AzToolsFramework::Components::EditorComponentBase* editorComponent,
            AZ::Entity* sourceEntity,
            AZ::Entity* exportEntity
        );

        using ExportComponentResult = AZ::Outcome<void, AZStd::string>;
        ExportComponentResult ExportComponent(
            AZ::Component* component,
            PrefabProcessorContext& prefabProcessorContext,
            AZ::Entity* sourceEntity,
            AZ::Entity* exportEntity);

        AZ::SerializeContext* m_serializeContext{ nullptr };
        EditorOnlyEntityHandler* m_editorOnlyEntityHandler{ nullptr };
        EditorOnlyEntityHandlers m_editorOnlyEntityHandlerCandidates{
            aznew WorldEditorOnlyEntityHandler(),
            aznew UiEditorOnlyEntityHandler() };
        ComponentRequirementsValidator m_componentRequirementsValidator;
        EntityIdSet m_editorOnlyEntityIds;
    };
} // namespace AzToolsFramework::Prefab::PrefabConversionUtils
