/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Core/Widgets/ViewportSettingsWidgets.h>
#include <EditorViewportSettings.h>
#include <MainWindow.h>
#include <ViewPane.h>
#include <Viewport.h>

ViewportFieldOfViewPropertyWidget::ViewportFieldOfViewPropertyWidget()
{
    // Set label name.
    m_label->setText(tr("Field of View"));

    // Set suffix on SpinBox
    m_spinBox->setSuffix(" deg");

    // Set starting value.
    m_spinBox->setValue(SandboxEditor::CameraDefaultFovDegrees());

    // Set minimum and maximun.
    // These should be set after the starting value if the default (0.0) is outside the range.
    m_spinBox->setMinimum(30.0);
    m_spinBox->setMaximum(120.0);


}

void ViewportFieldOfViewPropertyWidget::showEvent(QShowEvent* event)
{
    AzQtComponents::PropertyInputDoubleWidget::showEvent(event);
    auto* viewport = MainWindow::instance()->GetActiveViewport();
    if (viewport)
    {
        AzToolsFramework::ViewportInteraction::ViewportSettingsNotificationBus::Handler::BusDisconnect();
        AzToolsFramework::ViewportInteraction::ViewportSettingsNotificationBus::Handler::BusConnect(viewport->GetViewportId());
        OnCameraFovChanged(viewport->GetFOV());
    }
}

ViewportFieldOfViewPropertyWidget::~ViewportFieldOfViewPropertyWidget()
{
    AzToolsFramework::ViewportInteraction::ViewportSettingsNotificationBus::Handler::BusDisconnect();
}

void ViewportFieldOfViewPropertyWidget::OnSpinBoxValueChanged(double newValue)
{
    if (auto* pane = MainWindow::instance()->GetActiveView())
    {
        pane->SetViewportFOV(aznumeric_cast<float>(newValue));
    }
}

void ViewportFieldOfViewPropertyWidget::OnCameraFovChanged(float fovRadians)
{
    const float fovDegrees = AZ::RadToDeg(fovRadians);

    // Prevent signaling to avoid infinite loop.
    m_spinBox->blockSignals(true);
    m_spinBox->setValue(fovDegrees);
    m_spinBox->blockSignals(false);
}

ViewportCameraSpeedScalePropertyWidget::ViewportCameraSpeedScalePropertyWidget()
{
    // Set label name.
    m_label->setText(tr("Camera Speed Scale"));

    // Set starting value.
    m_spinBox->setMaximum(1.0e6f);
    m_spinBox->setValue(SandboxEditor::CameraSpeedScale());

    const int DefaultViewportId = 0;
    AzToolsFramework::ViewportInteraction::ViewportSettingsNotificationBus::Handler::BusConnect(DefaultViewportId);
}

ViewportCameraSpeedScalePropertyWidget::~ViewportCameraSpeedScalePropertyWidget()
{
    AzToolsFramework::ViewportInteraction::ViewportSettingsNotificationBus::Handler::BusDisconnect();
}

void ViewportCameraSpeedScalePropertyWidget::OnSpinBoxValueChanged(double newValue)
{
    SandboxEditor::SetCameraSpeedScale(aznumeric_cast<float>(newValue));
}

void ViewportCameraSpeedScalePropertyWidget::OnCameraSpeedScaleChanged(float value)
{
    // Prevent signaling to avoid infinite loop.
    m_spinBox->blockSignals(true);
    m_spinBox->setValue(value);
    m_spinBox->blockSignals(false);
}

ViewportGridSnappingSizePropertyWidget::ViewportGridSnappingSizePropertyWidget()
{
    // Set label name.
    m_label->setText(tr("Grid Snapping Size"));

    // Set suffix on SpinBox
    m_spinBox->setSuffix(" m");

    // Set starting value.
    m_spinBox->setValue(SandboxEditor::GridSnappingSize());
}

void ViewportGridSnappingSizePropertyWidget::OnSpinBoxValueChanged(double newValue)
{
    SandboxEditor::SetGridSnappingSize(aznumeric_cast<float>(newValue));
}

ViewportAngleSnappingSizePropertyWidget::ViewportAngleSnappingSizePropertyWidget()
{
    // Set label name.
    m_label->setText(tr("Angle Snapping Size"));

    // Set suffix on SpinBox
    m_spinBox->setSuffix(" deg");

    // Set starting value.
    m_spinBox->setValue(SandboxEditor::AngleSnappingSize());
}

void ViewportAngleSnappingSizePropertyWidget::OnSpinBoxValueChanged(double newValue)
{
    SandboxEditor::SetAngleSnappingSize(aznumeric_cast<float>(newValue));
}
