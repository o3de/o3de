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

namespace AZ
{
    class ReflectContext;

    namespace Debug
    {
        enum ArcBallControllerChannel : uint32_t
        {
            ArcBallControllerChannel_None        = 0x00,
            ArcBallControllerChannel_Center      = 0x01,
            ArcBallControllerChannel_Pan         = 0x02,
            ArcBallControllerChannel_Distance    = 0x04,
            ArcBallControllerChannel_Heading     = 0x08,
            ArcBallControllerChannel_Pitch       = 0x10,
            ArcBallControllerChannel_Orientation = (ArcBallControllerChannel_Heading | ArcBallControllerChannel_Pitch)
        };

        class ArcBallControllerRequests
            : public AZ::ComponentBus
        {
        public:
            using MutexType = AZStd::recursive_mutex;
            static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;

            virtual void SetCenter(AZ::Vector3 center) = 0;
            virtual void SetPan(AZ::Vector3 pan) = 0;
            virtual void SetDistance(float distance) = 0;
            virtual void SetMinDistance(float minDistance) = 0;
            virtual void SetMaxDistance(float maxDistance) = 0;
            virtual void SetHeading(float heading) = 0;
            virtual void SetPitch(float pitch) = 0;
            virtual void SetPanningSensitivity(float panningSensitivity) = 0;
            virtual void SetZoomingSensitivity(float zoomingSensitivity) = 0;

            virtual AZ::Vector3 GetCenter() = 0;
            virtual AZ::Vector3 GetPan() = 0;
            virtual float GetDistance() = 0;
            virtual float GetMinDistance() = 0;
            virtual float GetMaxDistance() = 0;
            virtual float GetHeading() = 0;
            virtual float GetPitch() = 0;
            virtual float GetPanningSensitivity() = 0;
            virtual float GetZoomingSensitivity() = 0;
        };

        using ArcBallControllerRequestBus = AZ::EBus<ArcBallControllerRequests>;
    } // namespace Debug
} // namespace AZ
