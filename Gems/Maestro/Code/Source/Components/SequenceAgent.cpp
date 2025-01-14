/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "SequenceAgent.h"
#include <AzCore/Math/Color.h>
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Component/Entity.h>

namespace Maestro
{
    namespace SequenceAgentHelper
    {
        template <typename T>
        AZ_INLINE void DoSafeSet(AZ::BehaviorEBus::VirtualProperty* prop, AZ::EntityId entityId, T& data)
        {
            if (!prop->m_setter)
            {
                return;
            }
            if (prop->m_setter->m_event)
            {
                prop->m_setter->m_event->Invoke(entityId, data);
            }
            else if (prop->m_setter->m_broadcast)
            {
                prop->m_setter->m_broadcast->Invoke(data);
            }
        }

        template <typename T>
        AZ_INLINE void DoSafeGet(AZ::BehaviorEBus::VirtualProperty* prop, AZ::EntityId entityId, T& data)
        {
            if (!prop->m_getter)
            {
                return;
            }
            if (prop->m_getter->m_event)
            {
                prop->m_getter->m_event->InvokeResult(data, entityId);
            }
            else if (prop->m_getter->m_broadcast)
            {
                prop->m_getter->m_broadcast->InvokeResult(data);
            }
        }
    }

    /////////////////////////////////////////////////////////////////////////////////////////////
    void SequenceAgent::CacheAllVirtualPropertiesFromBehaviorContext()
    {
        AZ::BehaviorContext* behaviorContext = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(behaviorContext, &AZ::ComponentApplicationBus::Events::GetBehaviorContext);
        
        AZ::Entity::ComponentArrayType entityComponents;
        GetEntityComponents(entityComponents);

        m_addressToBehaviorVirtualPropertiesMap.clear();

        for (AZ::Component* component : entityComponents)
        {
            auto findClassIter = behaviorContext->m_typeToClassMap.find(GetComponentTypeUuid(*component));
            if (findClassIter != behaviorContext->m_typeToClassMap.end())
            {            
                AZ::BehaviorClass* behaviorClass = findClassIter->second;
                // go through all ebuses for this class and find all virtual properties
                for (auto reqBusName = behaviorClass->m_requestBuses.begin(); reqBusName != behaviorClass->m_requestBuses.end(); reqBusName++)
                {
                    auto findBusIter = behaviorContext->m_ebuses.find(*reqBusName);
                    if (findBusIter != behaviorContext->m_ebuses.end())
                    {
                        AZ::BehaviorEBus* behaviorEbus = findBusIter->second;
                        for (auto virtualPropertyIter = behaviorEbus->m_virtualProperties.begin(); virtualPropertyIter != behaviorEbus->m_virtualProperties.end(); virtualPropertyIter++)
                        {
                            SequenceComponentRequests::AnimatablePropertyAddress   address(component->GetId(), virtualPropertyIter->first);
                            m_addressToBehaviorVirtualPropertiesMap[address] = &virtualPropertyIter->second;
                        }

                        // Check for the GetAssetDuration event is present and cache it
                        for (auto eventIter = behaviorEbus->m_events.begin(); eventIter != behaviorEbus->m_events.end(); eventIter++)
                        {
                            if (eventIter->first == "GetAssetDuration")
                            {
                                AZ::BehaviorEBusEventSender* sender = &eventIter->second;

                                // Sanity check, Shouldn't be able to get here.
                                if (nullptr == sender)
                                {
                                    AZ_Error("SequenceAgent", false, "EBus %s, GetAssetDuration sender is null.", behaviorEbus->m_name.c_str());
                                    continue;
                                }
                                    
                                // Sanity check, m_broadcast should always be valid.
                                if (nullptr == sender->m_broadcast)
                                {
                                    AZ_Error("SequenceAgent", false, "EBus %s, GetAssetDuration sender->m_broadcast is null.", behaviorEbus->m_name.c_str());
                                    continue;
                                }

                                // GetAssetDuration shoulder return a float. 
                                if (!sender->m_broadcast->HasResult())
                                {
                                    AZ_Error("SequenceAgent", false, "EBus %s, GetAssetDuration should return result", behaviorEbus->m_name.c_str());
                                    continue;
                                }

                                // GetAssetDuration should take an asset id.
                                if (sender->m_broadcast->GetNumArguments() != 1)
                                {
                                    AZ_Error("SequenceAgent", false, "EBus %s, GetAssetDuration should take one argument, an asset id.", behaviorEbus->m_name.c_str());
                                    continue;
                                }

                                m_addressToGetAssetDurationMap[component->GetId()] = sender;
                            }
                        }
                    }
                }
            }
        }
    }

