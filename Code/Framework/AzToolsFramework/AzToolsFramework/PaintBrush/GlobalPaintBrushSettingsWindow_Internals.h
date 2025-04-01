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
#include <AzToolsFramework/PaintBrush/GlobalPaintBrushSettingsNotificationBus.h>
#include <AzToolsFramework/UI/PropertyEditor/PropertyEditorAPI_Internals.h>

namespace AZ
{
    class SerializeContext;
} // namespace AZ

namespace AzToolsFramework
{
    class ReflectedPropertyEditor;

    namespace Internal
    {
        //! PaintBrushSettingsWindow is a simple view pane that lets us view and edit the global paint brush settings.
        //! This pane is only visible while in a painting component mode.
        //! Unlike other component modes, this is built as a separate pane because the controls might get fairly complex over time
        //! and will likely get used frequently while painting, so the user should have the ability to move and dock these settings
        //! to wherever is best for their paint session.
        class GlobalPaintBrushSettingsWindow
            : public QListView
            , private AzToolsFramework::IPropertyEditorNotify
            , private AzToolsFramework::GlobalPaintBrushSettingsNotificationBus::Handler
        {
        public:
            GlobalPaintBrushSettingsWindow(QWidget* parent = nullptr);
            ~GlobalPaintBrushSettingsWindow();

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

        private:
            void OnVisiblePropertiesChanged() override;
            void OnSettingsChanged([[maybe_unused]] const GlobalPaintBrushSettings& newSettings) override;

            // RPE Support
            AzToolsFramework::ReflectedPropertyEditor* m_propertyEditor = nullptr;
            AZ::SerializeContext* m_serializeContext = nullptr;
        };

        // simple factory method
        GlobalPaintBrushSettingsWindow* CreateNewPaintBrushSettingsWindow(QWidget* parent = nullptr);
    } // namespace AzToolsFramework::Internal
} // namespace AzToolsFramework
