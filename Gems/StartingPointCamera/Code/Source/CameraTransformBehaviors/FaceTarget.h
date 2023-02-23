/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once
#include <CameraFramework/ICameraTransformBehavior.h>
#include <AzCore/RTTI/RTTI.h>
#include <AzCore/Math/Transform.h>
#include <AzCore/Memory/SystemAllocator.h>

namespace AZ
{
    class ReflectContext;
}

namespace Camera
{
    //////////////////////////////////////////////////////////////////////////
    /// This behavior will cause the camera to rotate to face the target
    //////////////////////////////////////////////////////////////////////////
    class FaceTarget
        : public ICameraTransformBehavior
    {
    public:
        ~FaceTarget() override = default;
        AZ_RTTI(FaceTarget, "{1A2CBCD0-1841-493C-8DB7-1BCA0D293019}", ICameraTransformBehavior)
        AZ_CLASS_ALLOCATOR(FaceTarget, AZ::SystemAllocator);
        static void Reflect(AZ::ReflectContext* reflection);

        //////////////////////////////////////////////////////////////////////////
        // ICameraTransformBehavior
        void AdjustCameraTransform(float deltaTime, const AZ::Transform& initialCameraTransform, const AZ::Transform& targetTransform, AZ::Transform& inOutCameraTransform) override;
        void Activate(AZ::EntityId) override {}
        void Deactivate() override {}

    private:
    };
}
