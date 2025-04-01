/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/RTTI/ReflectContext.h>
#include <AzCore/Serialization/SerializeContext.h>

#include <Components/ClothConfiguration.h>

namespace NvCloth
{
    void ClothConfiguration::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<ClothConfiguration>()
                ->Version(2)
                ->Field("Mesh Node", &ClothConfiguration::m_meshNode)
                ->Field("Mass", &ClothConfiguration::m_mass)
                ->Field("Use Custom Gravity", &ClothConfiguration::m_useCustomGravity)
                ->Field("Custom Gravity", &ClothConfiguration::m_customGravity)
                ->Field("Gravity Scale", &ClothConfiguration::m_gravityScale)
                ->Field("Stiffness Frequency", &ClothConfiguration::m_stiffnessFrequency)
                ->Field("Motion Constraints Max Distance", &ClothConfiguration::m_motionConstraintsMaxDistance)
                ->Field("Motion Constraints Scale", &ClothConfiguration::m_motionConstraintsScale)
                ->Field("Motion Constraints Bias", &ClothConfiguration::m_motionConstraintsBias)
                ->Field("Motion Constraints Stiffness", &ClothConfiguration::m_motionConstraintsStiffness)
                ->Field("Backstop Radius", &ClothConfiguration::m_backstopRadius)
                ->Field("Backstop Back Offset", &ClothConfiguration::m_backstopBackOffset)
                ->Field("Backstop Front Offset", &ClothConfiguration::m_backstopFrontOffset)
                ->Field("Damping", &ClothConfiguration::m_damping)
                ->Field("Linear Drag", &ClothConfiguration::m_linearDrag)
                ->Field("Angular Drag", &ClothConfiguration::m_angularDrag)
                ->Field("Linear Inertia", &ClothConfiguration::m_linearInteria)
                ->Field("Angular Inertia", &ClothConfiguration::m_angularInteria)
                ->Field("Centrifugal Inertia", &ClothConfiguration::m_centrifugalInertia)
                ->Field("Use Custom Wind Velocity", &ClothConfiguration::m_useCustomWindVelocity)
                ->Field("Wind Velocity", &ClothConfiguration::m_windVelocity)
                ->Field("Air Drag Coefficient", &ClothConfiguration::m_airDragCoefficient)
                ->Field("Air Lift Coefficient", &ClothConfiguration::m_airLiftCoefficient)
                ->Field("Fluid Density", &ClothConfiguration::m_fluidDensity)
                ->Field("Collision Friction", &ClothConfiguration::m_collisionFriction)
                ->Field("Collision Mass Scale", &ClothConfiguration::m_collisionMassScale)
                ->Field("Continuous Collision Detection", &ClothConfiguration::m_continuousCollisionDetection)
                ->Field("Collision Affects Static Particles", &ClothConfiguration::m_collisionAffectsStaticParticles)
                ->Field("Self Collision Distance", &ClothConfiguration::m_selfCollisionDistance)
                ->Field("Self Collision Stiffness", &ClothConfiguration::m_selfCollisionStiffness)
                ->Field("Horizontal Stiffness", &ClothConfiguration::m_horizontalStiffness)
                ->Field("Horizontal Stiffness Multiplier", &ClothConfiguration::m_horizontalStiffnessMultiplier)
                ->Field("Horizontal Compression Limit", &ClothConfiguration::m_horizontalCompressionLimit)
                ->Field("Horizontal Stretch Limit", &ClothConfiguration::m_horizontalStretchLimit)
                ->Field("Vertical Stiffness", &ClothConfiguration::m_verticalStiffness)
                ->Field("Vertical Stiffness Multiplier", &ClothConfiguration::m_verticalStiffnessMultiplier)
                ->Field("Vertical Compression Limit", &ClothConfiguration::m_verticalCompressionLimit)
                ->Field("Vertical Stretch Limit", &ClothConfiguration::m_verticalStretchLimit)
                ->Field("Bending Stiffness", &ClothConfiguration::m_bendingStiffness)
                ->Field("Bending Stiffness Multiplier", &ClothConfiguration::m_bendingStiffnessMultiplier)
                ->Field("Bending Compression Limit", &ClothConfiguration::m_bendingCompressionLimit)
                ->Field("Bending Stretch Limit", &ClothConfiguration::m_bendingStretchLimit)
                ->Field("Shearing Stiffness", &ClothConfiguration::m_shearingStiffness)
                ->Field("Shearing Stiffness Multiplier", &ClothConfiguration::m_shearingStiffnessMultiplier)
                ->Field("Shearing Compression Limit", &ClothConfiguration::m_shearingCompressionLimit)
                ->Field("Shearing Stretch Limit", &ClothConfiguration::m_shearingStretchLimit)
                ->Field("Tether Constraint Stiffness", &ClothConfiguration::m_tetherConstraintStiffness)
                ->Field("Tether Constraint Scale", &ClothConfiguration::m_tetherConstraintScale)
                ->Field("Solver Frequency", &ClothConfiguration::m_solverFrequency)
                ->Field("Acceleration Filter Iterations", &ClothConfiguration::m_accelerationFilterIterations)
                ->Field("Remove Static Triangles", &ClothConfiguration::m_removeStaticTriangles)
                ->Field("Update Normals of Static Particles", &ClothConfiguration::m_updateNormalsOfStaticParticles)
                ;
        }
    }
} // namespace NvCloth
