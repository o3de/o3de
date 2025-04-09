/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once
#include <AzCore/std/string/string.h>
#include <AzCore/RTTI/RTTI.h>
#include <CameraFramework/ICameraTargetAcquirer.h>
#include <AzCore/Component/Component.h>
#include <AzCore/Memory/SystemAllocator.h>

namespace AZ
{
    class ReflectContext;
}

namespace Camera
{
    //////////////////////////////////////////////////////////////////////////
    /// This will request Camera targets from the CameraTarget buses.  It will
    /// then return that target's transform when requested by the Camera Rig
    //////////////////////////////////////////////////////////////////////////
    class AcquireByEntityId
        : public ICameraTargetAcquirer
    {
    public:
        ~AcquireByEntityId() override = default;
        AZ_RTTI(AcquireByEntityId, "{14D0D355-1F83-4F46-9DE1-D41D23BDFC3C}", ICameraTargetAcquirer)
        AZ_CLASS_ALLOCATOR(AcquireByEntityId, AZ::SystemAllocator);
        static void Reflect(AZ::ReflectContext* reflection);

        //////////////////////////////////////////////////////////////////////////
        // ICameraTargetAcquirer
        bool AcquireTarget(AZ::Transform& outTransformInformation) override;
        void Activate(AZ::EntityId) override {}
        void Deactivate() override {}

    private:
        //////////////////////////////////////////////////////////////////////////
        // Reflected Data
        AZ::EntityId m_target = AZ::EntityId();
        bool m_shouldUseTargetRotation = true;
        bool m_shouldUseTargetPosition = true;
    };
} //namespace Camera
