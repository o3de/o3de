/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include "Controls/TimelineCtrl.h"

namespace TrackView
{
    /** A customized timeline widget for CTrackViewGraph.
    */
    class CTrackViewTimelineWidget
        : public TimelineWidget
    {
    public:

        CTrackViewTimelineWidget(QWidget* parent = nullptr)
            : TimelineWidget(parent)
            , m_stashedRecordModeWhileTimeDragging(false)
        {
        }

        virtual ~CTrackViewTimelineWidget() = default;

    protected:
        void mousePressEvent(QMouseEvent* event) override;
        void mouseReleaseEvent(QMouseEvent* event) override;

    private:
        bool m_stashedRecordModeWhileTimeDragging;
    };
} // namespace TrackView
