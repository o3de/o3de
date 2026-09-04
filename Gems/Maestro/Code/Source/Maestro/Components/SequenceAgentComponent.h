/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Component/Component.h>
#include <Maestro/Bus/SequenceAgentComponentBus.h>
#include "SequenceAgent.h"

namespace AzFramework
{
    class TransformComponent;
}

namespace Maestro
{
    class SequenceAgentComponent
        : public AZ::Component
        , public SequenceAgentComponentRequestBus::MultiHandler
        , public SequenceAgent
    {
    public:
        friend class EditorSequenceAgentComponent;

        AZ_COMPONENT(SequenceAgentComponent, "{67DC06D3-1F16-4FAB-B3F8-D8C0A3AF4F61}");

        //////////////////////////////////////////////////////////////////////////
        // AZ::Component interface implementation
        void Init() override;
        void Activate() override;
        void Deactivate() override;
        //////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        // SequenceAgentComponentRequestBus::Handler Interface
        void GetAnimatedPropertyValue(AnimatedValue& returnValue, const Maestro::SequenceComponentRequests::AnimatablePropertyAddress& animatableAddress) override;
        bool SetAnimatedPropertyValue(const Maestro::SequenceComponentRequests::AnimatablePropertyAddress& animatableAddress, const AnimatedValue& value) override;

        AZ::Uuid GetAnimatedAddressTypeId(const AnimatablePropertyAddress& animatableAddress) override;

        void GetAssetDuration(AnimatedValue& returnValue, AZ::ComponentId componentId, const AZ::Data::AssetId& assetId) override;

        void ConnectSequence(const AZ::EntityId& sequenceEntityId) override;
        void DisconnectSequence() override;
        //~SequenceAgentComponentRequestBus::Handler Interface
        //////////////////////////////////////////////////////////////////////////
        
    protected:
        // Required Reflect function.
        static void Reflect(AZ::ReflectContext* context);

        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent)
        {
            dependent.push_back(AZ_CRC_CE("TransformService"));
        }

        // Override from SequenceAgent
        AZ::TypeId GetComponentTypeUuid(const AZ::Component& component) const override
        {
            return component.RTTI_GetType();
        }

        // Get all of the components available on the current entity.
        void GetEntityComponents(AZ::Entity::ComponentArrayType& entityComponents) const override;

    private:
        // connect and disconnect to all SequenceComponents registered with us
        void ConnectAllSequences();
        void DisconnectAllSequences();

        // set of ids of all unique Entities with SequenceComponent instances connected to this Agent
        AZStd::unordered_set<AZ::EntityId>       m_sequenceEntityIds;
    };

} // namespace Maestro
