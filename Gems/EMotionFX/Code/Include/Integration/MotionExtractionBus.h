/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/ComponentBus.h>
#include <AzCore/Math/Transform.h>

namespace EMotionFX
{
    class ActorInstance;

    namespace Integration
    {
        class MotionExtractionRequests
            : public AZ::ComponentBus
        {
        public:
            virtual void ExtractMotion(const AZ::Vector3& deltaPosition, float deltaTime) = 0;
        };
        using MotionExtractionRequestBus = AZ::EBus<MotionExtractionRequests>;

        // Optional main-thread modifier, called after animation evaluation and before movement
        // is submitted to physics or the entity transform. The handler calculates a replacement
        // desired pose; it must not apply movement itself. No handler preserves existing behavior.
        class RootMotionModifierRequests : public AZ::ComponentBus
        {
        public:
            static constexpr AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;

            // Return true to accept desiredWorldTransform, false to retain the original pose.
            // Translation is a desired endpoint for this simulation tick, not a teleport request.
            // Scale must remain unchanged; currentWorldTransform is the actual entity pose.
            virtual bool ModifyRootMotion(const ActorInstance& actorInstance, float simulationDeltaTime,
                const AZ::Transform& currentWorldTransform, AZ::Transform& desiredWorldTransform) = 0;
        };
        using RootMotionModifierRequestBus = AZ::EBus<RootMotionModifierRequests>;
    }
}
