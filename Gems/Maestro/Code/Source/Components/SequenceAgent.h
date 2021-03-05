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

#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Component/ComponentBus.h>
#include <AzCore/Component/EntityId.h>
#include <Maestro/Bus/SequenceAgentComponentBus.h>
#include <AzCore/Serialization/SerializeContext.h>

namespace Maestro
{
    class SequenceAgent
    {
        friend class AZ::SerializeContext;
        
    protected:
        // This pure virtual is required for the Editor and RunTime to find the componentTypeId - in the Editor
        // it accounts for the GenericComponentWrapper component
        virtual const AZ::Uuid& GetComponentTypeUuid(const AZ::Component& component) const = 0;

        // Get all of the components available on the current entity.
        virtual void GetEntityComponents(AZ::Entity::ComponentArrayType& entityComponents) const = 0;

        // this is called on Activate() - it traverses all components on the given entity and fills in m_addressToBehaviorVirtualPropertiesMap
        // with all virtual properties on EBuses it finds. Calling it clears out any previously mapped virtualProperties
        void CacheAllVirtualPropertiesFromBehaviorContext();

        AZ::Uuid GetVirtualPropertyTypeId(const Maestro::SequenceComponentRequests::AnimatablePropertyAddress& animatedAddress) const;

        void GetAnimatedPropertyValue(Maestro::SequenceComponentRequests::AnimatedValue& returnValue, AZ::EntityId entityId, const Maestro::SequenceComponentRequests::AnimatablePropertyAddress& animatableAddress);
        bool SetAnimatedPropertyValue(AZ::EntityId entityId, const Maestro::SequenceComponentRequests::AnimatablePropertyAddress& animatableAddress, const Maestro::SequenceComponentRequests::AnimatedValue& value);

        void GetAssetDuration(Maestro::SequenceComponentRequests::AnimatedValue& returnValue, AZ::ComponentId componentId, const AZ::Data::AssetId& assetId);

        AZStd::unordered_map<Maestro::SequenceComponentRequests::AnimatablePropertyAddress, AZ::BehaviorEBus::VirtualProperty*> m_addressToBehaviorVirtualPropertiesMap;        
        AZStd::unordered_map<AZ::ComponentId, AZ::BehaviorEBusEventSender*> m_addressToGetAssetDurationMap;
    };
} // namespace Maestro

