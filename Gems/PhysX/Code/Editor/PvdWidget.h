#pragma once
/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#if !defined(Q_MOC_RUN)
#include <AzToolsFramework/UI/PropertyEditor/PropertyEditorAPI_Internals.h>
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
            AZ_CLASS_ALLOCATOR(PvdWidget, AZ::SystemAllocator);

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
        };
    }
}
