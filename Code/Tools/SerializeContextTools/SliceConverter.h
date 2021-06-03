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
            using TemplateEntityIdPair = AZStd::pair<AzToolsFramework::Prefab::TemplateId, AZ::EntityId>;

            bool ConnectToAssetProcessor();
            void DisconnectFromAssetProcessor();

            bool ConvertSliceFile(AZ::SerializeContext* serializeContext, const AZStd::string& slicePath, bool isDryRun);
            bool ConvertSliceToPrefab(
                AZ::SerializeContext* serializeContext,  AZ::IO::PathView outputPath, bool isDryRun, AZ::Entity* rootEntity);
            void FixPrefabEntities(AZ::Entity& containerEntity, SliceComponent::EntityList& sliceEntities);
            bool ConvertNestedSlices(
                SliceComponent* sliceComponent, AzToolsFramework::Prefab::Instance* sourceInstance,
                AZ::SerializeContext* serializeContext, bool isDryRun);
            bool ConvertSliceInstance(
                AZ::SliceComponent::SliceInstance& instance, AZ::Data::Asset<AZ::SliceAsset>& sliceAsset,
                AzToolsFramework::Prefab::TemplateReference nestedTemplate, AzToolsFramework::Prefab::Instance* topLevelInstance);
            void SetParentEntity(const AZ::Entity& entity, const AZ::EntityId& parentId, bool onlySetIfInvalid);
            void PrintPrefab(AzToolsFramework::Prefab::TemplateId templateId);
            bool SavePrefab(AZ::IO::PathView outputPath, AzToolsFramework::Prefab::TemplateId templateId);

            // Track all of the entity IDs created and the prefab entity aliases that map to them.  This mapping is used
            // with nested slice conversion to remap parent entity IDs to the correct prefab entity IDs.
            AZStd::unordered_map<TemplateEntityIdPair, AzToolsFramework::Prefab::EntityAlias> m_aliasIdMapper;

            // Track all of the created prefab template IDs on a slice conversion so that they can get removed at the end of the
            // conversion for that file.
            AZStd::unordered_set<AzToolsFramework::Prefab::TemplateId> m_createdTemplateIds;
        };
    } // namespace SerializeContextTools
} // namespace AZ
