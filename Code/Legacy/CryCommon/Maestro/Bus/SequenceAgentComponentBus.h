/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Component/ComponentBus.h>
#include <AzCore/EBus/EBus.h>
#include <AzCore/std/string/string.h>
#include <Maestro/Bus/SequenceComponentBus.h>

namespace Maestro
{
    /*!
    * SequenceAgentComponentRequests EBus Interface
    * Messages serviced by SequenceAgentComponents.
    *
    * The EBus is Id'ed on a pair of SequenceEntityId, SequenceAgentEntityId 
    */

    //
    // SequenceComponents broadcast to SequenceAgentComponents via a pair of Ids:
    //     sequenceEntityId, sequenceAgentEntityId
    using SequenceAgentEventBusId = AZStd::pair<AZ::EntityId, AZ::EntityId>;        // SequenceComponenet Entity Id, SequenceAgent EntityId

    class SequenceAgentComponentBus
        : public AZ::EBusTraits
    {
    public:
        virtual ~SequenceAgentComponentBus() = default;

        //////////////////////////////////////////////////////////////////////////
        // EBusTraits overrides
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        typedef SequenceAgentEventBusId BusIdType;
        //////////////////////////////////////////////////////////////////////////
    };

    class SequenceAgentComponentRequests
        : public SequenceAgentComponentBus
    {
    public:
        using AnimatablePropertyAddress = Maestro::SequenceComponentRequests::AnimatablePropertyAddress;
        using AnimatedValue = Maestro::SequenceComponentRequests::AnimatedValue;

        //////////////////////////////////////////////////////////////////////////
        // EBusTraits overrides - application is a singleton
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;  // Only one component on a entity can implement the events
        //////////////////////////////////////////////////////////////////////////

        //! Called when a SequenceComponent is connected
        virtual void ConnectSequence(const AZ::EntityId& sequenceEntityId) = 0;

        //! Called when a SequenceComponent is disconnected
        virtual void DisconnectSequence() = 0;

        //! Get the value for an animated float property at the given address on the same entity as the agent.
        //! @param returnValue holds the value to get - this must be instance of one of the concrete subclasses of AnimatedValue, corresponding to the type of the property referenced by the animatedAdresss
        //! @param animatedAddress identifies the component and property to be set
        virtual void GetAnimatedPropertyValue(AnimatedValue& returnValue, const AnimatablePropertyAddress& animatableAddress) = 0;

        //! Set the value for an animated property at the given address on the same entity as the agent
        //! @param animatedAddress identifies the component and property to be set
        //! @param value the value to set - this must be instance of one of the concrete subclasses of AnimatedValue, corresponding to the type of the property referenced by the animatedAdresss
        //! @return true if the value was changed.
        virtual bool SetAnimatedPropertyValue(const AnimatablePropertyAddress& animatableAddress, const AnimatedValue& value) = 0;

        //! Returns the Uuid of the type that the 'getter' returns for this animatableAddress
        virtual AZ::Uuid GetAnimatedAddressTypeId(const Maestro::SequenceComponentRequests::AnimatablePropertyAddress& animatableAddress) = 0;

        //! Track View will expect some components (those using AZ::Data::AssetBlends as a virtual property) to supply a GetAssetDuration event
        //! so Track View can query the duration of an asset (like a motion) without having any knowledge of that that asset is.
        virtual void GetAssetDuration(AnimatedValue& returnValue, AZ::ComponentId componentId, const AZ::Data::AssetId& assetId) = 0;
    };

    using SequenceAgentComponentRequestBus = AZ::EBus<SequenceAgentComponentRequests>;

} // namespace Maestro

namespace AZStd
{
    template <>
    struct hash < Maestro::SequenceAgentEventBusId >
    {
        inline size_t operator()(const Maestro::SequenceAgentEventBusId& eventBusId) const
        {
            AZStd::hash<AZ::EntityId> entityIdHasher;
            size_t retVal = entityIdHasher(eventBusId.first);
            AZStd::hash_combine(retVal, eventBusId.second);
            return retVal;
        }
    };
}
