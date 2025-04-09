/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Math/Crc.h>

namespace PhysX
{
    namespace NativeTypeIdentifiers
    {
        static const AZ::Crc32 World = AZ_CRC_CE("PhysXWorld");
        static const AZ::Crc32 RigidBody = AZ_CRC_CE("PhysXRigidBody");
        static const AZ::Crc32 RigidBodyStatic = AZ_CRC_CE("PhysXRigidBodyStatic");
        static const AZ::Crc32 ArticulationLink = AZ_CRC_CE("PhysXArticulationLink");
        static const AZ::Crc32 D6Joint = AZ_CRC_CE("PhysXD6Joint");
        static const AZ::Crc32 CharacterController = AZ_CRC_CE("PhysXCharacterController");
        static const AZ::Crc32 CharacterControllerReplica = AZ_CRC_CE("PhysXCharacterControllerReplica");
        static const AZ::Crc32 RagdollNode = AZ_CRC_CE("PhysXRagdollNode");
        static const AZ::Crc32 Ragdoll = AZ_CRC_CE("PhysXRagdoll");
        static const AZ::Crc32 FixedJoint = AZ_CRC_CE("PhysXFixedJoint");
        static const AZ::Crc32 HingeJoint = AZ_CRC_CE("PhysXHingeJoint");
        static const AZ::Crc32 BallJoint = AZ_CRC_CE("PhysXBallJoint");
        static const AZ::Crc32 PrismaticJoint = AZ_CRC_CE("PhysXPrismaticJoint");
    } // namespace NativeTypeIdentifiers
} // namespace PhysX
