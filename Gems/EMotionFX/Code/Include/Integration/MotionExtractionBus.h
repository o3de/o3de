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
        // is submitted to physics or the entity transform. ModifyRootMotion calculates a
        // replacement desired pose without applying movement. QueueRootMotionTranslation may
        // optionally defer physics translation to a separate movement owner.
        // No handler preserves existing behavior.
        class RootMotionModifierRequests : public AZ::ComponentBus
        {
        public:
            static constexpr AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;

            // Return true to accept desiredWorldTransform, false to retain the original pose.
            // Translation is a desired endpoint for this simulation tick, not a teleport request.
            // Scale must remain unchanged; currentWorldTransform is the actual entity pose.
            virtual bool ModifyRootMotion(const ActorInstance& actorInstance, float simulationDeltaTime,
                const AZ::Transform& currentWorldTransform, AZ::Transform& desiredWorldTransform) = 0;

            // Optional ownership of physics translation. Called after pose modification,
            // only for physics characters. Returning true defers this displacement to the
            // handler's physics integration; EMotionFX must not also submit tick velocity.
            // deltaPosition is WORLD metres for the scaled animation tick. The owner must
            // consume it once, through collision movement, and support interruption before
            // physics starts. Default false preserves existing character-controller behavior.
            virtual bool QueueRootMotionTranslation(const AZ::Vector3& /*deltaPosition*/, float /*simulationDeltaTime*/)
            {
                return false;
            }
        };
        using RootMotionModifierRequestBus = AZ::EBus<RootMotionModifierRequests>;
    }
}
