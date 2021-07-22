/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <IAudioSystem.h>
#include <AzCore/Component/ComponentBus.h>
#include <AzCore/Math/Transform.h>

namespace LmbrCentral
{
    /*!
     * AudioListenerComponentRequests EBus Interface
     * Messages serviced by AudioListenerComponents.
     */
    class AudioListenerComponentRequests
        : public AZ::ComponentBus
    {
    public:
        //! Sets the Entity for which the Audio Listener should track rotation.
        virtual void SetRotationEntity(const AZ::EntityId entityId) = 0;

        //! Sets the Entity for which the Audio Listener should track position.
        virtual void SetPositionEntity(const AZ::EntityId entityId) = 0;

        //! Essentially the same as calling SetRotationEntity and SetPositionEntity with the same Entity.
        virtual void SetFullTransformEntity(const AZ::EntityId entityId)
        {
            SetRotationEntity(entityId);
            SetPositionEntity(entityId);
        }

        virtual void SetListenerEnabled(bool enabled) = 0;
    };

    using AudioListenerComponentRequestBus = AZ::EBus<AudioListenerComponentRequests>;

} // namespace LmbrCentral
