/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "precompiled.h"

#include "ScriptEvent.h"
#include "ScriptEventRegistration.h"

#include <ScriptEvents/ScriptEventsBus.h>
#include <ScriptEvents/ScriptEventFundamentalTypes.h>
#include <ScriptEvents/ScriptEventsAsset.h>
#include <ScriptEvents/Internal/BehaviorContextBinding/ScriptEventMethod.h>
#include <ScriptEvents/Internal/BehaviorContextBinding/ScriptEventBroadcast.h>
#include "ScriptEventDefinition.h"

namespace ScriptEvents
{
    static AZ::Outcome<bool, AZStd::string> IsAddressableTypeWithError(const AZ::Uuid& uuid)
    {
        static AZStd::pair<AZ::Uuid, const char*> unsupportedTypes[] = {
            { AZ::Uuid::CreateNull(), "null" },
            { azrtti_typeid<void>(), "void" },
            { azrtti_typeid<float>(), "float" },
            { azrtti_typeid<double>(), "double" } // Due to precision issues, floating point numbers make poor address types
        };

        for (auto& unsupportedType : unsupportedTypes)
        {
            if (unsupportedType.first == uuid)
            {
                return AZ::Failure(AZStd::string::format("The type %s with id %s is not supported as an address type.", unsupportedType.second, uuid.ToString<AZStd::string>().c_str()));
            }
        }

        return AZ::Success(true);
    }

    namespace Internal
    {
        void Utils::BehaviorParameterFromParameter(AZ::BehaviorContext* behaviorContext, const Parameter& parameter, const char* name, AZ::BehaviorParameter& outParameter)
        {
            AZ::Uuid typeId = parameter.GetType();

            outParameter.m_azRtti = nullptr;
            outParameter.m_traits = AZ::BehaviorParameter::TR_NONE;

            const FundamentalTypes* fundamentalTypes = nullptr;
            ScriptEventBus::BroadcastResult(fundamentalTypes, &ScriptEventRequests::GetFundamentalTypes);

            if (typeId == azrtti_typeid<void>())
            {
                outParameter.m_name = name;
                outParameter.m_typeId = typeId;
            }
            else if (fundamentalTypes->IsFundamentalType(typeId))
            {
                const char* typeName = fundamentalTypes->FindFundamentalTypeName(typeId);
                outParameter.m_name = name ? name : (typeName ? typeName : "UnknownType");
                outParameter.m_typeId = typeId;
            }
            else if (const auto& behaviorClass = behaviorContext->m_typeToClassMap.at(typeId))
            {
                outParameter.m_azRtti = behaviorClass->m_azRtti;
                outParameter.m_name = name ? name : behaviorClass->m_name.c_str();
                outParameter.m_typeId = typeId;
            }
            else
            {
                outParameter.m_name = "ERROR";
                outParameter.m_typeId = AZ::Uuid::CreateNull();
                AZStd::string uuid;
                typeId.ToString(uuid);

                AZ_Error("Script Events", false, "Failed to find type %s for parameter %s", uuid.c_str(), name ? name : "UnknownType");
            }
        }

