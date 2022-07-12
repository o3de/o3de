/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "SpawningLibrary.h"

#include <AzFramework/Spawnable/SpawnableEntitiesInterface.h>
#include <AzFramework/Spawnable/Script/SpawnableScriptAssetRef.h>
#include <ScriptCanvas/Libraries/Libraries.h>
#include <ScriptCanvas/Libraries/Core/ContainerTypeReflection.h>

namespace ScriptCanvas::SpawningLibrary
{
    void Reflect(AZ::ReflectContext* reflection)
    {
        using namespace AzFramework;
        using namespace ContainerTypeReflection;
            
        if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflection))
        {
            serializeContext->RegisterGenericType<AZStd::unordered_map<Data::StringType, EntitySpawnTicket>>();
            serializeContext->RegisterGenericType<AZStd::unordered_map<Data::NumberType, EntitySpawnTicket>>();
        }

        if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(reflection))
        {
            // reflecting EntitySpawnTicket to support Map<string, EntitySpawnTicket> and Map<Number, EntitySpawnTicket>
            // as Script Canvas variable types
            behaviorContext->Class<BehaviorClassReflection<EntitySpawnTicket>>("ReflectOnDemandTargets_EntitySpawnTicket")
                ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)
                ->Attribute(AZ::Script::Attributes::Ignore, true)
                // required to support Array<EntitySpawnTicket> variable type in Script Canvas
                ->Method(
                    "ReflectVector",
                    [](const AZStd::vector<EntitySpawnTicket>&)
                    {
                    })
                // required to support Map<String, EntitySpawnTicket> variable type in Script Canvas
                ->Method(
                    "MapStringToSpawnTicketInstance",
                    [](const AZStd::unordered_map<Data::StringType, EntitySpawnTicket>&)
                    {
                    })
                // required to support Map<Number, EntitySpawnTicket> variable type in Script Canvas
                ->Method(
                    "MapNumberToSpawnTicketInstance",
                    [](const AZStd::unordered_map<Data::NumberType, EntitySpawnTicket>&)
                    {
                    });

            // reflecting SpawnableScriptAssetRef to support Map<string, SpawnableScriptAssetRef> and Map<Number, SpawnableScriptAssetRef>
            // as Script Canvas variable types
            behaviorContext
                ->Class<BehaviorClassReflection<Scripts::SpawnableScriptAssetRef>>("ReflectOnDemandTargets_SpawnableScriptAssetRef")
                ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)
                ->Attribute(AZ::Script::Attributes::Ignore, true)
                // required to support Array<SpawnableScriptAssetRef> variable type in Script Canvas
                ->Method(
                    "ReflectVector",
                    [](const AZStd::vector<Scripts::SpawnableScriptAssetRef>&)
                    {
                    })
                // required to support Map<String, SpawnableScriptAssetRef> variable type in Script Canvas
                ->Method(
                    "MapStringToSpawnableScriptAssetRef",
                    [](const AZStd::unordered_map<Data::StringType, Scripts::SpawnableScriptAssetRef>&)
                    {
                    })
                // required to support Map<Number, SpawnableScriptAssetRef> variable type in Script Canvas
                ->Method(
                    "MapNumberToSpawnableScriptAssetRef",
                    [](const AZStd::unordered_map<Data::NumberType, Scripts::SpawnableScriptAssetRef>&)
                    {
                    });
        }

        HashContainerReflector<EntitySpawnTicket>::Reflect(reflection);
        HashContainerReflector<Scripts::SpawnableScriptAssetRef>::Reflect(reflection);
    }
}
