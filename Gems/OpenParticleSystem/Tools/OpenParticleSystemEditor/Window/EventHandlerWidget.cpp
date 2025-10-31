/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <EventHandlerWidget.h>
#include <Window/EffectorInspector.h>
#include <AzToolsFramework/UI/PropertyEditor/PropertyRowWidget.hxx>
#include <AzToolsFramework/UI/PropertyEditor/PropertyStringComboBoxCtrl.hxx>

namespace OpenParticleSystemEditor
{
    EventHandlerWidget::EventHandlerWidget(
        const AZStd::string& moduleName,
        AZStd::any* instance,
        AZ::SerializeContext* serializeContext,
        AzToolsFramework::IPropertyEditorNotify* pnotify,
        QWidget* parent)
        : QWidget(parent)
        , m_moduleName(moduleName)
        , m_parent(parent)
        , m_moduleId(instance->get_type_info().m_id)
    {
        m_propertyEditor = new AzToolsFramework::ReflectedPropertyEditor(this);
        m_propertyEditor->SetHideRootProperties(false);
        m_propertyEditor->SetAutoResizeLabels(true);
        m_propertyEditor->SetValueComparisonFunction({});
        m_propertyEditor->SetSavedStateKey({});
        m_propertyEditor->Setup(serializeContext, pnotify, false);
        m_propertyEditor->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Minimum);
        m_propertyEditor->AddInstance(
            AZStd::any_cast<void>(instance), instance->get_type_info().m_id, nullptr, nullptr);
        m_btnDelete = new QToolButton(this);
        m_btnDelete->setIcon(QIcon(":/Gallery/img/UI20/Delete.svg"));
        connect(m_btnDelete, &QToolButton::pressed, this, &EventHandlerWidget::OnClickDelete);
        m_layout = new QHBoxLayout(this);
        m_layout->addWidget(m_btnDelete, 0, Qt::AlignTop);
        m_layout->addWidget(m_propertyEditor);
        setLayout(m_layout);
    }

    void EventHandlerWidget::Refresh() const
    {
        m_propertyEditor->QueueInvalidation(AzToolsFramework::PropertyModificationRefreshLevel::Refresh_EntireTree);
    }

    void EventHandlerWidget::ClearInstances()
    {
        m_layout->removeWidget(m_propertyEditor);
        m_layout->removeWidget(m_btnDelete);
        m_propertyEditor->deleteLater();
        m_btnDelete->deleteLater();

        delete m_btnDelete;
        m_btnDelete = nullptr;

        m_propertyEditor->ClearInstances();
        delete m_propertyEditor;
        m_propertyEditor = nullptr;
    }

    void EventHandlerWidget::OnClickDelete()
    {
        EventHandlerWidgets* widget = dynamic_cast<EventHandlerWidgets*>(m_parent);
        if (widget != nullptr)
        {
            widget->DeleteEventHandler(m_moduleName);
        }
    }

    int EventHandlerWidget::GetEventEmitterName(size_t index)
    {
        auto instance = m_propertyEditor->GetNodeAtIndex(static_cast<int>(index));
        auto widgetList = m_propertyEditor->GetWidgetFromNode(instance)->GetChildrenRows();
        for (auto& iter : widgetList)
        {
            auto comboBox = dynamic_cast<AzToolsFramework::PropertyStringComboBoxCtrl*>(iter->GetChildWidget());
            if (comboBox && comboBox->hasFocus())
            {
                return comboBox->GetCurrentIndex();
            }
        }
        return -1;
    }
}