    /////////////////////////////////////////////////////////////////////////////////////////////
    AZ::Uuid SequenceAgent::GetVirtualPropertyTypeId(const SequenceComponentRequests::AnimatablePropertyAddress& animatableAddress) const
    {
        AZ::Uuid retTypeUuid = AZ::Uuid::CreateNull();

        auto findIter = m_addressToBehaviorVirtualPropertiesMap.find(animatableAddress);
        if (findIter != m_addressToBehaviorVirtualPropertiesMap.end())
        {
            if (findIter->second->m_getter->m_event)
            {
                retTypeUuid = findIter->second->m_getter->m_event->GetResult()->m_typeId;
            }
            else if(findIter->second->m_getter->m_broadcast)
            {
                retTypeUuid = findIter->second->m_getter->m_broadcast->GetResult()->m_typeId;
            }
        }
        return retTypeUuid;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////////
    bool SequenceAgent::SetAnimatedPropertyValue(AZ::EntityId entityId, const SequenceComponentRequests::AnimatablePropertyAddress& animatableAddress, const SequenceComponentRequests::AnimatedValue& value)
    {
        bool changed = false;
        const AZ::Uuid propertyTypeId = GetVirtualPropertyTypeId(animatableAddress);

        auto findIter = m_addressToBehaviorVirtualPropertiesMap.find(animatableAddress);
        if (findIter != m_addressToBehaviorVirtualPropertiesMap.end())
        {
            if (propertyTypeId == AZ::Vector3::TYPEINFO_Uuid())
            {
                AZ::Vector3 vector3Value(.0f, .0f, .0f);
                value.GetValue(vector3Value);               // convert the generic value to a Vector3
                SequenceAgentHelper::DoSafeSet(findIter->second, entityId, vector3Value);
                changed = true;
            }
            else if (propertyTypeId == AZ::Color::TYPEINFO_Uuid())
            {
                AZ::Vector3 vector3Value(.0f, .0f, .0f);
                value.GetValue(vector3Value);               // convert the generic value to a Vector3
                AZ::Color colorValue(AZ::Color::CreateFromVector3(vector3Value));
                SequenceAgentHelper::DoSafeSet(findIter->second, entityId, colorValue);
                changed = true;
            }
            else if (propertyTypeId == AZ::Quaternion::TYPEINFO_Uuid())
            {
                AZ::Quaternion quaternionValue(AZ::Quaternion::CreateIdentity());
                value.GetValue(quaternionValue);
                SequenceAgentHelper::DoSafeSet(findIter->second, entityId, quaternionValue);
                changed = true;
            }
            else if (propertyTypeId == AZ::AzTypeInfo<bool>::Uuid())
            {
                bool boolValue = true;
                value.GetValue(boolValue);                  // convert the generic value to a bool
                SequenceAgentHelper::DoSafeSet(findIter->second, entityId, boolValue);
                changed = true;
            }
            else if (propertyTypeId == AZ::AzTypeInfo<AZ::s32>::Uuid())
            {
                AZ::s32 s32value = 0;
                value.GetValue(s32value);
                findIter->second->m_setter->m_event->Invoke(entityId, s32value);
                changed = true;
            }
            else if (propertyTypeId == AZ::AzTypeInfo<AZ::u32>::Uuid())
            {
                AZ::u32 u32value = 0;
                value.GetValue(u32value);
                findIter->second->m_setter->m_event->Invoke(entityId, u32value);
                changed = true;
            }
            else if (propertyTypeId == AZ::AzTypeInfo<AZ::Data::AssetId>::Uuid())
            {
                AZ::Data::AssetId assetIdValue;
                value.GetValue(assetIdValue);
                findIter->second->m_setter->m_event->Invoke(entityId, assetIdValue);
                changed = true;
            }
            else
            {
                // fall-through default is to cast to float
                float floatValue = .0f;
                value.GetValue(floatValue);                 // convert the generic value to a float
                SequenceAgentHelper::DoSafeSet(findIter->second, entityId, floatValue);
                changed = true;
            }
        }
        AZ_Trace("SequenceAgent::SetAnimatedPropertyValue", "Value changed: %s", changed ? "true" : "false");
        return changed;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////////
    void SequenceAgent::GetAnimatedPropertyValue(SequenceComponentRequests::AnimatedValue& returnValue, AZ::EntityId entityId, const SequenceComponentRequests::AnimatablePropertyAddress& animatableAddress)
    {
        const AZ::Uuid propertyTypeId = GetVirtualPropertyTypeId(animatableAddress);

        auto findIter = m_addressToBehaviorVirtualPropertiesMap.find(animatableAddress);
        if (findIter != m_addressToBehaviorVirtualPropertiesMap.end())
        {
            if (propertyTypeId == AZ::Vector3::TYPEINFO_Uuid())
            {
                AZ::Vector3 vector3Value(AZ::Vector3::CreateZero());
                SequenceAgentHelper::DoSafeGet(findIter->second, entityId, vector3Value);
                returnValue.SetValue(vector3Value);
            }
            else if (propertyTypeId == AZ::Color::TYPEINFO_Uuid())
            {
                AZ::Color colorValue(AZ::Color::CreateZero());
                SequenceAgentHelper::DoSafeGet(findIter->second, entityId, colorValue);
                returnValue.SetValue((AZ::Vector3)colorValue);
            }
            else if (propertyTypeId == AZ::Quaternion::TYPEINFO_Uuid())
            {
                AZ::Quaternion quaternionValue(AZ::Quaternion::CreateIdentity());
                SequenceAgentHelper::DoSafeGet(findIter->second, entityId, quaternionValue);
                returnValue.SetValue(quaternionValue);
            }
            else if (propertyTypeId == AZ::AzTypeInfo<bool>::Uuid())
            {
                bool boolValue = false;
                SequenceAgentHelper::DoSafeGet(findIter->second, entityId, boolValue);
                returnValue.SetValue(boolValue);
            }
            else if (propertyTypeId == AZ::AzTypeInfo<AZ::s32>::Uuid())
            {
                AZ::s32 s32Value;
                findIter->second->m_getter->m_event->InvokeResult(s32Value, entityId);
                returnValue.SetValue(s32Value);
            }
            else if (propertyTypeId == AZ::AzTypeInfo<AZ::u32>::Uuid())
            {
                AZ::u32 u32Value;
                findIter->second->m_getter->m_event->InvokeResult(u32Value, entityId);
                returnValue.SetValue(u32Value);
            }
            else if (propertyTypeId == AZ::AzTypeInfo<AZ::Data::AssetId>::Uuid())
            {
                AZ::Data::AssetId assetIdValue;
                findIter->second->m_getter->m_event->InvokeResult(assetIdValue, entityId);
                returnValue.SetValue(assetIdValue);
            }
            else
            {
                // fall-through default is to cast to float
                float floatValue = .0f;
                SequenceAgentHelper::DoSafeGet(findIter->second, entityId, floatValue);
                returnValue.SetValue(floatValue);
            }
        }
    }

    void SequenceAgent::GetAssetDuration(SequenceComponentRequests::AnimatedValue& returnValue, AZ::ComponentId componentId, const AZ::Data::AssetId& assetId)
    {
        auto findIter = m_addressToGetAssetDurationMap.find(componentId);
        if (findIter != m_addressToGetAssetDurationMap.end())
        {
            if (nullptr != findIter->second)
            {
                AZ::BehaviorEBusEventSender* sender = findIter->second;
                if (nullptr != sender)
                {
                    AZ::BehaviorMethod* broadcast = sender->m_broadcast;
                    if (nullptr != broadcast)
                    {
                        float floatValue = 0.0f;
                        broadcast->InvokeResult(floatValue, assetId);
                        returnValue.SetValue(floatValue);
                    }
                }
            }
        }
    }

} // namespace Maestro
