/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <PropertyEditorWidget.h>
#include <Window/EffectorInspector.h>
#include <AzToolsFramework/UI/PropertyEditor/PropertyRowWidget.hxx>
#include <AzToolsFramework/UI/PropertyEditor/PropertyStringComboBoxCtrl.hxx>
#include <Document/ParticleDocumentBus.h>

namespace OpenParticleSystemEditor
{
    PropertyEditorWidget::PropertyEditorWidget(
        const AZStd::string& moduleName,
        const AZStd::string& className,
        bool checked,
        AZStd::any* instance,
        AZ::SerializeContext* serializeContext,
        AzToolsFramework::IPropertyEditorNotify* pnotify,
        QWidget* parent)
        : QWidget(parent)
        , m_moduleName(moduleName)
        , m_className(className)
        , m_parent(parent)
        , m_instance(instance)
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

        OpenParticle::ParticleSourceData* sourceData = nullptr;
        EffectorInspector* inspector = dynamic_cast<EffectorInspector*>(m_parent);
        if (inspector != nullptr)
        {
            m_widgetName = inspector->m_widgetName;
        }
        EBUS_EVENT_ID_RESULT(sourceData, m_widgetName, OpenParticleSystemEditor::ParticleDocumentRequestBus, GetParticleSourceData);
        if (sourceData)
        {
            m_propertyEditor->setEnabled(sourceData->CheckModuleState(m_instance->get_type_info().m_id));
        }

        m_checkBox = new QCheckBox(this);
        m_checkBox->setStyleSheet(QString::fromUtf8("QCheckBox::indicator:unchecked{image: url(:/stylesheet/img/UI20/checkbox/off.svg);}\n"
                              "QCheckBox::indicator:checked{image: url(:/stylesheet/img/UI20/checkbox/on.svg);}"));
        m_checkBox->setChecked(checked);
        bool visibility = false;
        if (m_moduleName != PARTICLE_LINE_NAMES[WIDGET_LINE_EMITTER])
        {
            connect(m_checkBox, SIGNAL(clicked(bool)), this, SLOT(OnClickCheckBox(bool)));
            visibility = true;
        }
        m_checkBox->setVisible(visibility);
        
        m_layout = new QHBoxLayout(this);
        m_layout->addWidget(m_checkBox, 0, Qt::AlignTop);
        m_layout->addWidget(m_propertyEditor);

        setLayout(m_layout);
    }

    void PropertyEditorWidget::Refresh() const
    {
        OpenParticle::ParticleSourceData* sourceData = nullptr;
        EBUS_EVENT_ID_RESULT(sourceData, m_widgetName, OpenParticleSystemEditor::ParticleDocumentRequestBus, GetParticleSourceData);
        if (sourceData)
        {
            m_propertyEditor->setEnabled(sourceData->CheckModuleState(m_instance->get_type_info().m_id));
        }
        m_propertyEditor->QueueInvalidation(AzToolsFramework::PropertyModificationRefreshLevel::Refresh_EntireTree);
    }

    void PropertyEditorWidget::Refresh(AZStd::any* instance)
    {
        m_propertyEditor->ClearInstances();
        m_instance = instance;
        OpenParticle::ParticleSourceData* sourceData = nullptr;
        EBUS_EVENT_ID_RESULT(sourceData, m_widgetName, OpenParticleSystemEditor::ParticleDocumentRequestBus, GetParticleSourceData);
        if (sourceData)
        {
            m_propertyEditor->setEnabled(sourceData->CheckModuleState(m_instance->get_type_info().m_id));
        }
        m_propertyEditor->AddInstance(AZStd::any_cast<void>(instance), m_instance->get_type_info().m_id, nullptr, nullptr);
        m_propertyEditor->QueueInvalidation(AzToolsFramework::PropertyModificationRefreshLevel::Refresh_EntireTree);
    }

    void PropertyEditorWidget::ClearInstances()
    {
        m_layout->removeWidget(m_propertyEditor);
        m_layout->removeWidget(m_checkBox);
        m_propertyEditor->deleteLater();
        m_checkBox->deleteLater();

        delete m_checkBox;
        m_checkBox = nullptr;

        m_propertyEditor->ClearInstances();
        delete m_propertyEditor;
        m_propertyEditor = nullptr;
    }

    void PropertyEditorWidget::OnClickCheckBox(bool bCheck)
    {
        EffectorInspector* inspector = dynamic_cast<EffectorInspector*>(m_parent);
        if (inspector != nullptr)
        {
            inspector->UpdatePropertyEditorWidget(m_moduleName, m_className, bCheck);
        }
    }

    AZStd::string PropertyEditorWidget::GetModuleName()
    {
        return m_moduleName;
    }

    int PropertyEditorWidget::GetEventEmitterName(size_t index)
    {
        auto instance = m_propertyEditor->GetNodeAtIndex(static_cast<int>(index));
        auto widgetList = m_propertyEditor->GetWidgetFromNode(instance)->GetChildrenRows();
        for (auto& iter : widgetList)
        {
            auto comboBox = dynamic_cast<AzToolsFramework::PropertyStringComboBoxCtrl*>(iter->GetChildWidget());
            if (comboBox)
            {
                return comboBox->GetCurrentIndex();
            }
        }
        return -1;
    }

    void PropertyEditorWidget::SetCheckBoxEnable(bool check) const
    {
        if (m_moduleName == PARTICLE_LINE_NAMES[WIDGET_LINE_EMITTER])
        {
            m_checkBox->setEnabled(false);
        }
        else
        {
            m_checkBox->setEnabled(check);
        }
    }

    void PropertyEditorWidget::SetChecked(bool checked) const
    {
        m_checkBox->setChecked(checked);
        m_propertyEditor->setEnabled(checked);
    }
}
