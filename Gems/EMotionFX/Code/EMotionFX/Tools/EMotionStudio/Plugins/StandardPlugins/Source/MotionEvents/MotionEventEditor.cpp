/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <QVBoxLayout>
#include <QMenu>

#include <EMotionStudio/Plugins/StandardPlugins/Source/MotionEvents/MotionEventEditor.h>
#include <Source/Editor/ObjectEditor.h>
#include <EMotionFX/Source/MotionEvent.h>

#include <AzToolsFramework/UI/PropertyEditor/PropertyEditorAPI.h>
#include <AzCore/Component/ComponentApplication.h>

namespace EMStudio
{
    MotionEventEditor::MotionEventEditor(EMotionFX::Motion* motion, EMotionFX::MotionEvent* motionEvent, QWidget* parent)
        : QWidget(parent)
        , m_eventDataEditor(motion, motionEvent, nullptr, this)
    {
        Init();
        SetMotionEvent(motion, motionEvent);
        connect(&m_eventDataEditor, &EventDataEditor::eventsChanged, this, &MotionEventEditor::SetMotionEvent);
    }

    void MotionEventEditor::Init()
    {
        AZ::SerializeContext* context = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(context, &AZ::ComponentApplicationBus::Events::GetSerializeContext);

        m_baseObjectEditor = new EMotionFX::ObjectEditor(context);

        QVBoxLayout* layout = new QVBoxLayout(this);
        layout->setMargin(0);
        layout->addWidget(m_baseObjectEditor);
        layout->addWidget(&m_eventDataEditor);
    }

    void MotionEventEditor::SetMotionEvent(EMotionFX::Motion* motion, EMotionFX::MotionEvent* motionEvent)
    {
        setVisible(bool(motionEvent));
        if (!motionEvent)
        {
            return;
        }

        if (m_motionEvent != motionEvent)
        {
            m_baseObjectEditor->ClearInstances(false);
            m_baseObjectEditor->AddInstance(motionEvent, azrtti_typeid(motionEvent));
        }
        else
        {
            m_baseObjectEditor->InvalidateValues();
        }

        m_motionEvent = motionEvent;

        m_eventDataEditor.SetEventDataSet(motion, motionEvent, &m_motionEvent->GetEventDatas());
    }
} // end namespace EMStudio
