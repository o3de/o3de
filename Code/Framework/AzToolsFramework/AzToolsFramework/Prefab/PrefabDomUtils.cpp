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

#include <AzCore/Serialization/Json/JsonSerialization.h>
#include <AzToolsFramework/Prefab/PrefabDomUtils.h>
#include <AzToolsFramework/Prefab/Instance/Instance.h>
#include <AzToolsFramework/Prefab/Instance/InstanceEntityIdMapper.h>
#include <AzToolsFramework/Prefab/Instance/InstanceSerializer.h>

namespace AzToolsFramework
{
    namespace Prefab
    {
        namespace PrefabDomUtils
        {
            PrefabDomValueReference FindPrefabDomValue(PrefabDomValue& parentValue, const char* valueName)
            {
                PrefabDomValue::MemberIterator valueIterator = parentValue.FindMember(valueName);
                if (valueIterator == parentValue.MemberEnd())
                {
                    return AZStd::nullopt;
                }

                return valueIterator->value;
            }

            PrefabDomValueConstReference FindPrefabDomValue(const PrefabDomValue& parentValue, const char* valueName)
            {
                PrefabDomValue::ConstMemberIterator valueIterator = parentValue.FindMember(valueName);
                if (valueIterator == parentValue.MemberEnd())
                {
                    return AZStd::nullopt;
                }

                return valueIterator->value;
            }

            bool StoreInstanceInPrefabDom(const Instance& instance, PrefabDom& prefabDom)
            {
                InstanceEntityIdMapper entityIdMapper;
                entityIdMapper.SetStoringInstance(instance);

                // Need to store the id mapper as both its type and its base type
                // Meta data is found by type id and we need access to both types at different levels (Instance, EntityId)
                AZ::JsonSerializerSettings settings;
                settings.m_metadata.Add(static_cast<AZ::JsonEntityIdSerializer::JsonEntityIdMapper*>(&entityIdMapper));
                settings.m_metadata.Add(&entityIdMapper);

                AZ::JsonSerializationResult::ResultCode result =
                    AZ::JsonSerialization::Store(prefabDom, prefabDom.GetAllocator(), instance, settings);

                if (result.GetProcessing() == AZ::JsonSerializationResult::Processing::Halted)
                {
                    AZ_Error("Prefab", false,
                        "Failed to serialize prefab instance with source path %s. "
                        "Unable to proceed.",
                        instance.GetTemplateSourcePath().c_str());

                    return false;
                }

                return true;
            }

            bool LoadInstanceFromPrefabDom(Instance& instance, const PrefabDom& prefabDom, bool shouldClearContainers)
            {
                InstanceEntityIdMapper entityIdMapper;
                entityIdMapper.SetLoadingInstance(instance);

                AZ::JsonDeserializerSettings settings;
                settings.m_metadata.Add(static_cast<AZ::JsonEntityIdSerializer::JsonEntityIdMapper*>(&entityIdMapper));
                settings.m_metadata.Add(&entityIdMapper);
                settings.m_clearContainers = shouldClearContainers;

                AZ::JsonSerializationResult::ResultCode result =
                    AZ::JsonSerialization::Load(instance, prefabDom, settings);

                if (result.GetProcessing() == AZ::JsonSerializationResult::Processing::Halted)
                {
                    AZ_Error("Prefab", false,
                        "Failed to de-serialize Prefab Instance from Prefab DOM. "
                        "Unable to proceed.");

                    return false;
                }

                entityIdMapper.FixUpUnresolvedEntityReferences();

                return true;
            }


        } // namespace PrefabDomUtils
    } // namespace Prefab
} // namespace AzToolsFramework
