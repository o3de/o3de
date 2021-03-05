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
        AZ_CLASS_ALLOCATOR(Rotate, AZ::SystemAllocator, 0); ///< Use AZ::SystemAllocator, otherwise a CryEngine allocator will be used. This will cause the Asset Processor to crash when this object is deleted, because of the wrong uninitialisation order
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