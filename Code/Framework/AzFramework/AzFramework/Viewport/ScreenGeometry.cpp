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
#include <AzCore/RTTI/BehaviorContext.h>

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

        if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->Class<ScreenPoint>()
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)
                ->Constructor()
                ->Constructor<int, int>()
                ->Property("x", BehaviorValueProperty(&ScreenPoint::m_x))
                ->Property("y", BehaviorValueProperty(&ScreenPoint::m_y))
                ;

            behaviorContext->Class<ScreenVector>()
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)
                ->Constructor()
                ->Constructor<int, int>()
                ->Property("x", BehaviorValueProperty(&ScreenVector::m_x))
                ->Property("y", BehaviorValueProperty(&ScreenVector::m_y))
                ;

            behaviorContext->Class<ScreenSize>()
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)
                ->Constructor()
                ->Constructor<int, int>()
                ->Property("width", BehaviorValueProperty(&ScreenSize::m_width))
                ->Property("height", BehaviorValueProperty(&ScreenSize::m_height))
                ;
        }
    }
} // namespace AzFramework
