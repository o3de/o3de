/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzQtComponents/Components/Widgets/SpinBox.h>

#include <QLabel>
#include <QWidget>

// Base Widget to allow double property value input edits.
class PropertyInputDoubleWidget
    : public QWidget
{
public:
    PropertyInputDoubleWidget();
    ~PropertyInputDoubleWidget();

protected:
    // TODO - add function to initialize value.
    // TODO - add way to update the widget if the value is changed elsewhere.
    virtual void OnSpinBoxValueChanged(double newValue) = 0;

    QLabel* m_label = nullptr;
    AzQtComponents::DoubleSpinBox* m_spinBox = nullptr;
};

// Field of View Widget
class ViewportFieldOfViewPropertyWidget
    : public PropertyInputDoubleWidget
{
public:
    ViewportFieldOfViewPropertyWidget();

private:
    void OnSpinBoxValueChanged(double newValue) override;
};

// Camera Speed Scale Widget
class ViewportCameraSpeedScalePropertyWidget
    : public PropertyInputDoubleWidget
{
public:
    ViewportCameraSpeedScalePropertyWidget();

private:
    void OnSpinBoxValueChanged(double newValue) override;
};
