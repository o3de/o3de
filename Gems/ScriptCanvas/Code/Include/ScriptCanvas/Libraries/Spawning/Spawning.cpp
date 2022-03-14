/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <ScriptCanvas/Libraries/Libraries.h>
#include <ScriptCanvas/Libraries/Spawning/Spawning.h>
#include <ScriptCanvas/Libraries/Core/ContainerTypeReflection.h>
#include <AzFramework/Spawnable/SpawnableEntitiesInterface.cpp>

namespace ScriptCanvas
{
    namespace Library
    {
        void Spawning::Reflect(AZ::ReflectContext* reflection)
        {
            using namespace AzFramework;
            using namespace ContainerTypeReflection;

            Nodeables::Spawning::SpawnableAsset::Reflect(reflection);

            if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflection))
            {
                serializeContext->Class<Spawning, LibraryDefinition>()
                    ->Version(1)
                    ;

                CreateTypeAsMapValueHelper<Data::StringType, EntitySpawnTicket>::ReflectClassInfo(serializeContext);
                CreateTypeAsMapValueHelper<Data::NumberType, EntitySpawnTicket>::ReflectClassInfo(serializeContext);

                serializeContext->RegisterGenericType<AZStd::vector<EntitySpawnTicket>>();
                serializeContext->RegisterGenericType<AZStd::unordered_map<Data::StringType, EntitySpawnTicket>>();
                serializeContext->RegisterGenericType<AZStd::unordered_map<Data::NumberType, EntitySpawnTicket>>();

                AZ::EditContext* editContext = serializeContext->GetEditContext();
                if (editContext)
                {
                    editContext->Class<Spawning>("Spawning", "")
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::Icon, "Icons/ScriptCanvas/Libraries/Entity.png");
                }
            }

            if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(reflection))
            {
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
            }

            HashContainerReflector<EntitySpawnTicket>::Reflect(reflection);
        }

        void Spawning::InitNodeRegistry(NodeRegistry& nodeRegistry)
        {
            AddNodeToRegistry<Spawning, Nodes::CreateSpawnTicketNodeableNode>(nodeRegistry);
            AddNodeToRegistry<Spawning, Nodes::SpawnNodeableNode>(nodeRegistry);
            AddNodeToRegistry<Spawning, Nodes::DespawnNodeableNode>(nodeRegistry);
        }

        AZStd::vector<AZ::ComponentDescriptor*> Spawning::GetComponentDescriptors()
        {
            return AZStd::vector<AZ::ComponentDescriptor*>({
                Nodes::CreateSpawnTicketNodeableNode::CreateDescriptor(),
                Nodes::SpawnNodeableNode::CreateDescriptor(),
                Nodes::DespawnNodeableNode::CreateDescriptor(),
                });
        }
    }
}
