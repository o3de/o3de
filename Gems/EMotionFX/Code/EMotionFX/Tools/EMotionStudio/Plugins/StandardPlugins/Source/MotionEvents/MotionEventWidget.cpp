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

#include <EMotionFX/Tools/EMotionStudio/Plugins/StandardPlugins/Source/MotionEvents/moc_MotionEventWidget.cpp>
