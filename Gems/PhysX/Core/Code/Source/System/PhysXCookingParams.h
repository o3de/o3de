/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <PxPhysicsAPI.h>

namespace PhysX
{
    namespace PxCooking
    {
        //! Return PxCookingParams better suited for use at run-time, these parameters will improve cooking time.
        //! Reference: https://docs.nvidia.com/gameworks/content/gameworkslibrary/physx/guide/Manual/Geometry.html#triangle-meshes
        physx::PxCookingParams GetRealTimeCookingParams();

        //! Return PxCookingParams better suited for use at edit-time, these parameters will
        //! increase cooking time but improve accuracy/precision.
        physx::PxCookingParams GetEditTimeCookingParams();
    }
}
