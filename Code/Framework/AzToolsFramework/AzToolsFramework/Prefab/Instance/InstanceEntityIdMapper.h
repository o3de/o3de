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
            enum class EntityIdGenerationApproach
            {
                Hashed, //! Creates an entity id that's stable between runs. (Default)
                Random //! Entities get a new randomly generate id.
            };

            AZ_RTTI(InstanceEntityIdMapper, "{EA76C3A7-1210-4DFB-A1C0-2F8E8B0B888E}", AZ::JsonEntityIdSerializer::JsonEntityIdMapper);

            static inline constexpr uint64_t SeedKey = 5915587277; // Random prime number

            AZ::JsonSerializationResult::Result MapJsonToId(AZ::EntityId& outputValue, const rapidjson::Value& inputValue, AZ::JsonDeserializerContext& context) override;
            AZ::JsonSerializationResult::Result MapIdToJson(rapidjson::Value& outputValue, const AZ::EntityId& inputValue, AZ::JsonSerializerContext& context) override;

            void SetEntityIdGenerationApproach(EntityIdGenerationApproach approach);

            void SetStoringInstance(const Instance& storingInstance);
            void SetLoadingInstance(Instance& loadingInstance);

            static AZ::EntityId GenerateEntityIdForAliasPath(const AliasPathView& aliasPath, uint64_t seedKey = SeedKey);

        private:
            EntityAlias ResolveReferenceId(const AZ::EntityId& entityId);

            AliasPath m_instanceAbsolutePath;

            const Instance* m_storingInstance = nullptr;
            Instance* m_loadingInstance = nullptr;

            uint64_t m_randomSeed = SeedKey;

            EntityIdGenerationApproach m_entityIdGenerationApproach { EntityIdGenerationApproach::Hashed };
        };
    }
}
