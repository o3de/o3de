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
#include <AzCore/Memory/SystemAllocator.h>

namespace Camera
{
    //////////////////////////////////////////////////////////////////////////
    /// Use this behavior to offset the camera's position by a fixed amount
    //////////////////////////////////////////////////////////////////////////
    class OffsetCameraPosition
        : public ICameraTransformBehavior
    {
    public:
        ~OffsetCameraPosition() override = default;
        AZ_RTTI(OffsetCameraPosition, "{DB64D5DA-84B7-45B7-B221-B5A07BDA2F69}", ICameraTransformBehavior)
        AZ_CLASS_ALLOCATOR(OffsetCameraPosition, AZ::SystemAllocator);
        static void Reflect(AZ::ReflectContext* reflection);

        //////////////////////////////////////////////////////////////////////////
        // ICameraTransformBehavior
        void AdjustCameraTransform(float deltaTime, const AZ::Transform& initialCameraTransform, const AZ::Transform& targetTransform, AZ::Transform& inOutCameraTransform) override;
        void Activate(AZ::EntityId) override {}
        void Deactivate() override {}

    private:
        //////////////////////////////////////////////////////////////////////////
        // Reflected Data
        AZ::Vector3 m_offset = AZ::Vector3::CreateZero();
        bool m_isRelativeOffset = false;
    };
} // namespace Camera
