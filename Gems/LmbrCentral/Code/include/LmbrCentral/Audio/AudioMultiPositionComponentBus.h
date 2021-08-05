/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/ComponentBus.h>

#include <IAudioInterfacesCommonData.h>

namespace LmbrCentral
{
    /*!
     * AudioMultiPositionComponentRequests EBus Interface
     * Messages serviced by AudioMultiPositionComponents.
     * Multi-Position Component helps to simulate sounds that cover an area in the game,
     * or are instanced in multiple places.
     */
    class AudioMultiPositionComponentRequests
        : public AZ::ComponentBus
    {
    public:
        //! Adds an Entity's position to the Multi-Position audio component.
        virtual void AddEntity(const AZ::EntityId& entityId) = 0;

        //! Removes and Entity's position from the Multi-Position audio component.
        virtual void RemoveEntity(const AZ::EntityId& entityId) = 0;

        //! Sets the behavior type.
        virtual void SetBehaviorType(Audio::MultiPositionBehaviorType type) = 0;
    };

    using AudioMultiPositionComponentRequestBus = AZ::EBus<AudioMultiPositionComponentRequests>;

} // namespace LmbrCentral
