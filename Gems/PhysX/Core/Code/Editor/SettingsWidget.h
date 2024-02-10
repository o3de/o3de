/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include <AzToolsFramework/UI/PropertyEditor/PropertyEditorAPI_Internals.h>
#include <AzFramework/Physics/Configuration/SceneConfiguration.h>
#include <QWidget>
#include <PhysX/Configuration/PhysXConfiguration.h>
#endif

namespace PhysX
{
    namespace Editor
    {
        class DocumentationLinkWidget;

        class SettingsWidget
            : public QWidget
            , private AzToolsFramework::IPropertyEditorNotify
        {
            Q_OBJECT

        public:
            AZ_CLASS_ALLOCATOR(SettingsWidget, AZ::SystemAllocator);

            explicit SettingsWidget(QWidget* parent = nullptr);

            void SetValue(const PhysX::PhysXSystemConfiguration& physxSystemConfiguration,
                const AzPhysics::SceneConfiguration& defaultSceneConfiguration,
                const Debug::DebugDisplayData& debugDisplayData);

        signals:
            void onValueChanged(const PhysX::PhysXSystemConfiguration& physxSystemConfiguration,
                const AzPhysics::SceneConfiguration& defaultSceneConfiguration,
                const Debug::DebugDisplayData& debugDisplayData);

        private:
            void CreatePropertyEditor(QWidget* parent);

            void BeforePropertyModified(AzToolsFramework::InstanceDataNode* /*node*/) override;
            void AfterPropertyModified(AzToolsFramework::InstanceDataNode* /*node*/) override;
            void SetPropertyEditingActive(AzToolsFramework::InstanceDataNode* /*node*/) override;
            void SetPropertyEditingComplete(AzToolsFramework::InstanceDataNode* /*node*/) override;
            void SealUndoStack() override;

            AzToolsFramework::ReflectedPropertyEditor* m_propertyEditor;
            DocumentationLinkWidget* m_documentationLinkWidget;
            PhysX::PhysXSystemConfiguration m_physxSystemConfiguration;
            AzPhysics::SceneConfiguration m_defaultSceneConfiguration;
            Debug::DebugDisplayData m_debugDisplayData;
        };
    }
}
