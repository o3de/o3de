/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Source/EditorComponentDescriptors.h>

#include <Source/EditorBallJointComponent.h>
#include <Source/EditorColliderComponent.h>
#include <Source/EditorFixedJointComponent.h>
#include <Source/EditorForceRegionComponent.h>
#include <Source/EditorHeightfieldColliderComponent.h>
#include <Source/EditorHingeJointComponent.h>
#include <Source/EditorJointComponent.h>
#include <Source/EditorRigidBodyComponent.h>
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
            EditorFixedJointComponent::CreateDescriptor(),
            EditorForceRegionComponent::CreateDescriptor(),
            EditorHeightfieldColliderComponent::CreateDescriptor(),
            EditorHingeJointComponent::CreateDescriptor(),
            EditorJointComponent::CreateDescriptor(),
            EditorRigidBodyComponent::CreateDescriptor(),
            EditorShapeColliderComponent::CreateDescriptor(),
            EditorSystemComponent::CreateDescriptor(),
            Pipeline::MeshBehavior::CreateDescriptor(),
            Pipeline::MeshExporter::CreateDescriptor()
        };

        return descriptors;
    }

} // namespace PhysX
