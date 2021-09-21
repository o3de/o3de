/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzToolsFramework/ToolsComponents/EditorComponentBase.h>
#include <AzCore/Component/TickBus.h>

#include <Maestro/Bus/EditorSequenceComponentBus.h>
#include "SequenceComponent.h"
#include "../Cinematics/AnimSequence.h"

namespace Maestro
{
    class EditorSequenceComponent
        : public AzToolsFramework::Components::EditorComponentBase
        , public Maestro::EditorSequenceComponentRequestBus::Handler
        , public Maestro::SequenceComponentRequestBus::Handler
        , public AZ::TickBus::Handler       // for refreshing propertyGrids after SetAnimatedPropertyValue events
    {
    public:
        AZ_EDITOR_COMPONENT(EditorSequenceComponent, EditorSequenceComponentTypeId);    // EditorSequenceComponentTypeId is defined in EditorSequenceComponentBus.h

        using AnimatablePropertyAddress = Maestro::SequenceComponentRequests::AnimatablePropertyAddress;
        using AnimatedValue = Maestro::SequenceComponentRequests::AnimatedValue;

        EditorSequenceComponent();
        ~EditorSequenceComponent();

        //////////////////////////////////////////////////////////////////////////
        // AZ::Component interface implementation
        void Init() override;
        void Activate() override;
        void Deactivate() override;
        //////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        // EditorSequenceComponentRequestBus::Handler Interface
        void GetAllAnimatablePropertiesForComponent(IAnimNode::AnimParamInfos& addressList, AZ::EntityId id, AZ::ComponentId componentId) override;
        void GetAnimatableComponents(AZStd::vector<AZ::ComponentId>& componentIds, AZ::EntityId id) override;

        void AddEntityToAnimate(AZ::EntityId entityToAnimate) override;

        void RemoveEntityToAnimate(AZ::EntityId removedEntityId) override;

        bool MarkEntityAsDirty() const override;

        AnimValueType GetValueType(const AZStd::string& animatableAddress) override;
        // ~EditorSequenceComponentRequestBus::Handler Interface

        //////////////////////////////////////////////////////////////////////////
        // SequenceComponentRequestBus::Handler Interface
        /**
        * Get the current value for a property
        * @param returnValue holds the value to get - this must be instance of one of the concrete subclasses of AnimatedValue, corresponding to the type of the property referenced by the animatedAdresss
        * @param animatedEntityId the entity Id of the entity containing the animatedAddress
        * @param animatedAddress identifies the component and property to be set
        */
        void GetAnimatedPropertyValue(AnimatedValue& returnValue, const AZ::EntityId& animatedEntityId, const AnimatablePropertyAddress& animatableAddress) override;
        /**
        * Set a value for an animated property at the given address on the given entity.
        * @param animatedEntityId the entity Id of the entity containing the animatedAddress
        * @param animatedAddress identifies the component and property to be set
        * @param value the value to set - this must be instance of one of the concrete subclasses of AnimatedValue, corresponding to the type of the property referenced by the animatedAdresss
        * @return true if the value was changed.
        */
        bool SetAnimatedPropertyValue(const AZ::EntityId& animatedEntityId, const AnimatablePropertyAddress& animatableAddress, const AnimatedValue& value) override;

        AZ::Uuid GetAnimatedAddressTypeId(const AZ::EntityId& animatedEntityId, const Maestro::SequenceComponentRequests::AnimatablePropertyAddress& animatableAddress) override;

        void GetAssetDuration(AnimatedValue& returnValue, const AZ::EntityId& animatedEntityId, AZ::ComponentId componentId, const AZ::Data::AssetId& assetId) override;

        //////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        // TickBus - used to refresh property displays when values are animated
        void OnTick(float deltaTime, AZ::ScriptTimePoint time) override;
        //////////////////////////////////////////////////////////////////////////

        // TODO - this should be on a Bus, right?
        IAnimSequence* GetSequence() { return m_sequence.get(); }

    protected:

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
        {
            provided.push_back(AZ_CRC("SequenceService", 0x7cbe5938));
        }
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
        {
            // This guarantees that only one SequenceComponent will ever be on an entity
            incompatible.push_back(AZ_CRC("SequenceService", 0x7cbe5938));
            incompatible.push_back(AZ_CRC_CE("NonUniformScaleService"));
        }

        // Required Reflect function.
        static void Reflect(AZ::ReflectContext* context);

        ////////////////////////////////////////////////////////////////////////
        void BuildGameEntity(AZ::Entity* gameEntity) override;
        ////////////////////////////////////////////////////////////////////////
    private:
        // pointer and id of the CryMovie anim sequence responsible for playback/recording
        AZStd::intrusive_ptr<IAnimSequence> m_sequence;
        uint32                           m_sequenceId;

        static AZ::ScriptTimePoint       s_lastPropertyRefreshTime;
        static const double              s_refreshPeriodMilliseconds;       // property refresh period for SetAnimatedPropertyValue events
        static const uint32              s_invalidSequenceId;
    };
} // namespace Maestro
