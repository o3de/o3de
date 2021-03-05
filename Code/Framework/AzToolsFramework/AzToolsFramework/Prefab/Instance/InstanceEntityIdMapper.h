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
namespace AzToolsFramework
{
    namespace Prefab
    {
        class Instance;

        class InstanceEntityIdMapper
            : public AZ::JsonEntityIdSerializer::JsonEntityIdMapper
        {
        public:
            AZ_RTTI(InstanceEntityIdMapper, "{EA76C3A7-1210-4DFB-A1C0-2F8E8B0B888E}", AZ::JsonEntityIdSerializer::JsonEntityIdMapper);

            AZ::JsonSerializationResult::Result MapJsonToId(AZ::EntityId& outputValue, const rapidjson::Value& inputValue, AZ::JsonDeserializerContext& context) override;
            AZ::JsonSerializationResult::Result MapIdToJson(rapidjson::Value& outputValue, const AZ::EntityId& inputValue, AZ::JsonSerializerContext& context) override;

            void SetStoringInstance(const Instance& storingInstance);
            void SetLoadingInstance(Instance& loadingInstance);

        private:
            AZ::EntityId ResolveEntityReferencePath(const AZStd::string& entityIdReferencePath);
            EntityAlias ResolveEntityId(const AZ::EntityId& entityId);

            const Instance* m_storingInstance = nullptr;
            Instance* m_loadingInstance = nullptr;

            inline static constexpr char ReferencePathDelimiter = '/';
        };
    }
}
