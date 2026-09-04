/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Component/ComponentBus.h>
#include <AzCore/std/string/string.h>
#include <IMovieSystem.h>

#include "SequenceComponentBus.h"
enum class AnimValueType;

namespace Maestro
{
    /*!
    * EditorSequenceComponentRequests EBus Interface
    * Messages serviced by DirectorComponents.
    */
    class EditorSequenceComponentRequests
        : public AZ::ComponentBus
    {
    public:
        //////////////////////////////////////////////////////////////////////////
        // EBusTraits overrides - application is a singleton
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;  // Only one component on a entity can implement the events
        //////////////////////////////////////////////////////////////////////////

        //! Adds an entity to be animated
        virtual bool AddEntityToAnimate(AZ::EntityId entityToAnimate) = 0;

        //! Remove EntityToAnimate
        virtual void RemoveEntityToAnimate(AZ::EntityId removedEntityId) = 0;

        //! Marks the entity as dirty in the Editor. Returns true if the entity was marked as dirty, false otherwise.
        virtual bool MarkEntityAsDirty() const = 0;

        //! Fills in a list of all animatable properites for a given component on a given entity.
        virtual void GetAllAnimatablePropertiesForComponent(IAnimNode::AnimParamInfos& addressList, AZ::EntityId id, AZ::ComponentId componentId) = 0;

        //! Fills in a list of all animatable component ids for the given entity.
        virtual void GetAnimatableComponents(AZStd::vector<AZ::ComponentId>& componentIds, AZ::EntityId id) = 0;

        //! Return the AnimValueType type for the given address
        virtual AnimValueType GetValueType(const AZStd::string& animatableAddress) = 0;
    };

    using EditorSequenceComponentRequestBus = AZ::EBus<EditorSequenceComponentRequests>;

// defined in the bus header so we can refer to it in the Editor code
#define EditorSequenceComponentTypeId "{C02DC0E2-D0F3-488B-B9EE-98E28077EC56}"

} // namespace Maestro
