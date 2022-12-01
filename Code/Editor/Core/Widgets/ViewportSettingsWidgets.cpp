/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Core/Widgets/ViewportSettingsWidgets.h>
#include <EditorViewportSettings.h>

#include <QHBoxLayout>

constexpr int FieldMargins = 18;

PropertyInputDoubleWidget::PropertyInputDoubleWidget()
{
    // Create Label.
    m_label = new QLabel(this);
    m_label->setContentsMargins(QMargins(0, 0, FieldMargins/2, 0));

    // Create SpinBox.
    m_spinBox = new AzQtComponents::DoubleSpinBox(this);
    // TODO - turn 64 into a constant
    m_spinBox->setFixedWidth(64);

    // Trigger OnSpinBoxValueChanged when the user changes the value.
    connect(
        m_spinBox,
        QOverload<double>::of(&AzQtComponents::DoubleSpinBox::valueChanged),
        this,
        [&](double value)
        {
            OnSpinBoxValueChanged(value);
        }
    );

    // Clear focus when the user is done editing (especially if this is added to a menu).
    QObject::connect(m_spinBox, &AzQtComponents::DoubleSpinBox::editingFinished, &QWidget::clearFocus);

    // Add to Layout.
    QHBoxLayout* layout = new QHBoxLayout(this);
    layout->setContentsMargins(QMargins(FieldMargins, 0, FieldMargins, 0));
    layout->addWidget(m_label);
    layout->addWidget(m_spinBox);
}

PropertyInputDoubleWidget::~PropertyInputDoubleWidget()
{
}

ViewportFieldOfViewPropertyWidget::ViewportFieldOfViewPropertyWidget()
{
    // Set label name.
    m_label->setText("Field of View");

    // Set suffix on SpinBox
    m_spinBox->setSuffix("deg");
    m_spinBox->setMinimum(30.0);
    m_spinBox->setMaximum(120.0);

    // Set starting value.
    m_spinBox->setValue(SandboxEditor::CameraDefaultFovDegrees());
}

void ViewportFieldOfViewPropertyWidget::OnSpinBoxValueChanged(double newValue)
{
    SandboxEditor::SetCameraDefaultFovDegrees(aznumeric_cast<float>(newValue));
}

ViewportCameraSpeedScalePropertyWidget::ViewportCameraSpeedScalePropertyWidget()
{
    // Set label name.
    m_label->setText("Camera Speed Scale");

    // Set starting value.
    m_spinBox->setValue(SandboxEditor::CameraSpeedScale());
}

void ViewportCameraSpeedScalePropertyWidget::OnSpinBoxValueChanged(double newValue)
{
    SandboxEditor::SetCameraSpeedScale(aznumeric_cast<float>(newValue));
}
