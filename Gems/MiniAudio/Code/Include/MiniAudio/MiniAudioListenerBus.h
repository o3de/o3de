/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/ComponentBus.h>
#include <AzCore/Math/Vector3.h>

namespace MiniAudio
{
    class MiniAudioListenerRequests : public AZ::ComponentBus
    {
    public:
        ~MiniAudioListenerRequests() override = default;

        virtual void SetFollowEntity(const AZ::EntityId& followEntity) = 0;
        virtual void SetPosition(const AZ::Vector3& position) = 0;
        virtual AZ::u32 GetChannelCount() const = 0;

        //! Global volume controls
        virtual float GetGlobalVolumePercentage() const = 0;
        virtual void SetGlobalVolumePercentage(float globalVolume) = 0;
        virtual float GetGlobalVolumeDecibels() const = 0;
        virtual void SetGlobalVolumeDecibels(float globalVolumeDecibels) = 0;

        //! Cone controls for directional attenuation
        virtual float GetInnerAngleInRadians() const = 0;
        virtual void SetInnerAngleInRadians(float innerAngleInRadians) = 0;
        virtual float GetInnerAngleInDegrees() const = 0;
        virtual void SetInnerAngleInDegrees(float innerAngleInDegrees) = 0;
        virtual float GetOuterAngleInRadians() const = 0;
        virtual void SetOuterAngleInRadians(float outerAngleInRadians) = 0;
        virtual float GetOuterAngleInDegrees() const = 0;
        virtual void SetOuterAngleInDegrees(float outerAngleInDegrees) = 0;
        virtual float GetOuterVolumePercentage() const = 0;
        virtual void SetOuterVolumePercentage(float outerVolume) = 0;
        virtual float GetOuterVolumeDecibels() const = 0;
        virtual void SetOuterVolumeDecibels(float outerVolume) = 0;
    };

    using MiniAudioListenerRequestBus = AZ::EBus<MiniAudioListenerRequests>;

} // namespace MiniAudio
