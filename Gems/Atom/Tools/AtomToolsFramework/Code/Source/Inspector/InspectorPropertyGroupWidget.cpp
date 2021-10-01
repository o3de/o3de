/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AtomToolsFramework/Inspector/InspectorPropertyGroupWidget.h>
#include <AzToolsFramework/UI/PropertyEditor/ReflectedPropertyEditor.hxx>
#include <AzToolsFramework/UI/PropertyEditor/PropertyEditorAPI.h>
#include <AzCore/Component/ComponentApplicationBus.h>

namespace AtomToolsFramework
{
    InspectorPropertyGroupWidget::InspectorPropertyGroupWidget(
        void* instance,
        void* instanceToCompare,
        const AZ::Uuid& instanceClassId,
        AzToolsFramework::IPropertyEditorNotify* instanceNotificationHandler,
        QWidget* parent,
        const AZ::u32 saveStateKey,
        const AzToolsFramework::InstanceDataHierarchy::ValueComparisonFunction& valueComparisonFunction,
        const AzToolsFramework::IndicatorQueryFunction& indicatorQueryFunction,
        int leafIndentSize)
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
        m_propertyEditor->SetAutoResizeLabels(false);
        m_propertyEditor->SetLeafIndentation(leafIndentSize);
        m_propertyEditor->SetValueComparisonFunction(valueComparisonFunction);
        m_propertyEditor->SetIndicatorQueryFunction(indicatorQueryFunction);
        m_propertyEditor->SetSavedStateKey(saveStateKey);
        m_propertyEditor->Setup(context, instanceNotificationHandler, false);
        m_propertyEditor->AddInstance(instance, instanceClassId, nullptr, instanceToCompare);
        m_propertyEditor->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
        m_propertyEditor->InvalidateAll();

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

    AzToolsFramework::ReflectedPropertyEditor* InspectorPropertyGroupWidget::GetPropertyEditor()
    {
        return m_propertyEditor;
    }
} // namespace AtomToolsFramework

#include <AtomToolsFramework/Inspector/moc_InspectorPropertyGroupWidget.cpp>
