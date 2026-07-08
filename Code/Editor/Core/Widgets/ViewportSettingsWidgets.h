/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzQtComponents/PropertyInput/PropertyInputWidgets.h>

#include <AzToolsFramework/Viewport/ViewportMessages.h>

#include <QCoreApplication>

// Field of View Widget
class ViewportFieldOfViewPropertyWidget
    : public AzQtComponents::PropertyInputDoubleWidget
    , private AzToolsFramework::ViewportInteraction::ViewportSettingsNotificationBus::Handler
{
    Q_DECLARE_TR_FUNCTIONS(ViewportFieldOfViewPropertyWidget)

public:
    ViewportFieldOfViewPropertyWidget();
    ~ViewportFieldOfViewPropertyWidget();

private:
    void OnSpinBoxValueChanged(double newValue) override;

    // ViewportSettingsNotificationBus overrides ...
    void OnCameraFovChanged(float fovRadians) override;
};

// Camera Speed Scale Widget
class ViewportCameraSpeedScalePropertyWidget
    : public AzQtComponents::PropertyInputDoubleWidget
    , private AzToolsFramework::ViewportInteraction::ViewportSettingsNotificationBus::Handler
{
    Q_DECLARE_TR_FUNCTIONS(ViewportCameraSpeedScalePropertyWidget)

public:
    ViewportCameraSpeedScalePropertyWidget();
    ~ViewportCameraSpeedScalePropertyWidget();

private:
    void OnSpinBoxValueChanged(double newValue) override;

    // ViewportSettingsNotificationBus overrides ...
    void OnCameraSpeedScaleChanged(float value) override;
};

// Grid Size
class ViewportGridSnappingSizePropertyWidget
    : public AzQtComponents::PropertyInputDoubleWidget
{
    Q_DECLARE_TR_FUNCTIONS(ViewportGridSnappingSizePropertyWidget)

public:
    ViewportGridSnappingSizePropertyWidget();

private:
    void OnSpinBoxValueChanged(double newValue) override;
};

// Angle Snap Interval
class ViewportAngleSnappingSizePropertyWidget
    : public AzQtComponents::PropertyInputDoubleWidget
{
    Q_DECLARE_TR_FUNCTIONS(ViewportAngleSnappingSizePropertyWidget)

public:
    ViewportAngleSnappingSizePropertyWidget();

private:
    void OnSpinBoxValueChanged(double newValue) override;
};
