/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Component/Entity.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/std/containers/vector.h>

#include <NvBlastExtDamageShaders.h>
#include <NvBlastTypes.h>

namespace AzPhysics
{
    struct SimulatedBody;
}

namespace AZ
{
    class Transform;
}

namespace Nv::Blast
{
    class TkActor;
} // namespace Nv::Blast

namespace Blast
{
    class BlastFamily;

    //! Represents a part of the destructed object by holding a reference to an entity with a rigid body.
    class BlastActor
    {
    public:
        AZ_CLASS_ALLOCATOR(BlastActor, AZ::SystemAllocator, 0);

        virtual ~BlastActor() = default;

        // Applies pending damage to the actor. Actual damage will be simulated by BlastSystemComponent.
        virtual void Damage(const NvBlastDamageProgram& program, NvBlastExtProgramParams* programParams) = 0;

        virtual AZ::Transform GetTransform() const = 0;
        virtual const BlastFamily& GetFamily() const = 0;
        virtual const Nv::Blast::TkActor& GetTkActor() const = 0;
        virtual AzPhysics::SimulatedBody* GetSimulatedBody() = 0;
        virtual const AzPhysics::SimulatedBody* GetSimulatedBody() const = 0;
        virtual const AZ::Entity* GetEntity() const = 0;
        virtual const AZStd::vector<uint32_t>& GetChunkIndices() const = 0;
        virtual bool IsStatic() const = 0;
    };
} // namespace Blast
