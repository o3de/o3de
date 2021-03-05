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
            , m_stashedRecordModeWhileTimeDragging(false) {};
        virtual ~CTrackViewTimelineWidget() = default;

    protected:
        void mousePressEvent(QMouseEvent* event) override;
        void mouseReleaseEvent(QMouseEvent* event) override;

    private:
        bool m_stashedRecordModeWhileTimeDragging;
    };
} // namespace TrackView