        void Utils::BehaviorParameterFromType(AZ::Uuid typeId, bool addressable, AZ::BehaviorParameter& outParameter)
        {
            AZ::BehaviorContext* behaviorContext = nullptr;
            AZ::ComponentApplicationBus::BroadcastResult(behaviorContext, &AZ::ComponentApplicationBus::Events::GetBehaviorContext);
            AZ_Assert(behaviorContext, "Script Events require a valid Behavior Context");

            outParameter.m_traits = AZ::BehaviorParameter::TR_NONE;
            outParameter.m_typeId = typeId;
            outParameter.m_azRtti = nullptr;

            if (addressable)
            {
                AZ::Outcome<bool, AZStd::string> isAddressableType = IsAddressableTypeWithError(typeId);
                if (!isAddressableType.IsSuccess())
                {
                    AZ_Error("Script Events", false, "%s", isAddressableType.GetError().c_str());
                    return;
                }
            }

            const FundamentalTypes* fundamentalTypes = nullptr;
            ScriptEventBus::BroadcastResult(fundamentalTypes, &ScriptEventRequests::GetFundamentalTypes);

            if (fundamentalTypes && fundamentalTypes->IsFundamentalType(typeId))
            {
                const char* typeName = fundamentalTypes->FindFundamentalTypeName(typeId);
                outParameter.m_name = typeName;
            }
            else if (behaviorContext->m_typeToClassMap.find(typeId) != behaviorContext->m_typeToClassMap.end())
            {
                if (const auto& behaviorClass = behaviorContext->m_typeToClassMap.at(typeId))
                {
                    outParameter.m_azRtti = behaviorClass->m_azRtti;
                    outParameter.m_name = behaviorClass->m_name.c_str();
                }
            }
            else if (typeId == AZ::Uuid::CreateNull() || typeId == AZ::BehaviorContext::GetVoidTypeId())
            {
                outParameter.m_name = "void";
            }
            else
            {
                AZ_Warning("Script Events", false, "Invalid type specified for BehaviorParameter %s", typeId.ToString<AZStd::string>().c_str());
            }
        }

        AZ::BehaviorEBus* Utils::ConstructAndRegisterScriptEventBehaviorEBus(const ScriptEvents::ScriptEvent& definition)
        {
            AZ::BehaviorContext* behaviorContext = nullptr;
            AZ::ComponentApplicationBus::BroadcastResult(behaviorContext, &AZ::ComponentApplicationBus::Events::GetBehaviorContext);

            if (behaviorContext == nullptr)
            {
                return nullptr;
            }

            AZ::BehaviorEBus* bus = aznew AZ::BehaviorEBus();

            bus->m_attributes.push_back(AZStd::make_pair(AZ::RuntimeEBusAttribute, aznew AZ::Edit::AttributeData<bool>(true)));
            bus->m_name = definition.GetName();

            AZ::Uuid busIdType = azrtti_typeid<void>();
            bool addressRequired = definition.IsAddressRequired();
            if (addressRequired)
            {
                busIdType = definition.GetAddressType();
            }

            BehaviorParameterFromType(busIdType, addressRequired, bus->m_idParam);

            bus->m_createHandler = aznew DefaultBehaviorHandlerCreator(bus, behaviorContext, bus->m_name + "::CreateHandler");
            bus->m_destroyHandler = aznew DefaultBehaviorHandlerDestroyer(bus, behaviorContext, bus->m_name + "::DestroyHandler");

            for (auto& method : definition.GetMethods())
            {
                const AZStd::string& methodName = method.GetName();

                // If the script event has a valid address type, then we create an event method
                if (IsAddressableTypeWithError(busIdType).IsSuccess())
                {
                    bus->m_events[methodName].m_event = aznew ScriptEventMethod(behaviorContext, definition, methodName);
                }

                // For all Script Events provide a Broadcast, using Broadcast will bypass the addressing mechanism.
                bus->m_events[methodName].m_broadcast = aznew ScriptEventBroadcast(behaviorContext, definition, methodName);
            }

            behaviorContext->m_ebuses[bus->m_name] = bus;

            return bus;
        }

        bool Utils::DestroyScriptEventBehaviorEBus(AZStd::string_view ebusName)
        {
            AZ::BehaviorContext* behaviorContext = nullptr;
            AZ::ComponentApplicationBus::BroadcastResult(behaviorContext, &AZ::ComponentApplicationBus::Events::GetBehaviorContext);

            if (behaviorContext == nullptr)
            {
                return false;
            }

            bool erasedBus = false;

            auto behaviorEbusEntry = behaviorContext->m_ebuses.find(ebusName);
            if (behaviorEbusEntry != behaviorContext->m_ebuses.end())
            {
                AZ::BehaviorEBus* bus = behaviorEbusEntry->second;
                delete bus;

                behaviorContext->m_ebuses.erase(behaviorEbusEntry);
                erasedBus = true;
            }

            return erasedBus;
        }

    }
}
