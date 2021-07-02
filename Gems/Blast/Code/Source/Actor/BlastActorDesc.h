/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/std/containers/vector.h>
#include <AzFramework/Physics/Material.h>
#include <AzFramework/Physics/Configuration/RigidBodyConfiguration.h>

namespace Nv::Blast
{
    class TkActor;
} // namespace Nv::Blast

namespace Blast
{
    class BlastFamily;

    //! Data required to create a BlastActor.
    struct BlastActorDesc
    {
        BlastFamily* m_family;
        Nv::Blast::TkActor* m_tkActor;
        Physics::MaterialId m_physicsMaterialId;
        AZ::Vector3 m_parentLinearVelocity = AZ::Vector3::CreateZero();
        AZ::Vector3 m_parentCenterOfMass = AZ::Vector3::CreateZero();
        AzPhysics::RigidBodyConfiguration m_bodyConfiguration; //!< Either rigid dynamic or rigid static
        AZStd::vector<uint32_t> m_chunkIndices; //!< Chunks that are going to simulate this actor.
        AZStd::shared_ptr<AZ::Entity> m_entity; //!< Entity that the actor should use to simulate rigid body
        bool m_isStatic = false; //!< Denotes whether actor should be simulated by a static or dynamic rigid body.
        bool m_isLeafChunk = false; //!< Denotes whether this actor represented by a single leaf chunk.
        float m_scale = 1.0f; //!< Uniform scale applied to the actor.
    };
} // namespace Blast
