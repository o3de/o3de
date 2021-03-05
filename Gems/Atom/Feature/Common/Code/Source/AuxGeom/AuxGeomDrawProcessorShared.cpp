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
