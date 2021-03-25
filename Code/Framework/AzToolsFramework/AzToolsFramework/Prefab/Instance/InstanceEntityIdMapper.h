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

#include <AzCore/Component/EntityIdSerializer.h>
#include <AzCore/IO/Path/Path.h>

namespace AzToolsFramework
{
    namespace Prefab
    {
        class Instance;

        class InstanceEntityIdMapper final
            : public AZ::JsonEntityIdSerializer::JsonEntityIdMapper
        {
        public:
            AZ_RTTI(InstanceEntityIdMapper, "{EA76C3A7-1210-4DFB-A1C0-2F8E8B0B888E}", AZ::JsonEntityIdSerializer::JsonEntityIdMapper);

            AZ::JsonSerializationResult::Result MapJsonToId(AZ::EntityId& outputValue, const rapidjson::Value& inputValue, AZ::JsonDeserializerContext& context) override;
            AZ::JsonSerializationResult::Result MapIdToJson(rapidjson::Value& outputValue, const AZ::EntityId& inputValue, AZ::JsonSerializerContext& context) override;

            void SetStoringInstance(const Instance& storingInstance);
            void SetLoadingInstance(Instance& loadingInstance);

            /**
            * Fixes up any unresolved entity references by assigning them to the id value of the entity it references.
            * This is done by computing the absolute alias paths of all EntityIds and EntityId references
            * then matching the references to the id values they reference.
            * During a load instance and alias information is stored to build out these paths,
            * but will be incomplete until the whole Load call is finished.
            * Calling this after Load and the mapper have finished will give complete information
            * on all entities and references discovered in the load
            */
            void FixUpUnresolvedEntityReferences();

        private:
            using AliasPath = AZ::IO::Path;

            EntityAlias ResolveReferenceId(const AZ::EntityId& entityId);

            void GetAbsoluteInstanceAliasPath(const Instance* instance, AliasPath& aliasPathResult);

            AliasPath m_instanceAbsolutePath;

            const Instance* m_storingInstance = nullptr;
            Instance* m_loadingInstance = nullptr;

            static constexpr const char m_aliasPathSeperator = '/';

            AZStd::unordered_map<Instance*, AZStd::vector<AZStd::pair<EntityAlias, AZ::EntityId>>> m_resolvedEntityAliases;
            AZStd::unordered_map<Instance*, AZStd::vector<AZStd::pair<EntityAlias, AZ::EntityId*>>> m_unresolvedEntityAliases;
        };
    }
}
