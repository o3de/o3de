/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "LyShine_precompiled.h"
#include "UiAnimSerialize.h"

#include <LyShine/UiBase.h>
#include "CompoundSplineTrack.h"
#include "BoolTrack.h"
#include "UiAnimationSystem.h"
#include "AnimSplineTrack.h"
#include "AnimSequence.h"
#include "AzEntityNode.h"
#include "EventNode.h"

////////////////////////////////////////////////////////////////////////////////////////////////////
// NAMESPACE FUNCTIONS
////////////////////////////////////////////////////////////////////////////////////////////////////

namespace UiAnimSerialize
{
    void ReflectUiAnimTypes(AZ::SerializeContext* context)
    {
        TUiAnimSplineTrack<Vec2>::Reflect(context);
        UiCompoundSplineTrack::Reflect(context);
        UiBoolTrack::Reflect(context);
        CUiAnimSequence::Reflect(context);
        CUiAnimNode::Reflect(context);
        CUiAnimAzEntityNode::Reflect(context);
        CUiAnimEventNode::Reflect(context);
        CUiTrackEventTrack::Reflect(context);
    }
}
