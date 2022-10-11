/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Editor/InspectorBus.h>
#include "MotionEventWidget.h"
#include "MotionEventEditor.h"
#include <QVBoxLayout>

namespace EMStudio
{
#define MOTIONEVENT_MINIMAL_RANGE 0.01f

    MotionEventWidget::MotionEventWidget(QWidget* parent)
        : QWidget(parent)
        , m_editor(new MotionEventEditor())
    {
        Init();
        ReInit();
    }

    MotionEventWidget::~MotionEventWidget()
    {
        // Clear the inspector in case this window is currently shown.
        EMStudio::InspectorRequestBus::Broadcast(&EMStudio::InspectorRequestBus::Events::ClearIfShown, this);
    }

    void MotionEventWidget::Init()
    {
        QVBoxLayout* layout = new QVBoxLayout(this);
        layout->setMargin(0);
        layout->addWidget(m_editor, 0, Qt::AlignTop);
    }


    void MotionEventWidget::ReInit(EMotionFX::Motion* motion, EMotionFX::MotionEvent* motionEvent)
    {
        m_editor->SetMotionEvent(motion, motionEvent);
    }
} // namespace EMStudio
