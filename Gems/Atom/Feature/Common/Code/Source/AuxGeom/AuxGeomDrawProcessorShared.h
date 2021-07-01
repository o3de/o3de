/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Atom/RHI.Reflect/RenderStates.h>
#include "AuxGeomBase.h"

namespace AZ
{
    namespace Render
    {
        RHI::CullMode ConvertToRHICullMode(AuxGeomFaceCullMode faceCull);
        RHI::DepthWriteMask ConvertToRHIDepthWriteMask(AuxGeomDepthWriteType depthWrite);
    }
}
