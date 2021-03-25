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

#include <AzCore/IO/ByteContainerStream.h>
#include <AzCore/RTTI/ReflectContext.h>
#include <AzCore/RTTI/TypeInfo.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/Utils.h>
#include <AzFramework/Spawnable/Spawnable.h>
#include <AzToolsFramework/Prefab/Spawnable/PrefabCatchmentProcessor.h>
#include <AzToolsFramework/Prefab/Spawnable/SpawnableUtils.h>

namespace AzToolsFramework::Prefab::PrefabConversionUtils
{
    void PrefabCatchmentProcessor::Process(PrefabProcessorContext& context)
    {
        context.ListPrefabs([&context](AZStd::string_view prefabName, PrefabDom& prefab)
            {
                ProcessPrefab(context, prefabName, prefab);
            });
    }

    void PrefabCatchmentProcessor::Reflect(AZ::ReflectContext* context)
    {
        if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context); serializeContext != nullptr)
        {
            serializeContext->Class<PrefabCatchmentProcessor, PrefabProcessor>()->Version(1);
        }
    }

    void PrefabCatchmentProcessor::ProcessPrefab(PrefabProcessorContext& context, AZStd::string_view prefabName, PrefabDom& prefab)
    {

        AZStd::string uniqueName = prefabName;
        uniqueName += '.';
        uniqueName += AzFramework::Spawnable::FileExtension;

        auto serializer = [](AZStd::vector<uint8_t>& output, const ProcessedObjectStore& object) -> bool
        {
            AZ::IO::ByteContainerStream stream(&output);
            return AZ::Utils::SaveObjectToStream(stream, AZ::DataStream::StreamType::ST_BINARY,
                AZStd::any_cast<void>(&object.GetObject()), object.GetObject().type());
        };

        auto spawnable = SpawnableUtils::CreateSpawnable(prefab);
        SpawnableUtils::SortEntitiesByTransformHierarchy(spawnable);
        AZStd::any spawnableAny(AZStd::move(spawnable));

        context.GetProcessedObjects().emplace_back(AZStd::move(uniqueName), AZStd::move(spawnableAny),
            AZStd::move(serializer), AZ::AzTypeInfo<AzFramework::Spawnable>::Uuid());

        context.RemovePrefab(prefabName);
    }
} // namespace AzToolsFramework::Prefab::PrefabConversionUtils
