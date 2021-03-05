/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/
// Original file Copyright Crytek GMBH or its affiliates, used under license.

#include "EditorDefs.h"

#include "GridSettingsDialog.h"

// Editor
#include "Settings.h"
#include "Objects/SelectionGroup.h"
#include "ViewManager.h"


AZ_PUSH_DISABLE_DLL_EXPORT_MEMBER_WARNING
#include <ui_GridSettingsDialog.h>
AZ_POP_DISABLE_DLL_EXPORT_MEMBER_WARNING

// CGridSettingsDialog dialog

CGridSettingsDialog::CGridSettingsDialog(QWidget* pParent /*=NULL*/)
    : QDialog(pParent)
    , ui(new Ui::CGridSettingsDialog)
{
    ui->setupUi(this);

    setWindowTitle(tr("Grid/Snap Settings"));

    OnInitDialog();

    connect(ui->m_userDefined, &QCheckBox::clicked, this, &CGridSettingsDialog::OnBnUserDefined);
    connect(ui->m_getFromObject, &QCheckBox::clicked, this, &CGridSettingsDialog::OnBnGetFromObject);
    connect(ui->m_getAnglesFromObject, &QPushButton::clicked, this, &CGridSettingsDialog::OnBnGetAngles);
    connect(ui->m_getTranslationFromObject, &QPushButton::clicked, this, &CGridSettingsDialog::OnBnGetTranslation);

    auto doubleSpinBoxValueChanged = static_cast<void(QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged);

    connect(ui->m_angleX, doubleSpinBoxValueChanged, this, &CGridSettingsDialog::OnValueUpdate);
    connect(ui->m_angleY, doubleSpinBoxValueChanged, this, &CGridSettingsDialog::OnValueUpdate);
    connect(ui->m_angleZ, doubleSpinBoxValueChanged, this, &CGridSettingsDialog::OnValueUpdate);

    connect(ui->m_gridSize, doubleSpinBoxValueChanged, this, &CGridSettingsDialog::OnValueUpdate);
    connect(ui->m_gridScale, doubleSpinBoxValueChanged, this, &CGridSettingsDialog::OnValueUpdate);
    connect(ui->m_CPSize, doubleSpinBoxValueChanged, this, &CGridSettingsDialog::OnValueUpdate);

    connect(ui->m_displayCP, &QCheckBox::clicked, this, &CGridSettingsDialog::OnValueUpdate);
    connect(ui->m_getFromObject, &QCheckBox::clicked, this, &CGridSettingsDialog::OnValueUpdate);

    connect(ui->m_buttonBox, &QDialogButtonBox::accepted, this, &CGridSettingsDialog::accept);
    connect(ui->m_buttonBox, &QDialogButtonBox::rejected, this, &CGridSettingsDialog::reject);
}

CGridSettingsDialog::~CGridSettingsDialog()
{
}

//////////////////////////////////////////////////////////////////////////
void CGridSettingsDialog::OnInitDialog()
{
    CGrid* pGrid = GetIEditor()->GetViewManager()->GetGrid();

    ui->m_userDefined->setChecked(gSettings.snap.bGridUserDefined);
    ui->m_getFromObject->setChecked(gSettings.snap.bGridGetFromSelected);

    ui->m_angleX->setValue(pGrid->rotationAngles.x);
    ui->m_angleY->setValue(pGrid->rotationAngles.y);
    ui->m_angleZ->setValue(pGrid->rotationAngles.z);

    ui->m_translationX->setValue(pGrid->translation.x);
    ui->m_translationY->setValue(pGrid->translation.y);
    ui->m_translationZ->setValue(pGrid->translation.z);

    ui->m_gridSize->setValue(pGrid->size);
    ui->m_gridScale->setValue(pGrid->scale);
    ui->m_snapToGrid->setChecked(pGrid->IsEnabled());

    ui->m_angleSnap->setChecked(pGrid->IsAngleSnapEnabled());
    ui->m_angleSnapScale->setValue(pGrid->GetAngleSnap());
    ui->m_displayCP->setChecked(gSettings.snap.constructPlaneDisplay);

    ui->m_CPSize->setValue(gSettings.snap.constructPlaneSize);
    ui->m_displaySnapMarker->setChecked(gSettings.snap.markerDisplay);
    ui->m_snapMarkerSize->setValue(gSettings.snap.markerSize);

    ui->m_snapMarkerColor->SetColor(gSettings.snap.markerColor);

    EnableGridPropertyControls(gSettings.snap.bGridUserDefined, gSettings.snap.bGridGetFromSelected);
}

