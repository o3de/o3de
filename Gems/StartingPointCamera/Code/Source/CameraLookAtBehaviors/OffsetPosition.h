/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once
#include <CameraFramework/ICameraLookAtBehavior.h>
#include <AzCore/RTTI/RTTI.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/Math/Transform.h>
#include <AzCore/Memory/SystemAllocator.h>

namespace AZ
{
    class ReflectContext;
}

namespace Camera
{
    //////////////////////////////////////////////////////////////////////////
    /// Offset Position will offset the current LookAt target transform by "Positional Offset"
    //////////////////////////////////////////////////////////////////////////
    class OffsetPosition
        : public ICameraLookAtBehavior
    {
    public:
        ~OffsetPosition() override = default;
        AZ_RTTI(OffsetPosition, "{5B2975A6-839B-4DE0-842B-EDE78D778BC9}", ICameraLookAtBehavior);
        AZ_CLASS_ALLOCATOR(OffsetPosition, AZ::SystemAllocator);
        static void Reflect(AZ::ReflectContext* reflection);

        //////////////////////////////////////////////////////////////////////////
        // ICameraLookAtBehavior
        void AdjustLookAtTarget(float deltaTime, const AZ::Transform& targetTransform, AZ::Transform& outLookAtTargetTransform) override;
        void Activate(AZ::EntityId) override {}
        void Deactivate() override {}

    private:
        AZ::Vector3 m_positionalOffset = AZ::Vector3::CreateZero();
        bool m_isRelativeOffset = false;
    };
} // namespace Camera
