#pragma once
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

#if !defined(Q_MOC_RUN)
#include <AzToolsFramework/UI/PropertyEditor/PropertyEditorAPI_Internals.h>
#include <AzFramework/Physics/World.h>
#include <QWidget>
#include <PhysX/Configuration/PhysXConfiguration.h>
#endif

#pragma once

namespace PhysX
{
    namespace Editor
    {
        class DocumentationLinkWidget;

        class PvdWidget
            : public QWidget
            , private AzToolsFramework::IPropertyEditorNotify
        {
            Q_OBJECT

        public:
            AZ_CLASS_ALLOCATOR(PvdWidget, AZ::SystemAllocator, 0);

            explicit PvdWidget(QWidget* parent = nullptr);

            void SetValue(const Debug::PvdConfiguration& configuration);

        signals:
            void onValueChanged(const Debug::PvdConfiguration& configuration);

        private:
            void CreatePropertyEditor(QWidget* parent);

            void BeforePropertyModified(AzToolsFramework::InstanceDataNode* /*node*/) override;
            void AfterPropertyModified(AzToolsFramework::InstanceDataNode* /*node*/) override;
            void SetPropertyEditingActive(AzToolsFramework::InstanceDataNode* /*node*/) override;
            void SetPropertyEditingComplete(AzToolsFramework::InstanceDataNode* /*node*/) override;
            void SealUndoStack() override;

            AzToolsFramework::ReflectedPropertyEditor* m_propertyEditor;
            DocumentationLinkWidget* m_documentationLinkWidget;
            Debug::PvdConfiguration m_config;
            Physics::WorldConfiguration m_worldConfiguration;
        };
    }
}