//////////////////////////////////////////////////////////////////////////
void CGridSettingsDialog::accept()
{
    UpdateValues();
    gSettings.Save();
    QDialog::accept();
}

void CGridSettingsDialog::OnBnUserDefined()
{
    EnableGridPropertyControls(ui->m_userDefined->isChecked(), ui->m_getFromObject->isChecked());
    OnValueUpdate();
}

void CGridSettingsDialog::OnBnGetFromObject()
{
    EnableGridPropertyControls(ui->m_userDefined->isChecked(), ui->m_getFromObject->isChecked());
}

void CGridSettingsDialog::OnBnGetAngles()
{
    CSelectionGroup* sel = GetIEditor()->GetSelection();
    if (sel->GetCount() > 0)
    {
        CBaseObject* obj = sel->GetObject(0);
        Matrix34 tm = obj->GetWorldTM();
        AffineParts ap;
        ap.SpectralDecompose(tm);

        Vec3 rotation = Vec3(RAD2DEG(Ang3::GetAnglesXYZ(Matrix33(ap.rot))));

        ui->m_angleX->setValue(rotation.x);
        ui->m_angleY->setValue(rotation.y);
        ui->m_angleZ->setValue(rotation.z);
    }
}

void CGridSettingsDialog::OnBnGetTranslation()
{
    CSelectionGroup* sel = GetIEditor()->GetSelection();
    if (sel->GetCount() > 0)
    {
        CBaseObject* obj = sel->GetObject(0);
        Matrix34 tm = obj->GetWorldTM();
        Vec3 translation = tm.GetTranslation();

        ui->m_translationX->setValue(translation.x);
        ui->m_translationY->setValue(translation.y);
        ui->m_translationZ->setValue(translation.z);
    }
}

void CGridSettingsDialog::EnableGridPropertyControls(const bool isUserDefined, const bool isGetFromObject)
{
    ui->m_getFromObject->setEnabled(isUserDefined == true);

    ui->m_angleX->setEnabled(isUserDefined == true && isGetFromObject == false);
    ui->m_angleY->setEnabled(isUserDefined == true && isGetFromObject == false);
    ui->m_angleZ->setEnabled(isUserDefined == true && isGetFromObject == false);
    ui->m_translationX->setEnabled(isUserDefined == true && isGetFromObject == false);
    ui->m_translationY->setEnabled(isUserDefined == true && isGetFromObject == false);
    ui->m_translationZ->setEnabled(isUserDefined == true && isGetFromObject == false);
    ui->m_getAnglesFromObject->setEnabled(isUserDefined == true && isGetFromObject == false);
    ui->m_getTranslationFromObject->setEnabled(isUserDefined == true && isGetFromObject == false);
}

//////////////////////////////////////////////////////////////////////////
void CGridSettingsDialog::UpdateValues()
{
    CGrid* pGrid = GetIEditor()->GetViewManager()->GetGrid();

    pGrid->Enable(ui->m_snapToGrid->isChecked());
    pGrid->size = ui->m_gridSize->value();
    pGrid->scale = ui->m_gridScale->value();

    gSettings.snap.bGridUserDefined = ui->m_userDefined->isChecked();
    gSettings.snap.bGridGetFromSelected = ui->m_getFromObject->isChecked();
    pGrid->rotationAngles.x = ui->m_angleX->value();
    pGrid->rotationAngles.y = ui->m_angleY->value();
    pGrid->rotationAngles.z = ui->m_angleZ->value();
    pGrid->translation.x = ui->m_translationX->value();
    pGrid->translation.y = ui->m_translationY->value();
    pGrid->translation.z = ui->m_translationZ->value();

    pGrid->bAngleSnapEnabled = ui->m_angleSnap->isChecked();
    pGrid->angleSnap = ui->m_angleSnapScale->value();

    gSettings.snap.constructPlaneDisplay = ui->m_displayCP->isChecked();
    gSettings.snap.constructPlaneSize = ui->m_CPSize->value();

    gSettings.snap.markerDisplay = ui->m_displaySnapMarker->isChecked();
    gSettings.snap.markerSize = ui->m_snapMarkerSize->value();
    gSettings.snap.markerColor = ui->m_snapMarkerColor->Color();

    NotificationBus::Broadcast(&Notifications::OnGridValuesUpdated);
}


//////////////////////////////////////////////////////////////////////////
void CGridSettingsDialog::OnValueUpdate()
{
    UpdateValues();
    GetIEditor()->UpdateViews(eRedrawViewports);
}

#include <moc_GridSettingsDialog.cpp>
