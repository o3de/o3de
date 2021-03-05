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

#include <PhysX_precompiled.h>
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzToolsFramework/UI/PropertyEditor/ReflectedPropertyEditor.hxx>
#include <AzToolsFramework/UI/PropertyEditor/InstanceDataHierarchy.h>
#include <QBoxLayout>
#include <Editor/SettingsWidget.h>
#include <Editor/DocumentationLinkWidget.h>
#include <Source/NameConstants.h>

namespace PhysX
{
    namespace Editor
    {
        static const char* const s_settingsDocumentationLink = "Learn more about <a href=%0>configuring PhysX</a>";
        static const char* const s_settingsDocumentationAddress = "configuration/global";

        SettingsWidget::SettingsWidget(QWidget* parent)
            : QWidget(parent)
        {
            CreatePropertyEditor(this);
        }

        void SettingsWidget::SetValue(const AZ::Data::Asset<Physics::MaterialLibraryAsset>& materialLibrary,
            const AzPhysics::SceneConfiguration& defaultSceneConfiguration,
            const Debug::DebugDisplayData& debugDisplayData,
            const PhysX::WindConfiguration& windConfiguration)
        {
            m_defaultPhysicsMaterialLibrary.m_asset = materialLibrary;
            m_defaultSceneConfiguration = defaultSceneConfiguration;
            m_debugDisplayData = debugDisplayData;
            m_windConfiguration = windConfiguration;

            blockSignals(true);
            m_propertyEditor->ClearInstances();
            m_propertyEditor->AddInstance(&m_defaultPhysicsMaterialLibrary);
            m_propertyEditor->AddInstance(&m_defaultSceneConfiguration.m_legacyConfiguration);
            m_propertyEditor->AddInstance(&m_debugDisplayData);
            m_propertyEditor->AddInstance(&m_windConfiguration);
            m_propertyEditor->InvalidateAll();
            blockSignals(false);
        }

        void SettingsWidget::CreatePropertyEditor(QWidget* parent)
        {
            QVBoxLayout* verticalLayout = new QVBoxLayout(parent);
            verticalLayout->setContentsMargins(0, 0, 0, 0);
            verticalLayout->setSpacing(0);

            m_documentationLinkWidget = new DocumentationLinkWidget(s_settingsDocumentationLink, (UXNameConstants::GetPhysXDocsRoot() + s_settingsDocumentationAddress).c_str());

            AZ::SerializeContext* m_serializeContext;
            AZ::ComponentApplicationBus::BroadcastResult(m_serializeContext, &AZ::ComponentApplicationRequests::GetSerializeContext);
            AZ_Assert(m_serializeContext, "Failed to retrieve serialize context.");

            const int propertyLabelWidth = 250;
            m_propertyEditor = new AzToolsFramework::ReflectedPropertyEditor(parent);
            m_propertyEditor->Setup(m_serializeContext, this, true, propertyLabelWidth);
            m_propertyEditor->show();
            m_propertyEditor->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

            verticalLayout->addWidget(m_documentationLinkWidget);
            verticalLayout->addWidget(m_propertyEditor);
        }

        void SettingsWidget::BeforePropertyModified(AzToolsFramework::InstanceDataNode* /*node*/)
        {
        }

        void SettingsWidget::AfterPropertyModified(AzToolsFramework::InstanceDataNode* /*node*/)
        {
        }

        void SettingsWidget::SetPropertyEditingActive(AzToolsFramework::InstanceDataNode* /*node*/)
        {
        }

        void SettingsWidget::SetPropertyEditingComplete(AzToolsFramework::InstanceDataNode* /*node*/)
        {
            emit onValueChanged(m_defaultPhysicsMaterialLibrary.m_asset,
                m_defaultSceneConfiguration,
                m_debugDisplayData,
                m_windConfiguration
            );
        }

        void SettingsWidget::SealUndoStack()
        {
        }
    } // Editor
} // PhysX

#include <Editor/moc_SettingsWidget.cpp>
