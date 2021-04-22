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
        AZ_CLASS_ALLOCATOR(InspectorPropertyGroupWidget, AZ::SystemAllocator, 0);

        InspectorPropertyGroupWidget(
            void* instance,
            void* instanceToCompare,
            const AZ::Uuid& instanceClassId,
            AzToolsFramework::IPropertyEditorNotify* instanceNotificationHandler = {},
            QWidget* parent = {},
            const AzToolsFramework::InstanceDataHierarchy::ValueComparisonFunction& valueComparisonFunction = {},
            QString title = QString());

        void Refresh() override;
        void Rebuild() override;

        AzToolsFramework::ReflectedPropertyEditor* GetPropertyEditor();

    private:
        QVBoxLayout* m_layout = {};
        AzToolsFramework::ReflectedPropertyEditor* m_propertyEditor = {};
    };
} // namespace AtomToolsFramework
