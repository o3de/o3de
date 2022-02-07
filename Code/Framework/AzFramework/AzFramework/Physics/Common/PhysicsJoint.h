/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Math/Aabb.h>
#include <AzCore/Math/Crc.h>
#include <AzCore/Math/Quaternion.h>
#include <AzCore/Memory/Memory.h>
#include <AzCore/RTTI/RTTI.h>
#include <AzCore/std/containers/variant.h>
#include <AzCore/std/containers/vector.h>
#include <AzFramework/Physics/Common/PhysicsSimulatedBodyEvents.h>
#include <AzFramework/Physics/Common/PhysicsSceneQueries.h>
#include <AzFramework/Physics/Common/PhysicsTypes.h>

namespace AZ
{
    class ReflectContext;
}

namespace AzPhysics
{
    struct JointConfiguration;

    //! Base class for all Joints in Physics.
    struct Joint
    {
        AZ_CLASS_ALLOCATOR_DECL;
        AZ_RTTI(AzPhysics::Joint, "{1EEC9382-3434-4866-9B18-E93F151A6F59}");
        static void Reflect(AZ::ReflectContext* context);

        virtual ~Joint() = default;

        //! The current Scene the joint is contained.
        SceneHandle m_sceneOwner = AzPhysics::InvalidSceneHandle;

        //! The handle to this joint.
        JointHandle m_jointHandle = AzPhysics::InvalidJointHandle;

        //! Helper functions for setting user data.
        //! @param userData Can be a pointer to any type as internally will be cast to a void*. Object lifetime not managed by the Joint.
        template<typename T>
        void SetUserData(T* userData)
        {
            m_customUserData = static_cast<void*>(userData);
        }
        //! Helper functions for getting the set user data.
        //! @return Will return a void* to the user data set.
        void* GetUserData()
        {
            return m_customUserData;
        }

        virtual AZ::Crc32 GetNativeType() const = 0;
        virtual void* GetNativePointer() const = 0;

        virtual AzPhysics::SimulatedBodyHandle GetParentBodyHandle() const = 0;
        virtual AzPhysics::SimulatedBodyHandle GetChildBodyHandle() const = 0;

        virtual void SetParentBody(AzPhysics::SimulatedBodyHandle parentBody) = 0;
        virtual void SetChildBody(AzPhysics::SimulatedBodyHandle childBody) = 0;

        virtual void GenerateJointLimitVisualizationData(
            [[ maybe_unused ]] float scale,
            [[ maybe_unused ]] AZ::u32 angularSubdivisions,
            [[ maybe_unused ]] AZ::u32 radialSubdivisions,
            [[ maybe_unused ]] AZStd::vector<AZ::Vector3>& vertexBufferOut,
            [[ maybe_unused ]] AZStd::vector<AZ::u32>& indexBufferOut,
            [[ maybe_unused ]] AZStd::vector<AZ::Vector3>& lineBufferOut,
            [[ maybe_unused ]] AZStd::vector<bool>& lineValidityBufferOut) { }

    private:
        void* m_customUserData = nullptr;
    };

    //! Alias for a list of non owning weak pointers to Joint objects.
    using JointList = AZStd::vector<Joint*>;

    //! Interface to access Joint utilities and helper functions
    class JointHelpersInterface
    {
    public:
        AZ_RTTI(AzPhysics::JointHelpersInterface, "{A511C64D-C8A5-4E8F-9C69-8DC5EFAD0C4C}");

        JointHelpersInterface() = default;
        virtual ~JointHelpersInterface() = default;
        AZ_DISABLE_COPY_MOVE(JointHelpersInterface);

        //! Returns a list of supported Joint types
        virtual const AZStd::vector<AZ::TypeId> GetSupportedJointTypeIds() const = 0;

        //! Returns a TypeID if the request joint type is supported.
        //! If the Physics backend supports this joint type JointHelpersInterface::GetSupportedJointTypeId will return a AZ::TypeId.
        virtual AZStd::optional<const AZ::TypeId> GetSupportedJointTypeId(JointType typeEnum) const = 0;

        //! Computes parameters such as joint limit local rotations to give the desired initial joint limit orientation.
        //! @param jointLimitTypeId The type ID used to identify the particular kind of joint limit configuration to be created.
        //! @param parentWorldRotation The rotation in world space of the parent world body associated with the joint.
        //! @param childWorldRotation The rotation in world space of the child world body associated with the joint.
        //! @param axis Axis used to define the centre for limiting angular degrees of freedom.
        //! @param exampleLocalRotations A vector (which may be empty) containing example valid rotations in the local space
        //! of the child world body relative to the parent world body, which may optionally be used to help estimate the extents
        //! of the joint limit.
        virtual AZStd::unique_ptr<JointConfiguration> ComputeInitialJointLimitConfiguration(
            const AZ::TypeId& jointLimitTypeId,
            const AZ::Quaternion& parentWorldRotation,
            const AZ::Quaternion& childWorldRotation,
            const AZ::Vector3& axis,
            const AZStd::vector<AZ::Quaternion>& exampleLocalRotations) = 0;

        /// Generates joint limit visualization data in appropriate format to pass to DebugDisplayRequests draw functions.
        /// @param configuration The joint configuration to generate visualization data for.
        /// @param parentRotation The rotation of the joint's parent body (in the same frame as childRotation).
        /// @param childRotation The rotation of the joint's child body (in the same frame as parentRotation).
        /// @param scale Scale factor for the output display data.
        /// @param angularSubdivisions Level of detail in the angular direction (may be clamped in the implementation).
        /// @param radialSubdivisions Level of detail in the radial direction (may be clamped in the implementation).
        /// @param[out] vertexBufferOut Used with indexBufferOut to define triangles to be displayed.
        /// @param[out] indexBufferOut Used with vertexBufferOut to define triangles to be displayed.
        /// @param[out] lineBufferOut Used to define lines to be displayed.
        /// @param[out] lineValidityBufferOut Whether each line in the line buffer is part of a valid or violated limit.
        virtual void GenerateJointLimitVisualizationData(
            const JointConfiguration& configuration,
            const AZ::Quaternion& parentRotation,
            const AZ::Quaternion& childRotation,
            float scale,
            AZ::u32 angularSubdivisions,
            AZ::u32 radialSubdivisions,
            AZStd::vector<AZ::Vector3>& vertexBufferOut,
            AZStd::vector<AZ::u32>& indexBufferOut,
            AZStd::vector<AZ::Vector3>& lineBufferOut,
            AZStd::vector<bool>& lineValidityBufferOut) = 0;
    };
}
