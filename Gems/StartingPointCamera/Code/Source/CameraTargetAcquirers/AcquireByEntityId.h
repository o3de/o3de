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
        AZ_CLASS_ALLOCATOR(AcquireByEntityId, AZ::SystemAllocator, 0); ///< Use AZ::SystemAllocator, otherwise a CryEngine allocator will be used. This will cause the Asset Processor to crash when this object is deleted, because of the wrong uninitialisation order
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