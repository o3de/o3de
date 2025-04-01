/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include <QWidget>

#include <AzCore/std/containers/vector.h>
#include <AzQtComponents/Components/Widgets/Card.h>

#include <Source/Editor/ObjectEditor.h>

#include <EMotionStudio/Plugins/StandardPlugins/Source/MotionEvents/EventDataEditor.h>
#endif

class QMenu;
class QAction;

namespace EMotionFX
{
    class ObjectEditor;
    class MotionEvent;
}

namespace EMStudio
{
    class MotionEventEditor
        : public QWidget
    {
        Q_OBJECT //AUTOMOC
    public:
        MotionEventEditor(EMotionFX::Motion* motion = nullptr, EMotionFX::MotionEvent* event = nullptr, QWidget* parent = nullptr);

        EMotionFX::MotionEvent* GetMotionEvent() const { return m_motionEvent; }
        void SetMotionEvent(EMotionFX::Motion* motion, EMotionFX::MotionEvent* event);

    private:
        void Init();

        EMotionFX::MotionEvent* m_motionEvent = nullptr;
        EMotionFX::ObjectEditor* m_baseObjectEditor = nullptr;
        EventDataEditor m_eventDataEditor;
    };
} // namespace EMStudio
