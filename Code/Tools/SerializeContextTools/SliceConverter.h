/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/IO/Path/Path_fwd.h>
#include <AzCore/JSON/document.h>
#include <AzCore/JSON/stringbuffer.h>
#include <AzCore/Serialization/Json/JsonSerialization.h>
#include <AzCore/std/utils.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/string/string.h>
#include <AzToolsFramework/Prefab/PrefabSystemComponent.h>

#include <Converter.h>

namespace AZ
{
    class CommandLine;
    class Entity;
    class ModuleEntity;
    class SerializeContext;
    struct Uuid;

    namespace SerializeContextTools
    {
        class Application;

        class SliceConverter : public Converter
        {
        public:
            bool ConvertSliceFiles(Application& application);

        private:
            // When converting slice entities, especially for nested slices, we need to keep track of the original
            // entity ID, the entity alias it uses in the prefab, and which template and nested instance path it maps to.
            // As we encounter each instanced entity ID, we can look it up in this structure and use this to determine how to properly
            // add it to the correct place in the hierarchy.
            struct SliceEntityMappingInfo
            {
                SliceEntityMappingInfo(
                    AzToolsFramework::Prefab::TemplateId templateId,
                    AzToolsFramework::Prefab::EntityAlias entityAlias,
                    bool isMetadataEntity = false)
                    : m_templateId(templateId)
                    , m_entityAlias(entityAlias)
                    , m_isMetadataEntity(isMetadataEntity)
                {
                }

                AzToolsFramework::Prefab::TemplateId m_templateId;
                AzToolsFramework::Prefab::EntityAlias m_entityAlias;
                AZStd::vector<AzToolsFramework::Prefab::InstanceAlias> m_nestedInstanceAliases;
                bool m_isMetadataEntity{ false };
            };

            bool ConnectToAssetProcessor();
            void DisconnectFromAssetProcessor();
            bool NeedAssetProcessor() const;

            bool ConvertSliceFile(AZ::SerializeContext* serializeContext, const AZStd::string& slicePath, bool isDryRun);
            bool ConvertSliceToPrefab(
                AZ::SerializeContext* serializeContext,  AZ::IO::PathView outputPath, bool isDryRun, AZ::Entity* rootEntity);
            void FixPrefabEntities(AZ::Entity& containerEntity, SliceComponent::EntityList& sliceEntities);
            bool ConvertNestedSlices(
                SliceComponent* sliceComponent, AzToolsFramework::Prefab::Instance* sourceInstance,
                AZ::SerializeContext* serializeContext, bool isDryRun);
            bool ConvertSliceInstance(
                AZ::SliceComponent::SliceInstance& instance, AZ::Data::Asset<AZ::SliceAsset>& sliceAsset,
                AzToolsFramework::Prefab::TemplateReference nestedTemplate,
                AzToolsFramework::Prefab::Instance* topLevelInstance,
                AZ::SerializeContext* serializeContext);
            void UpdateCachedTransform(const AZ::Entity& entity);
            void SetParentEntity(const AZ::Entity& entity, const AZ::EntityId& parentId, bool onlySetIfInvalid);
            void PrintPrefab(AzToolsFramework::Prefab::TemplateId templateId);
            bool SavePrefab(AZ::IO::PathView outputPath, AzToolsFramework::Prefab::TemplateId templateId);
            void UpdateSliceEntityInstanceMappings(
                const AZ::SliceComponent::EntityIdToEntityIdMap& sliceEntityIdMap,
                const AZStd::string& currentInstanceAlias);
            AZStd::string GetInstanceAlias(const AZ::SliceComponent::SliceInstance& instance);

            void RemapIdReferences(
                const AZStd::unordered_map<AZ::EntityId, SliceEntityMappingInfo>& idMapper,
                AzToolsFramework::Prefab::Instance* topLevelInstance,
                AzToolsFramework::Prefab::Instance* nestedInstance,
                SliceComponent::InstantiatedContainer* instantiatedEntities,
                SerializeContext* context);

            AZ::IO::Path m_projectPath;

            // Track all of the entity IDs created and associate them with enough conversion information to know how to place the
            // entities in the correct place in the prefab hierarchy and fix up parent entity ID mappings to work with the nested
            // prefab schema.
            AZStd::unordered_map<AZ::EntityId, SliceEntityMappingInfo> m_aliasIdMapper;

            // When we don't use the asset processor, will store all of the discovered slices path
            // with their absolute path and the relative posix path used by asset hint
            AZStd::unordered_map<AZStd::string, AZStd::string> m_relativeToAbsoluteSlicePaths;

            // Track all of the created prefab template IDs on a slice conversion so that they can get removed at the end of the
            // conversion for that file.
            AZStd::unordered_set<AzToolsFramework::Prefab::TemplateId> m_createdTemplateIds;
        };
    } // namespace SerializeContextTools
} // namespace AZ
