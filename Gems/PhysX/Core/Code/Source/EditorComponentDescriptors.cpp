/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Source/EditorComponentDescriptors.h>

#include <Source/EditorArticulationLinkComponent.h>
#include <Source/EditorBallJointComponent.h>
#include <Source/EditorColliderComponent.h>
#include <Source/EditorMeshColliderComponent.h>
#include <Source/EditorFixedJointComponent.h>
#include <Source/EditorForceRegionComponent.h>
#include <Source/EditorHeightfieldColliderComponent.h>
#include <Source/EditorHingeJointComponent.h>
#include <Source/EditorJointComponent.h>
#include <Source/EditorPrismaticJointComponent.h>
#include <Source/EditorRigidBodyComponent.h>
#include <Source/EditorStaticRigidBodyComponent.h>
#include <Source/EditorShapeColliderComponent.h>
#include <Source/PhysXCharacters/Components/EditorCharacterControllerComponent.h>
#include <Source/PhysXCharacters/Components/EditorCharacterGameplayComponent.h>
#include <Source/Pipeline/MeshBehavior.h>
#include <Source/Pipeline/MeshExporter.h>
#include <Editor/Source/Components/EditorSystemComponent.h>


namespace PhysX
{
    AZStd::list<AZ::ComponentDescriptor*> GetEditorDescriptors()
    {
        AZStd::list<AZ::ComponentDescriptor*> descriptors =
        {
            EditorBallJointComponent::CreateDescriptor(),
            EditorCharacterControllerComponent::CreateDescriptor(),
            EditorCharacterGameplayComponent::CreateDescriptor(),
            EditorColliderComponent::CreateDescriptor(),
            EditorMeshColliderComponent::CreateDescriptor(),
            EditorFixedJointComponent::CreateDescriptor(),
            EditorForceRegionComponent::CreateDescriptor(),
            EditorHeightfieldColliderComponent::CreateDescriptor(),
            EditorHingeJointComponent::CreateDescriptor(),
            EditorPrismaticJointComponent::CreateDescriptor(),
            EditorJointComponent::CreateDescriptor(),
            EditorRigidBodyComponent::CreateDescriptor(),
            EditorStaticRigidBodyComponent::CreateDescriptor(),
            EditorShapeColliderComponent::CreateDescriptor(),
            EditorSystemComponent::CreateDescriptor(),
            EditorArticulationLinkComponent::CreateDescriptor(),
            Pipeline::MeshBehavior::CreateDescriptor(),
            Pipeline::MeshExporter::CreateDescriptor()
        };

        return descriptors;
    }

} // namespace PhysX
