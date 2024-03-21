/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

// Add the DragRight gesture
#if defined(CARBONATED)

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <CryCommon/ISystem.h>

#include "GestureRecognizerDragRight.h"

////////////////////////////////////////////////////////////////////////////////////////////////////
inline void Gestures::RecognizerDragRight::Config::Reflect(AZ::ReflectContext* context)
{
    if (AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context))
    {
        serialize->Class<Config>()
            ->Version(0)
            ->Field("minSecondsHeld", &Config::minSecondsHeld)
            ->Field("minPixelsMoved", &Config::minPixelsMoved)
            ->Field("pointerIndex", &Config::pointerIndex)
            ->Field("priority", &Config::priority)
        ;

        if (AZ::EditContext* ec = serialize->GetEditContext())
        {
            ec->Class<Config>("Drag Right Config", "Configuration values used to setup a gesture recognizer for drags.")
                ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                ->DataElement(AZ::Edit::UIHandlers::SpinBox, &Config::pointerIndex, "Pointer Index", "The pointer (button or finger) index to track.")
                    ->Attribute(AZ::Edit::Attributes::Min, 0)
                    ->Attribute(AZ::Edit::Attributes::Max, 10)
                ->DataElement(AZ::Edit::UIHandlers::Default, &Config::minSecondsHeld, "Min Seconds Held", "The min time in seconds after the initial press before a drag will be recognized.")
                    ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                ->DataElement(AZ::Edit::UIHandlers::Default, &Config::minPixelsMoved, "Min Pixels Moved", "The min distance in pixels that must be dragged before a drag will be recognized.")
                    ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
            ;
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
inline Gestures::RecognizerDragRight::RecognizerDragRight(const Config& config)
    : RecognizerDrag(config)
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////
inline Gestures::RecognizerDragRight::~RecognizerDragRight()
{
}

#endif
