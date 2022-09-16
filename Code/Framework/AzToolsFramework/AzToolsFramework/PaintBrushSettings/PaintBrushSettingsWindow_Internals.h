/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <QListView>
#include <AzToolsFramework/Entity/EditorEntityContextBus.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzToolsFramework/Manipulators/PaintBrushManipulator.h>
#include <AzToolsFramework/Manipulators/PaintBrushNotificationBus.h>
#include <AzToolsFramework/Manipulators/PaintBrushRequestBus.h>
#include <AzToolsFramework/UI/PropertyEditor/PropertyEditorAPI_Internals.h>

namespace AZ
{
    class SerializeContext;
} // namespace AZ

namespace AzToolsFramework
{
    class ReflectedPropertyEditor;
} // namespace AzToolsFramework

namespace PaintBrush
{
    namespace Internal
    {
        class PaintBrushSettingsWindow
            : public QListView
            , private AzToolsFramework::IPropertyEditorNotify
            , private AzToolsFramework::PaintBrushNotificationBus::Handler
            , private AzToolsFramework::PaintBrushSettingsRequestBus::Handler
        {
        public:
            PaintBrushSettingsWindow(QWidget* parent = nullptr);
            ~PaintBrushSettingsWindow();

        protected:

            // IPropertyEditorNotify overrides...
            // To use this window with the PropertyEditor, we need to inherit from IPropertyEditorNotify,
            // which requires us to implement these methods. However, we don't need them to actually do anything for this window.
            void BeforePropertyModified([[maybe_unused]] AzToolsFramework::InstanceDataNode* node) override
            {
            }

            void AfterPropertyModified([[maybe_unused]] AzToolsFramework::InstanceDataNode* node) override
            {
            }

            void SetPropertyEditingActive([[maybe_unused]] AzToolsFramework::InstanceDataNode* node) override
            {
            }

            void SetPropertyEditingComplete([[maybe_unused]] AzToolsFramework::InstanceDataNode* node) override
            {
            }

            void SealUndoStack() override
            {
            }

            // PaintBrushNotificationBus overrides...
            void OnPaintModeBegin([[maybe_unused]] const AZ::EntityComponentIdPair& id) override;
            void OnPaintModeEnd([[maybe_unused]] const AZ::EntityComponentIdPair& id) override;

            // PaintBrushSettingsRequestBus overrides...
            float GetRadius() const override;
            float GetIntensity() const override;
            float GetOpacity() const override;
            void SetRadius(float radius) override;
            void SetIntensity(float intensity) override;
            void SetOpacity(float opacity) override;

        private:
            AzToolsFramework::PaintBrushConfig m_paintBrush;

            // RPE Support
            AzToolsFramework::ReflectedPropertyEditor* m_propertyEditor = nullptr;
            AZ::SerializeContext* m_serializeContext = nullptr;
        };

        // simple factory method
        PaintBrushSettingsWindow* CreateNewPaintBrushSettingsWindow(QWidget* parent = nullptr);
    } // namespace PaintBrush::Internal
} // namespace PaintBrush
