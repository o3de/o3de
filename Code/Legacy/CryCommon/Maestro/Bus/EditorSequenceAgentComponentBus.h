/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Component/ComponentBus.h>
#include <AzCore/EBus/EBus.h>
#include <AzCore/std/string/string.h>
#include <Maestro/Bus/SequenceComponentBus.h>
#include <Maestro/Bus/SequenceAgentComponentBus.h>
#include <IMovieSystem.h>

#include "SequenceAgentComponentBus.h"

namespace Maestro
{
    /*!
    * EditorSequenceAgentComponentRequests EBus Interface
    * Messages serviced by EditorSequenceAgentComponents.
    *
    * The EBus is Id'ed on a pair of SequenceEntityId, SequenceAgentEntityId 
    */

    class EditorSequenceAgentComponentBus
        : public AZ::EBusTraits
    {
    public:
        virtual ~EditorSequenceAgentComponentBus() = default;

        //////////////////////////////////////////////////////////////////////////
        // EBusTraits overrides
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        using BusIdType = SequenceAgentEventBusId;
        //////////////////////////////////////////////////////////////////////////
    };

    class EditorSequenceAgentComponentRequests
        : public EditorSequenceAgentComponentBus
    {
    public:

        //////////////////////////////////////////////////////////////////////////
        // EBusTraits overrides - application is a singleton
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;  // Only one component on a entity can implement the events
        //////////////////////////////////////////////////////////////////////////

        //! Returns a list of all animatable properties for a given componentId on the entity which holds the SequenceAgent Component
        virtual void GetAllAnimatableProperties(IAnimNode::AnimParamInfos& propertyNames, AZ::ComponentId componentId) = 0;

        //! Append all animatable components on the entity which holds the SequenceAgent Component
        virtual void GetAnimatableComponents(AZStd::vector<AZ::ComponentId>& animatableComponentIds) = 0;
    };

    using EditorSequenceAgentComponentRequestBus = AZ::EBus<EditorSequenceAgentComponentRequests>;

    /**
    * Notifications from the Editor Sequence Agent Component
    */
    class EditorSequenceAgentComponentNotification
        : public AZ::EBusTraits
    {
    public:

        //////////////////////////////////////////////////////////////////////////
        // EBusTraits overrides
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        using BusIdType = AZ::EntityId;
        //////////////////////////////////////////////////////////////////////////

        virtual ~EditorSequenceAgentComponentNotification() {}

        /**
        * Called when a Sequence Agent has been connected to a Sequence.
        */
        virtual void OnSequenceAgentConnected() {}
    };

    using EditorSequenceAgentComponentNotificationBus = AZ::EBus<EditorSequenceAgentComponentNotification>;

} // namespace Maestro
