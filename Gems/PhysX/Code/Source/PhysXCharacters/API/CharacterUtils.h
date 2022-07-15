/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzFramework/Physics/Ragdoll.h>
#include <AzFramework/Physics/Common/PhysicsTypes.h>
#include <PhysXCharacters/API/Ragdoll.h>
#include <AzCore/Outcome/Outcome.h>
#include <PxPhysicsAPI.h>

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
            Ragdoll* CreateRagdoll(const Physics::RagdollConfiguration& configuration, AzPhysics::SceneHandle sceneHandle);

            //! Creates a joint drive with properties based on the input values.
            //! The input values are validated and the damping ratio is used to calculate the damping value used internally.
            //! @param strength The joint strength (also referred to as stiffness).
            //! @param dampingRatio The ratio of the damping value to the critical damping value, indicating whether the
            //! joint drive is under-damped, critically damped or over-damped.
            //! @param forceLimit The upper limit on the force the joint can apply to reach its target.
            //! @return The created joint drive.
            physx::PxD6JointDrive CreateD6JointDrive(float strength, float dampingRatio, float forceLimit);

            //! Contains information about a node in a hierarchy and how deep it is in the hierarchy relative to the root.
            struct DepthData
            {
                int m_depth = -1; //!< Depth of the joint in the hierarchy. The root has depth 0, its children depth 1, and so on.
                size_t m_index = 0; //<! Index of the joint in the hierarchy.
            };

            //! Given information about the parent nodes for each node in a hierarchy, computes how deep each node is in the
            //! hierarchy relative to the root level. Assumes that the input parent index data corresponds to a tree structure, i.e.
            //! does not contain any cycles.
            AZStd::vector<DepthData> ComputeHierarchyDepths(const AZStd::vector<size_t>& parentIndices);
        } // namespace Characters
    } // namespace Utils
} // namespace PhysX
