/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Core/Widgets/ViewportSettingsWidgets.h>
#include <EditorViewportSettings.h>

ViewportFieldOfViewPropertyWidget::ViewportFieldOfViewPropertyWidget()
{
    // Set label name.
    m_label->setText("Field of View");

    // Set suffix on SpinBox
    m_spinBox->setSuffix(" deg");

    // Set starting value.
    m_spinBox->setValue(SandboxEditor::CameraDefaultFovDegrees());

    // Set minimum and maximun.
    // These should be set after the starting value if the default (0.0) is outside the range.
    m_spinBox->setMinimum(30.0);
    m_spinBox->setMaximum(120.0);

    const int DefaultViewportId = 0;
    AzToolsFramework::ViewportInteraction::ViewportSettingsNotificationBus::Handler::BusConnect(DefaultViewportId);
}

ViewportFieldOfViewPropertyWidget::~ViewportFieldOfViewPropertyWidget()
{
    AzToolsFramework::ViewportInteraction::ViewportSettingsNotificationBus::Handler::BusDisconnect();
}

void ViewportFieldOfViewPropertyWidget::OnSpinBoxValueChanged(double newValue)
{
    SandboxEditor::SetCameraDefaultFovDegrees(aznumeric_cast<float>(newValue));
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
    m_label->setText("Camera Speed Scale");

    // Set starting value.
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
    m_label->setText("Grid Snapping Size");

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
    m_label->setText("Angle Snapping Size");

    // Set suffix on SpinBox
    m_spinBox->setSuffix(" deg");

    // Set starting value.
    m_spinBox->setValue(SandboxEditor::AngleSnappingSize());
}

void ViewportAngleSnappingSizePropertyWidget::OnSpinBoxValueChanged(double newValue)
{
    SandboxEditor::SetAngleSnappingSize(aznumeric_cast<float>(newValue));
}
