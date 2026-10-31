/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Maestro/Editor/TrackView/TrackViewTimeline.h>

#include "EditorDefs.h"

#include <Maestro/Editor/AnimationContext.h>

namespace TrackView
{
    void CTrackViewTimelineWidget::mousePressEvent(QMouseEvent* event)
    {
        m_stashedRecordModeWhileTimeDragging = Maestro::Editor::GetAnimation()->IsRecordMode();
        Maestro::Editor::GetAnimation()->SetRecording(false);  // disable recording while dragging time

        TimelineWidget::mousePressEvent(event);
    }

    void CTrackViewTimelineWidget::mouseReleaseEvent(QMouseEvent* event)
    {
        TimelineWidget::mouseReleaseEvent(event);
    
        if (m_stashedRecordModeWhileTimeDragging)
        {
            Maestro::Editor::GetAnimation()->SetRecording(true);   // restore recording
            m_stashedRecordModeWhileTimeDragging = false;       // reset stash
        }
    }
} // namespace TrackView
