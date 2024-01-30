/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <PhysX/ArticulationTypes.h>
#include <PxPhysicsAPI.h>

namespace PhysX
{
    //! Get a native PhysX enum from a wrapped articulation joint type.
    physx::PxArticulationJointType::Enum GetPxArticulationJointType(ArticulationJointType articulationJointType);

    //! Get a native PhysX enum from a wrapped articulation axis.
    physx::PxArticulationAxis::Enum GetPxArticulationAxis(ArticulationJointAxis articulationJointAxis);

    //! Get a native PhysX enum from a wrapped articulation motion type.
    physx::PxArticulationMotion::Enum GetPxArticulationMotion(ArticulationJointMotionType articulationJointMotionType);

    //! Get a wrapped articulation joint type from a native PhysX enum.
    ArticulationJointType GetArticulationJointType(physx::PxArticulationJointType::Enum articulationJointType);

    //! Get a wrapped articulation joint axis from a native PhysX enum.
    ArticulationJointAxis GetArticulationJointAxis(physx::PxArticulationAxis::Enum articulationAxis);

    //! Get a wrapped articulation joint motion type from a native PhysX enum.
    ArticulationJointMotionType GetArticulationJointMotionType(physx::PxArticulationMotion::Enum articulationMotion);
} // namespace PhysX


