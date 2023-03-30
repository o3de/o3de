/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <ArticulationUtils.h>

namespace PhysX
{
    physx::PxArticulationJointType::Enum GetPxArticulationJointType(ArticulationJointType articulationJointType)
    {
        switch (articulationJointType)
        {
        case ArticulationJointType::Fix:
            return physx::PxArticulationJointType::eFIX;
        case ArticulationJointType::Hinge:
            return physx::PxArticulationJointType::eREVOLUTE;
        case ArticulationJointType::Prismatic:
            return physx::PxArticulationJointType::ePRISMATIC;
        default:
            AZ_ErrorOnce("Articulation Utils", false, "unsupported joint type");
            return physx::PxArticulationJointType::eFIX;
        }
    }

    physx::PxArticulationAxis::Enum GetPxArticulationAxis(ArticulationJointAxis articulationJointAxis)
    {
        switch (articulationJointAxis)
        {
        case ArticulationJointAxis::Twist:
            return physx::PxArticulationAxis::eTWIST;
        case ArticulationJointAxis::SwingY:
            return physx::PxArticulationAxis::eSWING1;
        case ArticulationJointAxis::SwingZ:
            return physx::PxArticulationAxis::eSWING2;
        case ArticulationJointAxis::X:
            return physx::PxArticulationAxis::eX;
        case ArticulationJointAxis::Y:
            return physx::PxArticulationAxis::eY;
        case ArticulationJointAxis::Z:
            return physx::PxArticulationAxis::eZ;
        default:
            AZ_ErrorOnce("Articulation Utils", false, "unsupported joint axis");
            return physx::PxArticulationAxis::eTWIST;
        }
    }

    physx::PxArticulationMotion::Enum GetPxArticulationMotion(ArticulationJointMotionType articulationJointMotionType)
    {
        switch (articulationJointMotionType)
        {
        case ArticulationJointMotionType::Free:
            return physx::PxArticulationMotion::eFREE;
        case ArticulationJointMotionType::Limited:
            return physx::PxArticulationMotion::eLIMITED;
        case ArticulationJointMotionType::Locked:
            return physx::PxArticulationMotion::eLOCKED;
        default:
            AZ_ErrorOnce("Articulation Utils", false, "unsupported joint motion type");
            return physx::PxArticulationMotion::eLOCKED;
        }
    }

    ArticulationJointType GetArticulationJointType(physx::PxArticulationJointType::Enum articulationJointType)
    {
        switch (articulationJointType)
        {
        case physx::PxArticulationJointType::eFIX:
            return ArticulationJointType::Fix;
        case physx::PxArticulationJointType::eREVOLUTE:
            return ArticulationJointType::Hinge;
        case physx::PxArticulationJointType::ePRISMATIC:
            return ArticulationJointType::Prismatic;
        default:
            AZ_ErrorOnce("Articulation Utils", false, "unsupported articulation joint type");
            return ArticulationJointType::Unsupported;
        }
    }

    ArticulationJointAxis GetArticulationJointAxis(physx::PxArticulationAxis::Enum articulationAxis)
    {
        switch (articulationAxis)
        {
        case physx::PxArticulationAxis::eTWIST:
            return ArticulationJointAxis::Twist;
        case physx::PxArticulationAxis::eSWING1:
            return ArticulationJointAxis::SwingY;
        case physx::PxArticulationAxis::eSWING2:
            return ArticulationJointAxis::SwingZ;
        case physx::PxArticulationAxis::eX:
            return ArticulationJointAxis::X;
        case physx::PxArticulationAxis::eY:
            return ArticulationJointAxis::Y;
        case physx::PxArticulationAxis::eZ:
            return ArticulationJointAxis::Z;
        default:
            AZ_ErrorOnce("Articulation Utils", false, "unsupported articulation axis");
            return ArticulationJointAxis::Twist;
        }
    }

    ArticulationJointMotionType GetArticulationJointMotionType(physx::PxArticulationMotion::Enum articulationMotion)
    {
        switch (articulationMotion)
        {
        case physx::PxArticulationMotion::eFREE:
            return ArticulationJointMotionType::Free;
        case physx::PxArticulationMotion::eLIMITED:
            return ArticulationJointMotionType::Limited;
        case physx::PxArticulationMotion::eLOCKED:
            return ArticulationJointMotionType::Locked;
        default:
            AZ_ErrorOnce("Articulation Utils", false, "unsupported articulation motion");
            return ArticulationJointMotionType::Locked;
        }
    }
} // namespace PhysX


