/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Memory/Memory.h>
#include <AzCore/Serialization/Json/BaseJsonSerializer.h>

namespace AzToolsFramework
{
    namespace Prefab
    {
        class Instance;

        class JsonInstanceSerializer
            : public AZ::BaseJsonSerializer
        {
        public:

            AZ_RTTI(JsonInstanceSerializer, "{C255AE3E-844C-49A1-9519-09E53750D400}", BaseJsonSerializer);
            AZ_CLASS_ALLOCATOR_DECL;

            AZ::JsonSerializationResult::Result Store(rapidjson::Value& outputValue, const void* inputValue, const void* defaultValue,
                const AZ::Uuid& valueTypeId, AZ::JsonSerializerContext& context) override;

            AZ::JsonSerializationResult::Result Load(void* outputValue, const AZ::Uuid& outputValueTypeId, const rapidjson::Value& inputValue,
                AZ::JsonDeserializerContext& context) override;

        private:

            //! Identifies the entities to reload from the patches provided and reloads them from the Dom provided.
            //! @param inputValue The Dom that contains the instance information.
            //! @param context The context that could contain additional metadata needed for the deserialization.
            //! @instance The instance in which the entities need to be reloaded.
            //! @patches The patches to use to identify entities that need reloading.
            //! @result The result code that could be modified during the process of reloading.
            void Reload(
                const rapidjson::Value& inputValue,
                AZ::JsonDeserializerContext& context,
                Instance* instance,
                PrefabDom patches,
                AZ::JsonSerializationResult::ResultCode& result);

            //! Clears all the entities in the instance and loads them from scratch using the DOM provided.
            //! @param inputValue The Dom that contains the instance information.
            //! @param context The context that could contain additional metadata needed for the deserialization.
            //! @instance The instance in which the entities need to be loaded.
            //! @result The result code that could be modified during the process of loading.
            void ClearAndLoadEntities(
                const rapidjson::Value& inputValue,
                AZ::JsonDeserializerContext& context,
                Instance* instance,
                AZ::JsonSerializationResult::ResultCode& result);

            //! Clears all the nested instances in the instance and loads them from scratch using the DOM provided.
            //! @param inputValue The Dom that contains the instance information.
            //! @param context The context that could contain additional metadata needed for the deserialization.
            //! @instance The instance in which the entities need to be loaded.
            //! @result The result code that could be modified during the process of loading.
            void ClearAndLoadInstances(
                const rapidjson::Value& inputValue,
                AZ::JsonDeserializerContext& context,
                Instance* instance,
                AZ::JsonSerializationResult::ResultCode& result);
        };
    }
}
