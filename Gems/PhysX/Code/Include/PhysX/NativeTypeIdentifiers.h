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
        static const AZ::Crc32 World = AZ_CRC("PhysXWorld", 0x87c3e7ad);
        static const AZ::Crc32 RigidBody = AZ_CRC("PhysXRigidBody", 0xb2dcf053);
        static const AZ::Crc32 RigidBodyStatic = AZ_CRC("PhysXRigidBodyStatic", 0xe8b62e7e);
        static const AZ::Crc32 ArticulationLink = AZ_CRC("PhysXArticulationLink", 0x3181634c);
        static const AZ::Crc32 D6Joint = AZ_CRC("PhysXD6Joint", 0xff42ecdd);
        static const AZ::Crc32 CharacterController = AZ_CRC("PhysXCharacterController", 0x1b8787a6);
        static const AZ::Crc32 CharacterControllerReplica = AZ_CRC("PhysXCharacterControllerReplica", 0xa6f3cfcc);
        static const AZ::Crc32 RagdollNode = AZ_CRC("PhysXRagdollNode", 0xd1d24205);
        static const AZ::Crc32 Ragdoll = AZ_CRC("PhysXRagdoll", 0xe081b8b0);
        static const AZ::Crc32 FixedJoint = AZ_CRC("PhysXFixedJoint", 0x06efb273);
        static const AZ::Crc32 HingeJoint = AZ_CRC("PhysXHingeJoint", 0xfe3178e0);
        static const AZ::Crc32 BallJoint = AZ_CRC("PhysXBallJoint", 0x52bae2e7);
    } // namespace NativeTypeIdentifiers
} // namespace PhysX
