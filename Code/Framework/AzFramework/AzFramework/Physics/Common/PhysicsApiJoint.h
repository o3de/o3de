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
    //! Base class for all Joints in Physics.
    struct ApiJoint
    {
        AZ_CLASS_ALLOCATOR_DECL;
        AZ_RTTI(AzPhysics::ApiJoint, "{1EEC9382-3434-4866-9B18-E93F151A6F59}");
        static void Reflect(AZ::ReflectContext* context);

        virtual ~ApiJoint() = default;

        //! The current Scene the joint is contained.
        SceneHandle m_sceneOwner = AzPhysics::InvalidSceneHandle;

        //! The handle to this joint
        ApiJointHandle m_jointHandle = AzPhysics::InvalidApiJointHandle;

        //! Helper functions for setting user data.
        //! @param userData Can be a pointer to any type as internally will be cast to a void*. Object lifetime not managed by the ApiJoint.
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
            float scale,
            AZ::u32 angularSubdivisions,
            AZ::u32 radialSubdivisions,
            AZStd::vector<AZ::Vector3>& vertexBufferOut,
            AZStd::vector<AZ::u32>& indexBufferOut,
            AZStd::vector<AZ::Vector3>& lineBufferOut,
            AZStd::vector<bool>& lineValidityBufferOut) = 0;

    private:
        void* m_customUserData = nullptr;
    };

    //! Alias for a list of non owning weak pointers to ApiJoint objects.
    using ApiJointList = AZStd::vector<ApiJoint*>;
}
