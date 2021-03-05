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
        AZ_CLASS_ALLOCATOR(OffsetPosition, AZ::SystemAllocator, 0); ///< Use AZ::SystemAllocator, otherwise a CryEngine allocator will be used. This will cause the Asset Processor to crash when this object is deleted, because of the wrong uninitialisation order
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