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

#include "PhysX_precompiled.h"

#include <Source/EditorComponentDescriptors.h>

#include <Source/EditorBallJointComponent.h>
#include <Source/EditorColliderComponent.h>
#include <Source/EditorFixedJointComponent.h>
#include <Source/EditorForceRegionComponent.h>
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
