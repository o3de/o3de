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

#include <AtomToolsFramework/Inspector/InspectorPropertyGroupWidget.h>
#include <AzToolsFramework/UI/PropertyEditor/ReflectedPropertyEditor.hxx>
#include <AzToolsFramework/UI/PropertyEditor/PropertyEditorAPI.h>

namespace AtomToolsFramework
{
    InspectorPropertyGroupWidget::InspectorPropertyGroupWidget(
        void* object,
        const AZ::Uuid& objectClassId,
        AzToolsFramework::IPropertyEditorNotify* objectNotificationHandler,
        QWidget* parent)
        : InspectorGroupWidget(parent)
    {
        AZ::SerializeContext* context = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(context, &AZ::ComponentApplicationRequests::GetSerializeContext);
        AZ_Assert(context, "No serialize context");

        m_layout = new QVBoxLayout(this);
        m_layout->setContentsMargins(0, 0, 0, 0);
        m_layout->setSpacing(0);

        m_propertyEditor = new AzToolsFramework::ReflectedPropertyEditor(this);
        m_propertyEditor->SetHideRootProperties(true);
        m_propertyEditor->SetAutoResizeLabels(true);
        m_propertyEditor->Setup(context, objectNotificationHandler, false);
        m_propertyEditor->AddInstance(object, objectClassId);
        m_propertyEditor->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
        m_propertyEditor->QueueInvalidation(AzToolsFramework::PropertyModificationRefreshLevel::Refresh_EntireTree);

        m_layout->addWidget(m_propertyEditor);
        setLayout(m_layout);
    }

    void InspectorPropertyGroupWidget::Refresh()
    {
        m_propertyEditor->QueueInvalidation(AzToolsFramework::PropertyModificationRefreshLevel::Refresh_AttributesAndValues);
    }

    void InspectorPropertyGroupWidget::Rebuild()
    {
        m_propertyEditor->QueueInvalidation(AzToolsFramework::PropertyModificationRefreshLevel::Refresh_EntireTree);
    }
} // namespace AtomToolsFramework

#include <AtomToolsFramework/Inspector/moc_InspectorPropertyGroupWidget.cpp>
