/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "ScreenGeometry.h"

#include <AzCore/RTTI/RTTI.h>
#include <AzCore/Serialization/SerializeContext.h>

namespace AzFramework
{
    void ScreenGeometryReflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<ScreenPoint>()->
                Field("X", &ScreenPoint::m_x)->
                Field("Y", &ScreenPoint::m_y);

            serializeContext->Class<ScreenVector>()->
                Field("X", &ScreenVector::m_x)->
                Field("Y", &ScreenVector::m_y);

            serializeContext->Class<ScreenSize>()->
                Field("Width", &ScreenSize::m_width)->
                Field("Height", &ScreenSize::m_height);
        }
    }
} // namespace AzFramework
