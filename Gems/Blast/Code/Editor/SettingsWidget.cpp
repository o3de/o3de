/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "StdAfx.h"

#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzToolsFramework/UI/PropertyEditor/InstanceDataHierarchy.h>
#include <AzToolsFramework/UI/PropertyEditor/ReflectedPropertyEditor.hxx>
#include <Editor/SettingsWidget.h>
#include <QBoxLayout>

namespace Blast
{
    namespace Editor
    {
        SettingsWidget::SettingsWidget(QWidget* parent)
            : QWidget(parent)
        {
            CreatePropertyEditor(this);
        }

        void SettingsWidget::SetValue(const BlastGlobalConfiguration& configuration)
        {
            m_configuration = configuration;

            blockSignals(true);
            m_propertyEditor->ClearInstances();
            m_propertyEditor->AddInstance(&m_configuration);
            m_propertyEditor->InvalidateAll();
            blockSignals(false);
        }

        void SettingsWidget::CreatePropertyEditor(QWidget* parent)
        {
            QVBoxLayout* verticalLayout = new QVBoxLayout(parent);
            verticalLayout->setContentsMargins(0, 0, 0, 0);
            verticalLayout->setSpacing(0);

            AZ::SerializeContext* m_serializeContext;
            AZ::ComponentApplicationBus::BroadcastResult(
                m_serializeContext, &AZ::ComponentApplicationRequests::GetSerializeContext);
            AZ_Assert(m_serializeContext, "Failed to retrieve serialize context.");

            static const int propertyLabelWidth = 250;
            m_propertyEditor = new AzToolsFramework::ReflectedPropertyEditor(parent);
            m_propertyEditor->Setup(m_serializeContext, this, true, propertyLabelWidth);
            m_propertyEditor->show();
            m_propertyEditor->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

            verticalLayout->addWidget(m_propertyEditor);
        }

        void SettingsWidget::BeforePropertyModified(AzToolsFramework::InstanceDataNode* /*node*/) {}

        void SettingsWidget::AfterPropertyModified(AzToolsFramework::InstanceDataNode* /*node*/)
        {
            emit onValueChanged(m_configuration);
        }

        void SettingsWidget::SetPropertyEditingActive(AzToolsFramework::InstanceDataNode* /*node*/) {}

        void SettingsWidget::SetPropertyEditingComplete(AzToolsFramework::InstanceDataNode* /*node*/) {}

        void SettingsWidget::SealUndoStack() {}
    } // namespace Editor
} // namespace Blast

#include <Editor/moc_SettingsWidget.cpp>
