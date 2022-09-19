/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "PaintBrushSettingsWindow.h"
#include "PaintBrushSettingsWindow_Internals.h"
#include <AzToolsFramework/API/ViewPaneOptions.h>
#include <QLabel>
#include <QListView>
#include <QScopedValueRollback>
#include <QVBoxLayout>

#include <AzFramework/DocumentPropertyEditor/ReflectionAdapter.h>
#include <UI/DocumentPropertyEditor/DocumentPropertyEditor.h>

#include <UI/PropertyEditor/PropertyRowWidget.hxx>
#include <UI/PropertyEditor/ReflectedPropertyEditor.hxx>

namespace PaintBrush
{
    class PaintBrushSettingsWindow;

    namespace Internal
    {
        void PaintBrushConfig::Reflect(AZ::ReflectContext* context)
        {
            if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<PaintBrushConfig>()
                    ->Version(1)
                    ->Field("Radius", &PaintBrushConfig::m_radius)
                    ->Field("Intensity", &PaintBrushConfig::m_intensity)
                    ->Field("Opacity", &PaintBrushConfig::m_opacity);

                if (auto editContext = serializeContext->GetEditContext())
                {
                    editContext->Class<PaintBrushConfig>("Paint Brush", "")
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                        ->DataElement(AZ::Edit::UIHandlers::Slider, &PaintBrushConfig::m_radius, "Radius", "Radius of the paint brush.")
                        ->Attribute(AZ::Edit::Attributes::Min, 0.01f)
                        ->Attribute(AZ::Edit::Attributes::SoftMin, 1.0f)
                        ->Attribute(AZ::Edit::Attributes::Max, 1024.0f)
                        ->Attribute(AZ::Edit::Attributes::SoftMax, 100.0f)
                        ->Attribute(AZ::Edit::Attributes::Step, 0.25f)
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, &PaintBrushConfig::OnRadiusChange)
                        ->DataElement(
                            AZ::Edit::UIHandlers::Slider, &PaintBrushConfig::m_intensity, "Intensity", "Intensity of the paint brush.")
                        ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                        ->Attribute(AZ::Edit::Attributes::Max, 1.0f)
                        ->Attribute(AZ::Edit::Attributes::Step, 0.025f)
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, &PaintBrushConfig::OnIntensityChange)
                        ->DataElement(AZ::Edit::UIHandlers::Slider, &PaintBrushConfig::m_opacity, "Opacity", "Opacity of the paint brush.")
                        ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                        ->Attribute(AZ::Edit::Attributes::Max, 1.0f)
                        ->Attribute(AZ::Edit::Attributes::Step, 0.025f)
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, &PaintBrushConfig::OnOpacityChange);
                }
            }
        }

        void PaintBrushConfig::SetRadius(float radius)
        {
            m_radius = radius;
            OnRadiusChange();
        }

        void PaintBrushConfig::SetIntensity(float intensity)
        {
            m_intensity = intensity;
            OnIntensityChange();
        }

        void PaintBrushConfig::SetOpacity(float opacity)
        {
            m_opacity = opacity;
            OnOpacityChange();
        }

        AZ::u32 PaintBrushConfig::OnIntensityChange()
        {
            // Notify listeners that the configuration changed. This is used to synchronize the paint brush settings with the manipulator.
            AzToolsFramework::PaintBrushSettingsNotificationBus::Broadcast(
                &AzToolsFramework::PaintBrushSettingsNotificationBus::Events::OnIntensityChanged, m_intensity);
            return AZ::Edit::PropertyRefreshLevels::ValuesOnly;
        }

        AZ::u32 PaintBrushConfig::OnOpacityChange()
        {
            // Notify listeners that the configuration changed. This is used to synchronize the paint brush settings with the manipulator.
            AzToolsFramework::PaintBrushSettingsNotificationBus::Broadcast(
                &AzToolsFramework::PaintBrushSettingsNotificationBus::Events::OnOpacityChanged, m_opacity);
            return AZ::Edit::PropertyRefreshLevels::ValuesOnly;
        }

        AZ::u32 PaintBrushConfig::OnRadiusChange()
        {
            // Notify listeners that the configuration changed. This is used to synchronize the paint brush settings with the manipulator.
            AzToolsFramework::PaintBrushSettingsNotificationBus::Broadcast(
                &AzToolsFramework::PaintBrushSettingsNotificationBus::Events::OnRadiusChanged, m_radius);
            return AZ::Edit::PropertyRefreshLevels::ValuesOnly;
        }

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

            m_propertyEditor->ClearInstances();
            m_propertyEditor->AddInstance(&m_paintBrush, azrtti_typeid(m_paintBrush), nullptr);

            m_propertyEditor->InvalidateAll();
            m_propertyEditor->setEnabled(false);

            m_propertyEditor->setObjectName("PaintBrushSettingsPropertyEditor");
            m_propertyEditor->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
            mainLayout->addWidget(m_propertyEditor);
            m_propertyEditor->show();

            setLayout(mainLayout);

            AzToolsFramework::PaintBrushSettingsRequestBus::Handler::BusConnect();
            AzToolsFramework::PaintBrushNotificationBus::Handler::BusConnect();
        }

        PaintBrushSettingsWindow::~PaintBrushSettingsWindow()
        {
            AzToolsFramework::PaintBrushNotificationBus::Handler::BusDisconnect();
            AzToolsFramework::PaintBrushSettingsRequestBus::Handler::BusDisconnect();
        }

        void PaintBrushSettingsWindow::OnPaintModeBegin([[maybe_unused]] const AZ::EntityComponentIdPair& id)
        {
            m_propertyEditor->setEnabled(true);
        }

        void PaintBrushSettingsWindow::OnPaintModeEnd([[maybe_unused]] const AZ::EntityComponentIdPair& id)
        {
            m_propertyEditor->setEnabled(false);
        }

        float PaintBrushSettingsWindow::GetRadius() const
        {
            return m_paintBrush.GetRadius();
        }

        float PaintBrushSettingsWindow::GetIntensity() const
        {
            return m_paintBrush.GetIntensity();
        }

        float PaintBrushSettingsWindow::GetOpacity() const
        {
            return m_paintBrush.GetOpacity();
        }

        void PaintBrushSettingsWindow::SetRadius(float radius)
        {
            m_paintBrush.SetRadius(radius);
        }

        void PaintBrushSettingsWindow::SetIntensity(float intensity)
        {
            m_paintBrush.SetIntensity(intensity);
        }

        void PaintBrushSettingsWindow::SetOpacity(float opacity)
        {
            m_paintBrush.SetOpacity(opacity);
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
        viewOptions.showInMenu = true;
        viewOptions.preferedDockingArea = Qt::DockWidgetArea::RightDockWidgetArea;
        // Keep the window active but hidden when it's closed. This is needed so that it still provides the paint brush settings
        // even when they aren't visible.
        viewOptions.isDeletable = false;
        // Don't enable/disable based on entering component mode. We'll enable more specifically ourselves based on when painting begins.
        viewOptions.isDisabledInComponentMode = false;

        AzToolsFramework::EditorRequestBus::Broadcast(
            &AzToolsFramework::EditorRequestBus::Events::RegisterViewPane,
            s_paintBrushSettingsName,
            "Tools",
            viewOptions,
            &Internal::CreateNewPaintBrushSettingsWindow);
    }
} // namespace Camera
