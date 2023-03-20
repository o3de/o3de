/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "GotoPositionDlg.h"
#include "EditorDefs.h"

// Editor
#include "EditorViewportCamera.h"
#include "EditorViewportSettings.h"
#include "GameEngine.h"
#include "ViewManager.h"

#include <AzFramework/Viewport/CameraInput.h>

#include <AzCore/Math/Transform.h>
#include <AzCore/Math/Vector3.h>

AZ_PUSH_DISABLE_DLL_EXPORT_MEMBER_WARNING
#include <ui_GotoPositionDlg.h>
AZ_POP_DISABLE_DLL_EXPORT_MEMBER_WARNING

void GoToPositionPitchConstraints::DeterminePitchRange(const AngleRangeConfigureFn& configurePitchRangeFn) const
{
    const auto [pitchMinRadians, pitchMaxRadians] = AzFramework::CameraPitchMinMaxRadians();
    configurePitchRangeFn(AZ::RadToDeg(pitchMinRadians), AZ::RadToDeg(pitchMaxRadians));
}

float GoToPositionPitchConstraints::PitchClampedRadians(float pitchDegrees) const
{
    return AzFramework::ClampPitchRotation(AZ::DegToRad(pitchDegrees));
}

GotoPositionDialog::GotoPositionDialog(QWidget* parent)
    : QDialog(parent)
    , m_ui(new Ui::GotoPositionDialog)
{
    m_ui->setupUi(this);
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    setFixedSize(size());
    OnInitDialog();

    auto doubleValueChanged = static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged);

    connect(m_ui->m_posEdit, &QLineEdit::editingFinished, this, &GotoPositionDialog::OnChangeEdit);
    connect(m_ui->m_dymX, doubleValueChanged, this, &GotoPositionDialog::OnUpdateNumbers);
    connect(m_ui->m_dymY, doubleValueChanged, this, &GotoPositionDialog::OnUpdateNumbers);
    connect(m_ui->m_dymZ, doubleValueChanged, this, &GotoPositionDialog::OnUpdateNumbers);
    connect(m_ui->m_dymAnglePitch, doubleValueChanged, this, &GotoPositionDialog::OnUpdateNumbers);
    connect(m_ui->m_dymAngleYaw, doubleValueChanged, this, &GotoPositionDialog::OnUpdateNumbers);
}

GotoPositionDialog::~GotoPositionDialog() = default;

void GotoPositionDialog::OnInitDialog()
{
    const auto cameraTransform = SandboxEditor::GetDefaultViewportCameraTransform();
    const auto cameraTranslation = cameraTransform.GetTranslation();
    const auto cameraRotation = AzFramework::EulerAngles(AZ::Matrix3x3::CreateFromQuaternion(cameraTransform.GetRotation()));
    const auto pitchDegrees = AZ::RadToDeg(cameraRotation.GetX());
    const auto yawDegrees = AZ::RadToDeg(cameraRotation.GetZ());

    // position
    const double CameraPositionExtent = 64000.0;
    m_ui->m_dymX->setRange(-CameraPositionExtent, CameraPositionExtent);
    m_ui->m_dymX->setValue(cameraTranslation.GetX());
    m_ui->m_dymY->setRange(-CameraPositionExtent, CameraPositionExtent);
    m_ui->m_dymY->setValue(cameraTranslation.GetY());
    m_ui->m_dymZ->setRange(-CameraPositionExtent, CameraPositionExtent);
    m_ui->m_dymZ->setValue(cameraTranslation.GetZ());

    // rotation
    m_goToPositionPitchConstraints.DeterminePitchRange(
        [this](const float minPitchDegrees, const float maxPitchDegrees)
        {
            m_ui->m_dymAnglePitch->setRange(minPitchDegrees, maxPitchDegrees);
        });
    m_ui->m_dymAnglePitch->setValue(pitchDegrees);

    m_ui->m_dymAngleYaw->setRange(-360, 360);
    m_ui->m_dymAngleYaw->setValue(yawDegrees);

    // ensure the goto button is highlighted correctly.
    m_ui->pushButton->setDefault(true);

    OnUpdateNumbers();
}

void GotoPositionDialog::OnChangeEdit()
{
    const int argCount = 5;
    AZStd::vector<float> transform(argCount);

    m_transform = m_ui->m_posEdit->text();
    const QStringList parts = m_transform.split(QRegularExpression("[\\s,;\\t]"), Qt::SkipEmptyParts);
    for (int i = 0; i < argCount && i < parts.count(); ++i)
    {
        transform[i] = parts[i].toFloat();
    }

    m_ui->m_dymX->setValue(transform[0]);
    m_ui->m_dymY->setValue(transform[1]);
    m_ui->m_dymZ->setValue(transform[2]);
    m_ui->m_dymAnglePitch->setValue(transform[3]);
    m_ui->m_dymAngleYaw->setValue(transform[4]);
}

void GotoPositionDialog::OnUpdateNumbers()
{
    m_ui->m_posEdit->setText(QString::fromLatin1("%1, %2, %3, %4, %5")
                                 .arg(m_ui->m_dymX->value(), 2, 'f', 2)
                                 .arg(m_ui->m_dymY->value(), 2, 'f', 2)
                                 .arg(m_ui->m_dymZ->value(), 2, 'f', 2)
                                 .arg(m_ui->m_dymAnglePitch->value(), 2, 'f', 2)
                                 .arg(m_ui->m_dymAngleYaw->value(), 2, 'f', 2));
}

void GotoPositionDialog::accept()
{
    const auto position = AZ::Vector3(
        aznumeric_cast<float>(m_ui->m_dymX->value()),
        aznumeric_cast<float>(m_ui->m_dymY->value()),
        aznumeric_cast<float>(m_ui->m_dymZ->value()));
    const auto pitchRadians = m_goToPositionPitchConstraints.PitchClampedRadians(aznumeric_cast<float>(m_ui->m_dymAnglePitch->value()));
    const auto yawRadians = AZ::DegToRad(aznumeric_cast<float>(m_ui->m_dymAngleYaw->value()));

    // either set or interpolate the camera to the new position and orientation
    SandboxEditor::HandleDefaultViewportCameraTransitionFromSetting(position, pitchRadians, yawRadians);

    QDialog::accept();
}

#include <moc_GotoPositionDlg.cpp>
