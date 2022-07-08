/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <ComponentDescriptors.h>
#include <Source/SystemComponent.h>
#include <Source/RigidBodyComponent.h>
#include <Source/BaseColliderComponent.h>
#include <Source/MeshColliderComponent.h>
#include <Source/BoxColliderComponent.h>
#include <Source/HeightfieldColliderComponent.h>
#include <Source/SphereColliderComponent.h>
#include <Source/CapsuleColliderComponent.h>
#include <Source/ShapeColliderComponent.h>
#include <Source/ForceRegionComponent.h>
#include <Source/StaticRigidBodyComponent.h>
#include <Source/PhysXCharacters/Components/CharacterControllerComponent.h>
#include <Source/PhysXCharacters/Components/CharacterGameplayComponent.h>
#include <Source/PhysXCharacters/Components/RagdollComponent.h>
#include <Source/JointComponent.h>
#include <Source/BallJointComponent.h>
#include <Source/FixedJointComponent.h>
#include <Source/HingeJointComponent.h>

namespace PhysX
{
    AZStd::list<AZ::ComponentDescriptor*> GetDescriptors()
    {
        AZStd::list<AZ::ComponentDescriptor*> descriptors =
        {
            SystemComponent::CreateDescriptor(),
            RigidBodyComponent::CreateDescriptor(),
            BaseColliderComponent::CreateDescriptor(),
            MeshColliderComponent::CreateDescriptor(),
            BoxColliderComponent::CreateDescriptor(),
            HeightfieldColliderComponent::CreateDescriptor(),
            SphereColliderComponent::CreateDescriptor(),
            CapsuleColliderComponent::CreateDescriptor(),
            ShapeColliderComponent::CreateDescriptor(),
            ForceRegionComponent::CreateDescriptor(),
            StaticRigidBodyComponent::CreateDescriptor(),
            CharacterControllerComponent::CreateDescriptor(),
            CharacterGameplayComponent::CreateDescriptor(),
            RagdollComponent::CreateDescriptor(),
            JointComponent::CreateDescriptor(),
            BallJointComponent::CreateDescriptor(),
            FixedJointComponent::CreateDescriptor(),
            HingeJointComponent::CreateDescriptor()
        };

        return descriptors;
    }
} // namespace PhysX
