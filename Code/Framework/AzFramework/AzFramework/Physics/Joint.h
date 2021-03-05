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

#include <AzCore/Math/Transform.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzFramework/Physics/WorldBody.h>

namespace Physics
{
    class JointLimitConfiguration
    {
    public:
        AZ_CLASS_ALLOCATOR(JointLimitConfiguration, AZ::SystemAllocator, 0);
        AZ_RTTI(JointLimitConfiguration, "{C9B70C4D-22D7-45AB-9B0A-30A4ED5E42DB}");
        static void Reflect(AZ::ReflectContext* context);

        JointLimitConfiguration() = default;
        JointLimitConfiguration(const JointLimitConfiguration&) = default;
        virtual ~JointLimitConfiguration() = default;

        virtual const char* GetTypeName();

        AZ::Quaternion m_parentLocalRotation = AZ::Quaternion::CreateIdentity(); ///< Parent joint frame relative to parent body.
        AZ::Vector3 m_parentLocalPosition = AZ::Vector3::CreateZero(); ///< Joint position relative to parent body.
        AZ::Quaternion m_childLocalRotation = AZ::Quaternion::CreateIdentity(); ///< Child joint frame relative to child body.
        AZ::Vector3 m_childLocalPosition = AZ::Vector3::CreateZero(); ///< Joint position relative to child body.
    };

    class Joint
    {
    public:
        AZ_CLASS_ALLOCATOR(Joint, AZ::SystemAllocator, 0);
        AZ_RTTI(Joint, "{405F517C-E986-4ACB-9606-D5D080DDE987}");

        virtual Physics::WorldBody* GetParentBody() const = 0;
        virtual Physics::WorldBody* GetChildBody() const = 0;
        virtual void SetParentBody(Physics::WorldBody* parentBody) = 0;
        virtual void SetChildBody(Physics::WorldBody* childBody) = 0;
        virtual const AZStd::string& GetName() const = 0;
        virtual void SetName(const AZStd::string& name) = 0;
        virtual const AZ::Crc32 GetNativeType() const = 0;
        virtual void* GetNativePointer() = 0;
        /// Generates joint limit visualization data in appropriate format to pass to DebugDisplayRequests draw functions.
        /// @param scale Scale factor for the output display data.
        /// @param angularSubdivisions Level of detail in the angular direction (may be clamped in the implementation).
        /// @param radialSubdivisions Level of detail in the radial direction (may be clamped in the implementation).
        /// @param[out] vertexBufferOut Used with indexBufferOut to define triangles to be displayed.
        /// @param[out] indexBufferOut Used with vertexBufferOut to define triangles to be displayed.
        /// @param[out] lineBufferOut Used to define lines to be displayed.
        /// @param[out] lineValidityBufferOut Whether each line in the line buffer is part of a valid or violated limit.
        virtual void GenerateJointLimitVisualizationData(
            float scale,
            AZ::u32 angularSubdivisions,
            AZ::u32 radialSubdivisions,
            AZStd::vector<AZ::Vector3>& vertexBufferOut,
            AZStd::vector<AZ::u32>& indexBufferOut,
            AZStd::vector<AZ::Vector3>& lineBufferOut,
            AZStd::vector<bool>& lineValidityBufferOut) = 0;
    };
} // namespace Physics
