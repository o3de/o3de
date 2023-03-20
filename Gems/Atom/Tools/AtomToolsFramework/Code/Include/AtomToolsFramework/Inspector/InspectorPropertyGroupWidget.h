/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include <AzCore/Memory/SystemAllocator.h>
#include <AzToolsFramework/UI/PropertyEditor/ReflectedPropertyEditor.hxx>
#include <AtomToolsFramework/Inspector/InspectorGroupWidget.h>

AZ_PUSH_DISABLE_WARNING(4251 4800, "-Wunknown-warning-option") // disable warnings spawned by QT
#include <QVBoxLayout>
#include <QWidget>
AZ_POP_DISABLE_WARNING
#endif

namespace AzToolsFramework
{
    class IPropertyEditorNotify;
    class ReflectedPropertyEditor;
}

namespace AtomToolsFramework
{
    class InspectorPropertyGroupWidget
        : public InspectorGroupWidget
    {
        Q_OBJECT
    public:
        AZ_CLASS_ALLOCATOR(InspectorPropertyGroupWidget, AZ::SystemAllocator);

        InspectorPropertyGroupWidget(
            void* instance,
            void* instanceToCompare,
            const AZ::Uuid& instanceClassId,
            AzToolsFramework::IPropertyEditorNotify* instanceNotificationHandler = {},
            QWidget* parent = {},
            const AZ::u32 saveStateKey = {},
            const AzToolsFramework::InstanceDataHierarchy::ValueComparisonFunction& valueComparisonFunction = {},
            const AzToolsFramework::IndicatorQueryFunction& indicatorQueryFunction = {},
            int leafIndentSize = 16);

        void Refresh() override;
        void Rebuild() override;

        AzToolsFramework::ReflectedPropertyEditor* GetPropertyEditor();

    private:
        QVBoxLayout* m_layout = {};
        AzToolsFramework::ReflectedPropertyEditor* m_propertyEditor = {};
    };
} // namespace AtomToolsFramework
