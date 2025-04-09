/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/RTTI/RTTI.h>
#include <AzCore/Component/EntityId.h>
#include <AzCore/Math/Vector3.h>

#include <Utils/AssetHelper.h>

namespace AZ
{
    class ReflectContext;
}

namespace NvCloth
{
    //! Configuration data for Cloth.
    struct ClothConfiguration
    {
        AZ_CLASS_ALLOCATOR(ClothConfiguration, AZ::SystemAllocator);
        AZ_TYPE_INFO(ClothConfiguration, "{96E2AF5E-3C98-4872-8F90-F56302A44F2A}");

        static void Reflect(AZ::ReflectContext* context);

        virtual ~ClothConfiguration() = default;

        bool IsUsingWorldBusGravity() const { return !m_useCustomGravity; }
        bool IsUsingWindBus() const { return !m_useCustomWindVelocity; }

        AZStd::string m_meshNode;

        // Mass and Gravity parameters
        float m_mass = 1.0f;
        bool m_useCustomGravity = false;
        AZ::Vector3 m_customGravity = AZ::Vector3(0.0f, 0.0f, -9.81f);
        float m_gravityScale = 1.0f;

        // Global stiffness frequency
        float m_stiffnessFrequency = 10.0f;

        // Motion constraints Parameters
        float m_motionConstraintsMaxDistance = 10.0f;
        float m_motionConstraintsScale = 1.0f;
        float m_motionConstraintsBias = 0.0f;
        float m_motionConstraintsStiffness = 1.0f;

        // Backstop Parameters
        float m_backstopRadius = 0.1f;
        float m_backstopBackOffset = 0.0f;
        float m_backstopFrontOffset = 0.0f;

        // Damping parameters
        AZ::Vector3 m_damping = AZ::Vector3(0.2f, 0.2f, 0.2f);
        AZ::Vector3 m_linearDrag = AZ::Vector3(0.2f, 0.2f, 0.2f);
        AZ::Vector3 m_angularDrag = AZ::Vector3(0.2f, 0.2f, 0.2f);

        // Inertia parameters
        AZ::Vector3 m_linearInteria = AZ::Vector3::CreateOne();
        AZ::Vector3 m_angularInteria = AZ::Vector3::CreateOne();
        AZ::Vector3 m_centrifugalInertia = AZ::Vector3::CreateOne();

        // Wind parameters
        bool m_useCustomWindVelocity = true;
        AZ::Vector3 m_windVelocity = AZ::Vector3(0.0f, 20.0f, 0.0f);
        float m_airDragCoefficient = 0.0f;
        float m_airLiftCoefficient = 0.0f;
        float m_fluidDensity = 1.0f;

        // Collision parameters
        float m_collisionFriction = 0.0f;
        float m_collisionMassScale = 0.0f;
        bool m_continuousCollisionDetection = false;
        bool m_collisionAffectsStaticParticles = false;

        // Self Collision parameters
        float m_selfCollisionDistance = 0.0f;
        float m_selfCollisionStiffness = 0.2f;

        // Tether Constraints parameters
        float m_tetherConstraintStiffness = 1.0f;
        float m_tetherConstraintScale = 1.0f;

        // Quality parameters
        float m_solverFrequency = 300.0f;
        uint32_t m_accelerationFilterIterations = 30;
        bool m_removeStaticTriangles = true;
        bool m_updateNormalsOfStaticParticles = false;

        // Fabric phases parameters
        float m_horizontalStiffness = 1.0f;
        float m_horizontalStiffnessMultiplier = 0.0f;
        float m_horizontalCompressionLimit = 0.0f;
        float m_horizontalStretchLimit = 0.0f;
        float m_verticalStiffness = 1.0f;
        float m_verticalStiffnessMultiplier = 0.0f;
        float m_verticalCompressionLimit = 0.0f;
        float m_verticalStretchLimit = 0.0f;
        float m_bendingStiffness = 1.0f;
        float m_bendingStiffnessMultiplier = 0.0f;
        float m_bendingCompressionLimit = 0.0f;
        float m_bendingStretchLimit = 0.0f;
        float m_shearingStiffness = 1.0f;
        float m_shearingStiffnessMultiplier = 0.0f;
        float m_shearingCompressionLimit = 0.0f;
        float m_shearingStretchLimit = 0.0f;

    private:
        // Making private functionality related with the Editor Context reflection,
        // it's unnecessary for the clients using ClothConfiguration.
        friend class EditorClothComponent;

        // Used by data elements in EditorClothComponent edit context.
        MeshNodeList m_meshNodeList;
        bool m_hasBackstopData = false;
        AZ::EntityId m_entityId;
    };
} // namespace NvCloth
