/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <QLabel>
#include <QListView>
#include <QScopedValueRollback>
#include <QVBoxLayout>

#include <AzFramework/DocumentPropertyEditor/ReflectionAdapter.h>

#include <AzToolsFramework/PaintBrushSettings/PaintBrushSettingsRequestBus.h>
#include <AzToolsFramework/PaintBrushSettings/PaintBrushSettingsWindow.h>
#include <AzToolsFramework/PaintBrushSettings/PaintBrushSettingsWindow_Internals.h>
#include <AzToolsFramework/API/ViewPaneOptions.h>

#include <UI/DocumentPropertyEditor/DocumentPropertyEditor.h>
#include <UI/PropertyEditor/PropertyRowWidget.hxx>
#include <UI/PropertyEditor/ReflectedPropertyEditor.hxx>

namespace PaintBrush
{
    class PaintBrushSettingsWindow;

    namespace Internal
    {
        PaintBrushSettingsWindow::PaintBrushSettingsWindow(QWidget* parent)
        {
            setObjectName("PaintBrushSettings");
            setParent(parent);

            QVBoxLayout* mainLayout = new QVBoxLayout();
            mainLayout->setContentsMargins(0, 0, 0, 0);
            mainLayout->setSpacing(0);

            AZ::ComponentApplicationBus::BroadcastResult(m_serializeContext, &AZ::ComponentApplicationRequests::GetSerializeContext);
            AZ_Assert(m_serializeContext, "Failed to retrieve serialize context.");

            m_propertyEditor = new AzToolsFramework::ReflectedPropertyEditor(this);
            m_propertyEditor->Setup(m_serializeContext, this, true);

            AzToolsFramework::PaintBrushSettings* settings = nullptr;
            AzToolsFramework::PaintBrushSettingsRequestBus::BroadcastResult(
                settings, &AzToolsFramework::PaintBrushSettingsRequestBus::Events::GetSettings);

            m_propertyEditor->ClearInstances();
            m_propertyEditor->AddInstance(settings, azrtti_typeid(*settings), nullptr);

            m_propertyEditor->InvalidateAll();
            m_propertyEditor->setEnabled(false);

            m_propertyEditor->setObjectName("PaintBrushSettingsPropertyEditor");
            m_propertyEditor->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
            mainLayout->addWidget(m_propertyEditor);
            m_propertyEditor->show();

            setLayout(mainLayout);

            AzToolsFramework::PaintBrushNotificationBus::Handler::BusConnect();
        }

        PaintBrushSettingsWindow::~PaintBrushSettingsWindow()
        {
            AzToolsFramework::PaintBrushNotificationBus::Handler::BusDisconnect();
        }

        void PaintBrushSettingsWindow::OnPaintModeBegin([[maybe_unused]] const AZ::EntityComponentIdPair& id)
        {
            m_propertyEditor->setEnabled(true);
        }

        void PaintBrushSettingsWindow::OnPaintModeEnd([[maybe_unused]] const AZ::EntityComponentIdPair& id)
        {
            m_propertyEditor->setEnabled(false);
        }

        // simple factory method
        PaintBrushSettingsWindow* CreateNewPaintBrushSettingsWindow(QWidget* parent)
        {
            return new PaintBrushSettingsWindow(parent);
        }
    } // namespace PaintBrush::Internal

    void RegisterPaintBrushSettingsWindow()
    {
        AzToolsFramework::ViewPaneOptions viewOptions;
        // Don't list this pane in the Tools menu, it will only be visible and accessible while in painting mode.
        viewOptions.showInMenu = false;
        // Don't enable/disable based on entering component mode. We'll enable more specifically ourselves based on when painting begins.
        viewOptions.isDisabledInComponentMode = false;
        // Default size of the window
        viewOptions.paneRect = QRect(50, 50, 350, 105);

        AzToolsFramework::EditorRequestBus::Broadcast(
            &AzToolsFramework::EditorRequestBus::Events::RegisterViewPane,
            s_paintBrushSettingsName,
            "Tools",
            viewOptions,
            &Internal::CreateNewPaintBrushSettingsWindow);
    }
} // namespace Camera
