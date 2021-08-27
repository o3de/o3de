/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzToolsFramework/UI/PropertyEditor/ReflectedPropertyEditor.hxx>
#include <AzToolsFramework/UI/PropertyEditor/InstanceDataHierarchy.h>
#include <QBoxLayout>
#include <Editor/PvdWidget.h>
#include <Editor/DocumentationLinkWidget.h>
#include <Source/NameConstants.h>

namespace PhysX
{
    namespace Editor
    {
        static const char* const s_pvdDocumentationLink = "Learn more about the <a href=%0>PhysX Visual Debugger (PVD).</a>";
        static const char* const s_pvdDocumentationAddress = "configuring/configuration-debugger";

        PvdWidget::PvdWidget(QWidget* parent)
            : QWidget(parent)
        {
            CreatePropertyEditor(this);
        }

        void PvdWidget::SetValue(const Debug::PvdConfiguration& configuration)
        {
            m_config = configuration;

            blockSignals(true);
            m_propertyEditor->ClearInstances();
            m_propertyEditor->AddInstance(&m_config);
            m_propertyEditor->InvalidateAll();
            blockSignals(false);
        }

        void PvdWidget::CreatePropertyEditor(QWidget* parent)
        {
            QVBoxLayout* verticalLayout = new QVBoxLayout(parent);
            verticalLayout->setContentsMargins(0, 0, 0, 0);
            verticalLayout->setSpacing(0);

            m_documentationLinkWidget = new DocumentationLinkWidget(s_pvdDocumentationLink, (UXNameConstants::GetPhysXDocsRoot() + s_pvdDocumentationAddress).c_str());

            AZ::SerializeContext* m_serializeContext;
            AZ::ComponentApplicationBus::BroadcastResult(m_serializeContext, &AZ::ComponentApplicationRequests::GetSerializeContext);
            AZ_Assert(m_serializeContext, "Failed to retrieve serialize context.");

            static const int propertyLabelWidth = 250;
            m_propertyEditor = new AzToolsFramework::ReflectedPropertyEditor(parent);
            m_propertyEditor->Setup(m_serializeContext, this, true, propertyLabelWidth);
            m_propertyEditor->show();
            m_propertyEditor->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

            verticalLayout->addWidget(m_documentationLinkWidget);
            verticalLayout->addWidget(m_propertyEditor);
        }

        void PvdWidget::BeforePropertyModified(AzToolsFramework::InstanceDataNode* /*node*/)
        {

        }

        void PvdWidget::AfterPropertyModified(AzToolsFramework::InstanceDataNode* /*node*/)
        {
            emit onValueChanged(m_config);
        }

        void PvdWidget::SetPropertyEditingActive(AzToolsFramework::InstanceDataNode* /*node*/)
        {

        }

        void PvdWidget::SetPropertyEditingComplete(AzToolsFramework::InstanceDataNode* /*node*/)
        {
            emit onValueChanged(m_config);
        }

        void PvdWidget::SealUndoStack()
        {

        }
    } // Editor
} // PhysX

#include <Editor/moc_PvdWidget.cpp>
