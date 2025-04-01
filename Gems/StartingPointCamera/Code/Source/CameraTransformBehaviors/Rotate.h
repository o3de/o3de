/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once
#include <CameraFramework/ICameraTransformBehavior.h>
#include <AzCore/Math/Transform.h>
#include <AzCore/RTTI/ReflectContext.h>
#include "StartingPointCamera/StartingPointCameraConstants.h"
#include <AzCore/Memory/SystemAllocator.h>

namespace Camera
{
    //////////////////////////////////////////////////////////////////////////
    /// This behavior will rotate the calculated camera transform
    //////////////////////////////////////////////////////////////////////////
    class Rotate
        : public ICameraTransformBehavior
    {
    public:
        ~Rotate() override = default;
        AZ_RTTI(Rotate, "{EE06111E-75E8-47F0-B243-5A5308A5F605}", ICameraTransformBehavior)
        AZ_CLASS_ALLOCATOR(Rotate, AZ::SystemAllocator);
        static void Reflect(AZ::ReflectContext* reflection);

        //////////////////////////////////////////////////////////////////////////
        // ICameraTransformBehavior
        void AdjustCameraTransform(float deltaTime, const AZ::Transform& initialCameraTransform, const AZ::Transform& targetTransform, AZ::Transform& inOutCameraTransform) override;
        void Activate(AZ::EntityId) override {}
        void Deactivate() override {}

    private:
        //////////////////////////////////////////////////////////////////////////
        // Reflected Data
        float m_angleInDegrees = 0.f;
        AxisOfRotation m_axisType = X_Axis;
    };
} // namespace Camera
