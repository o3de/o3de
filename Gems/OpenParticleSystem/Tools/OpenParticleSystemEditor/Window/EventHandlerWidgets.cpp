/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <EventHandlerWidgets.h>
#include <Window/EffectorInspector.h>

namespace OpenParticleSystemEditor
{
    EventHandlerWidgets::EventHandlerWidgets(
        AZ::SerializeContext* serializeContext,
        AzToolsFramework::IPropertyEditorNotify* pnotify,
        QWidget* parent)
        : QWidget(parent)
        , m_serializeContext(serializeContext)
        , m_pnotify(pnotify)
        , m_parent(parent)
    {
        auto clickBtnFunc = [this](AZ::u8 index) {
            EffectorInspector* inspector = dynamic_cast<EffectorInspector*>(m_parent);
            if (inspector != nullptr)
            {
                inspector->AddEventHandler(index);
            }
        };
        m_btnAddEventHandler = new QPushButton(this);
        m_btnAddEventHandler->setText(tr("Add Event Handler"));
        connect(m_btnAddEventHandler, &QPushButton::clicked, this, [clickBtnFunc]()
            {
                AZStd::invoke(clickBtnFunc, OpenParticle::ParticleSourceData::DetailConstant::EventIndex::EVENT_HANDLER);
            });

        m_btnAddInheritanceEventHandler = new QPushButton(this);
        m_btnAddInheritanceEventHandler->setText(tr("Add Inheritance Event Handler"));
        connect(m_btnAddInheritanceEventHandler, &QPushButton::clicked, this, [clickBtnFunc, this]()
            {
                AZStd::invoke(clickBtnFunc, OpenParticle::ParticleSourceData::DetailConstant::EventIndex::INHERITANCE_HANDLER);
                m_btnAddInheritanceEventHandler->setEnabled(false);
            });

        m_layout = new QVBoxLayout(this);
        m_layout->setContentsMargins(0, 0, 0, 0);
        m_layout->addWidget(m_btnAddEventHandler);
        m_layout->addWidget(m_btnAddInheritanceEventHandler);
        setLayout(m_layout);
    }

    void EventHandlerWidgets::ClearEventHandlerWidgets()
    {
        for (auto& widget : m_eventHandlerWidgets)
        {
            m_layout->removeWidget(widget);
            widget->ClearInstances();
            widget->deleteLater();
            delete widget;
            widget = nullptr;
        }
        m_eventHandlerWidgets.clear();
    }

    void EventHandlerWidgets::Clear()
    {
        ClearEventHandlerWidgets();

        if (m_btnAddEventHandler != nullptr)
        {
            m_layout->removeWidget(m_btnAddEventHandler);
            delete m_btnAddEventHandler;
            m_btnAddEventHandler = nullptr;
        }

        if (m_btnAddInheritanceEventHandler != nullptr)
        {
            m_layout->removeWidget(m_btnAddInheritanceEventHandler);
            delete m_btnAddInheritanceEventHandler;
            m_btnAddInheritanceEventHandler = nullptr;
        }
    }

    void EventHandlerWidgets::AddEventHandler(const AZStd::string& moduleName, AZStd::any* instance)
    {
        EventHandlerWidget* widget = new EventHandlerWidget(moduleName, instance, m_serializeContext, m_pnotify, this);
        m_eventHandlerWidgets.push_back(widget);
        m_layout->addWidget(widget);
        widget->Refresh();

    }

    void EventHandlerWidgets::DeleteEventHandler(AZStd::string& moduleName)
    {
        EffectorInspector* inspector = dynamic_cast<EffectorInspector*>(m_parent);
        if (inspector != nullptr)
        {
            inspector->DeleteEventHandler(moduleName);
        }
    }

    int EventHandlerWidgets::GetEventEmitterName(size_t index)
    {
        for (auto& widget : m_eventHandlerWidgets)
        {
            int emitterIndex = widget->GetEventEmitterName(index);
            if (emitterIndex != -1)
            {
                return emitterIndex;
            }
        }

        return -1;
    }

    void EventHandlerWidgets::SetCheckBoxEnable(bool check) const
    {
        m_btnAddEventHandler->setEnabled(check);
        
        bool inheritance = false;
        for (auto& widget : m_eventHandlerWidgets)
        {
            if (widget->m_moduleId == azrtti_typeid<OpenParticle::InheritanceHandler>())
            {
                inheritance = true;
                break;
            }
        }
        m_btnAddInheritanceEventHandler->setEnabled(check && !inheritance);
    }
}
