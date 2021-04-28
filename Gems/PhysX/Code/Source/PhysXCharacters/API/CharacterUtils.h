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
#include <AzFramework/Physics/Ragdoll.h>
#include <AzFramework/Physics/Common/PhysicsTypes.h>
#include <PhysXCharacters/API/Ragdoll.h>
#include <AzCore/Outcome/Outcome.h>

namespace Physics
{
    class CharacterConfiguration;
    class ShapeConfiguration;
} // namespace Physics

namespace PhysX
{
    class CharacterController;
    class PhysXScene;

    namespace Utils
    {
        namespace Characters
        {
            AZ::Outcome<size_t> GetNodeIndex(const Physics::RagdollConfiguration& configuration, const AZStd::string& nodeName);

            //! Creates a character controller based on the supplied configuration in the specified world.
            //! @param scene The scene to add the character controller to.
            //! @param characterConfig Information required to create the controller such as shape, slope behavior etc.
            CharacterController* CreateCharacterController(PhysXScene* scene, const Physics::CharacterConfiguration& characterConfig);

            //! Creates a ragdoll based on the specified setup and initial pose.
            //! @param configuration Information about collider geometry and joint setup required to initialize the ragdoll.
            //! @param sceneHandle A handle to the physics scene in which the ragdoll should be created.
            Ragdoll* CreateRagdoll(Physics::RagdollConfiguration& configuration, AzPhysics::SceneHandle sceneHandle);

            //! Creates a joint drive with properties based on the input values.
            //! The input values are validated and the damping ratio is used to calculate the damping value used internally.
            //! @param strength The joint strength (also referred to as stiffness).
            //! @param dampingRatio The ratio of the damping value to the critical damping value, indicating whether the
            //! joint drive is under-damped, critically damped or over-damped.
            //! @param forceLimit The upper limit on the force the joint can apply to reach its target.
            //! @return The created joint drive.
            physx::PxD6JointDrive CreateD6JointDrive(float strength, float dampingRatio, float forceLimit);
        } // namespace Characters
    } // namespace Utils
} // namespace PhysX
