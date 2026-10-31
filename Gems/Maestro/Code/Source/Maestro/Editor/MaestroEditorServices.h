/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

class CAnimationContext;
class CTrackViewSequenceManager;

namespace Maestro::Editor
{
    inline constexpr const char* TrackViewPaneName = "Track View";

    void ReloadTrackView();

    CAnimationContext* GetAnimation();
    CTrackViewSequenceManager* GetSequenceManager();

    void SetAnimationContext(CAnimationContext* animationContext);
    void SetSequenceManager(CTrackViewSequenceManager* sequenceManager);
}
