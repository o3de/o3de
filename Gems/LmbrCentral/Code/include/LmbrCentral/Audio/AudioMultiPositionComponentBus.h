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
