/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "AuxGeomDrawProcessorShared.h"

namespace AZ
{
    namespace Render
    {
        RHI::CullMode ConvertToRHICullMode(AuxGeomFaceCullMode faceCull)
        {
            switch(faceCull)
            {
                case FaceCull_None: return RHI::CullMode::None;
                case FaceCull_Front: return RHI::CullMode::Front;
                case FaceCull_Back: return RHI::CullMode::Back;
            }
            return RHI::CullMode::None;
        }

        RHI::DepthWriteMask ConvertToRHIDepthWriteMask(AuxGeomDepthWriteType depthWrite)
        {
            switch (depthWrite)
            {
            case DepthWrite_On:  return RHI::DepthWriteMask::All;
            case DepthWrite_Off: return RHI::DepthWriteMask::Zero;
            }
            AZ_Assert(false, "Invalid AuxGeom DepthWriteType value passed");
            return RHI::DepthWriteMask::All;
        }

    }
}
