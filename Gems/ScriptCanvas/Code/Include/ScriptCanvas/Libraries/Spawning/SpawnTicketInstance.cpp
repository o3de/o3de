/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <ScriptCanvas/Libraries/Core/ContainerTypeReflection.h>
#include <ScriptCanvas/Libraries/Spawning/SpawnTicketInstance.h>

namespace ScriptCanvas::Nodeables::Spawning
{
    void SpawnTicketInstance::Reflect(AZ::ReflectContext* context)
    {
        using namespace ContainerTypeReflection;

        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<SpawnTicketInstance>();

            CreateTypeAsMapValueHelper<AZStd::string, SpawnTicketInstance>::ReflectClassInfo(serializeContext);
            CreateTypeAsMapValueHelper<float, SpawnTicketInstance>::ReflectClassInfo(serializeContext);

            serializeContext->RegisterGenericType<AZStd::vector<SpawnTicketInstance>>();
            serializeContext->RegisterGenericType<AZStd::unordered_map<Data::StringType, SpawnTicketInstance>>();
            serializeContext->RegisterGenericType<AZStd::unordered_map<Data::NumberType, SpawnTicketInstance>>();

            if (AZ::EditContext* editContext = serializeContext->GetEditContext())
            {
                editContext->Class<SpawnTicketInstance>("SpawnTicketInstance", "A wrapper holding reference to EntitySpawnTicket.");
            }
        }

        if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->Class<SpawnTicketInstance>("SpawnTicketInstance")
                           ->Constructor()
                           ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)
                           ->Attribute(AZ::Script::Attributes::Category, "Spawning")
                           ->Attribute(AZ::Script::Attributes::Module, "Spawning")
                           ->Attribute(AZ::Script::Attributes::EnableAsScriptEventParamType, true);

            AZ::BehaviorClass* behaviorClass =
                AZ::BehaviorContextHelper::GetClass(behaviorContext, azrtti_typeid<BehaviorClassReflection<SpawnTicketInstance>>());
            AZ::BehaviorContext::ClassBuilder<BehaviorClassReflection<SpawnTicketInstance>> classBuilder(behaviorContext, behaviorClass);
            CreateTypeAsMapValueHelper<float, SpawnTicketInstance>::AddMethod(&classBuilder);
            CreateTypeAsMapValueHelper<AZStd::string, SpawnTicketInstance>::AddMethod(&classBuilder);

            behaviorContext
                ->Class<BehaviorClassReflection<SpawnTicketInstance>>(
                    AZStd::string::format("ReflectOnDemandTargets_%s", Data::Traits<SpawnTicketInstance>::GetName().data()).data())
                ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)
                ->Attribute(AZ::Script::Attributes::Ignore, true)
                // required to support Array<SpawnTicketInstance> variable type in Script Canvas
                ->Method(
                    "ReflectVector",
                    [](const AZStd::vector<SpawnTicketInstance>&)
                    {
                    })
                // required to support Map<String, SpawnTicketInstance> variable type in Script Canvas
                ->Method(
                    "Map_String_to_SpawnTicketInstance_Func",
                    [](const AZStd::unordered_map<Data::StringType, SpawnTicketInstance>&)
                    {
                    })
                // required to support Map<Number, SpawnTicketInstance> variable type in Script Canvas
                ->Method(
                    "Map_Number_to_SpawnTicketInstance_Func",
                    [](const AZStd::unordered_map<Data::NumberType, SpawnTicketInstance>&)
                    {
                    });
        }

        HashContainerReflector<SpawnTicketInstance>::Reflect(context);
    }
    
    SpawnTicketInstance::SpawnTicketInstance(const SpawnTicketInstance& rhs)
        : m_ticket(rhs.m_ticket)
    {
    }

    SpawnTicketInstance& SpawnTicketInstance::operator=(const SpawnTicketInstance& rhs)
    {
        m_ticket = rhs.m_ticket;
        return *this;
    }
}
