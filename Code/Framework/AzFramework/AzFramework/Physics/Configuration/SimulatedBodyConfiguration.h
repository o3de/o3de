/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
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
    //! Base Class of all Physics Bodies that will be simulated.
    struct SimulatedBodyConfiguration
    {
        AZ_CLASS_ALLOCATOR_DECL;
        AZ_RTTI(AzPhysics::SimulatedBodyConfiguration, "{52844E3D-79C8-4F34-AF63-5C45ADE77F85}");
        static void Reflect(AZ::ReflectContext* context);

        SimulatedBodyConfiguration() = default;
        virtual ~SimulatedBodyConfiguration() = default;

        // Basic initial settings.
        AZ::Vector3 m_position = AZ::Vector3::CreateZero();
        AZ::Quaternion m_orientation = AZ::Quaternion::CreateIdentity();
        bool m_startSimulationEnabled = true;

        // Entity/object association.
        AZ::EntityId m_entityId = AZ::EntityId(AZ::EntityId::InvalidEntityId);
        void* m_customUserData = nullptr;

        // For debugging/tracking purposes only.
        AZStd::string m_debugName;
    };

    //! Alias for a list of non owning weak pointers to SceneConfiguration objects.
    //! Used for the creation of multiple SimulatedBodies at once with Scene::AddSimulatedBodies.
    using SimulatedBodyConfigurationList = AZStd::vector<SimulatedBodyConfiguration*>;
}
