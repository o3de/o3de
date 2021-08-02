/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Component/EntityId.h>
#include <AzCore/Math/Quaternion.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/Memory/Memory.h>
#include <AzCore/RTTI/RTTI.h>
#include <AzCore/std/containers/vector.h>

namespace AZ
{
    class ReflectContext;
}

namespace AzPhysics
{
    //! Base Class of all Physics Joints that will be simulated.
    struct JointConfiguration
    {
        AZ_CLASS_ALLOCATOR_DECL;
        AZ_RTTI(AzPhysics::JointConfiguration, "{DF91D39A-4901-48C4-9159-93FD2ACA5252}");
        static void Reflect(AZ::ReflectContext* context);

        JointConfiguration() = default;
        virtual ~JointConfiguration() = default;

        // Entity/object association.
        void* m_customUserData = nullptr;

        // Basic initial settings.
        AZ::Quaternion m_parentLocalRotation = AZ::Quaternion::CreateIdentity(); ///< Parent joint frame relative to parent body.
        AZ::Vector3 m_parentLocalPosition = AZ::Vector3::CreateZero(); ///< Joint position relative to parent body.
        AZ::Quaternion m_childLocalRotation = AZ::Quaternion::CreateIdentity(); ///< Child joint frame relative to child body.
        AZ::Vector3 m_childLocalPosition = AZ::Vector3::CreateZero(); ///< Joint position relative to child body.
        bool m_startSimulationEnabled = true;
        
        // For debugging/tracking purposes only.
        AZStd::string m_debugName;
    };
}
