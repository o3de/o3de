/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
// NewTerrainDialog.cpp : implementation file
//

#include "EditorDefs.h"

#include "NewTerrainDialog.h"


AZ_PUSH_DISABLE_WARNING(4251, "-Wunknown-warning-option")
#include <ui_NewTerrainDialog.h>
AZ_POP_DISABLE_WARNING



CNewTerrainDialog::CNewTerrainDialog(QWidget* pParent /*=nullptr*/)
    : QDialog(pParent)
    , m_terrainResolutionIndex(0)
    , m_terrainUnitsIndex(0)
    , m_bUpdate(false)
    , ui(new Ui::CNewTerrainDialog)
    , m_initialized(false)
{
    ui->setupUi(this);

    setWindowTitle(tr("Terrain options"));

    // Default is 1024x1024, and m_terrainResolution holds an index to the combo box
    m_terrainResolutionIndex = 3;

    connect(ui->TERRAIN_RESOLUTION, SIGNAL(activated(int)), this, SLOT(OnComboBoxSelectionTerrainResolution()));
    connect(ui->TERRAIN_UNITS, SIGNAL(activated(int)), this, SLOT(OnComboBoxSelectionTerrainUnits()));
}


CNewTerrainDialog::~CNewTerrainDialog()
{
}


void CNewTerrainDialog::UpdateData(bool fromUi)
{
    if (fromUi)
    {
        m_terrainResolutionIndex = ui->TERRAIN_RESOLUTION->currentIndex();
        m_terrainUnitsIndex = ui->TERRAIN_UNITS->currentIndex();
    }
    else
    {
        ui->TERRAIN_RESOLUTION->setCurrentIndex(m_terrainResolutionIndex);
        ui->TERRAIN_UNITS->setCurrentIndex(m_terrainUnitsIndex);
    }
}


void CNewTerrainDialog::OnInitDialog()
{
    // Initialize terrain values.
    int resolution = Ui::START_TERRAIN_RESOLUTION;

    // Fill terrain resolution combo box
    for (int i = 0; i < 6; i++)
    {
        ui->TERRAIN_RESOLUTION->addItem(QString("%1x%1").arg(resolution));
        resolution *= 2;
    }

    UpdateTerrainUnits();
    UpdateTerrainInfo();

    // Save data.
    UpdateData(false);
}


void CNewTerrainDialog::UpdateTerrainUnits()
{
    uint32 terrainRes = GetTerrainResolution();
    int size = terrainRes * GetTerrainUnits();
    int maxUnit = IntegerLog2(Ui::MAXIMUM_TERRAIN_RESOLUTION / terrainRes);
    int units = Ui::START_TERRAIN_UNITS;

    ui->TERRAIN_UNITS->clear();
    for (int i = 0; i <= maxUnit; i++)
    {
        ui->TERRAIN_UNITS->addItem(QString::number(units));
        units *= 2;
    }
    if (size > Ui::MAXIMUM_TERRAIN_RESOLUTION)
    {
        m_terrainUnitsIndex = 0;
    }
    ui->TERRAIN_UNITS->setCurrentText(QString::number(m_terrainUnitsIndex));
}


void CNewTerrainDialog::UpdateTerrainInfo()
{
    int sizeX = GetTerrainResolution() * GetTerrainUnits();
    int sizeY = GetTerrainResolution() * GetTerrainUnits();

    QString str;
    if (sizeX >= 1000)
    {
        str = tr("Terrain Size: %1 x %2 Kilometers").arg((float)sizeX / 1000.0f, 0, 'f', 3).arg((float)sizeY / 1000.0f, 0, 'f', 3);
    }
    else if (sizeX > 0)
    {
        str = tr("Terrain Size: %1 x %2 Meters").arg(sizeX).arg(sizeY);
    }
    else
    {
        str = tr("Level will have no terrain");
    }

    ui->TERRAIN_INFO->setText(str);
}


int CNewTerrainDialog::GetTerrainResolution() const
{
    // convert combo box index into resolution value
    return Ui::START_TERRAIN_RESOLUTION * (1 << m_terrainResolutionIndex);
}


int CNewTerrainDialog::GetTerrainUnits() const
{
    // convert combo box index into units value
    return Ui::START_TERRAIN_UNITS * (1 << m_terrainUnitsIndex);
}


void CNewTerrainDialog::OnComboBoxSelectionTerrainResolution()
{
    UpdateData();

    UpdateTerrainUnits();

    UpdateTerrainInfo();
}


void CNewTerrainDialog::OnComboBoxSelectionTerrainUnits()
{
    UpdateData();

    UpdateTerrainInfo();
}


void CNewTerrainDialog::showEvent(QShowEvent* event)
{
    if (!m_initialized)
    {
        OnInitDialog();
        m_initialized = true;
    }
    QDialog::showEvent(event);
}

#include <moc_NewTerrainDialog.cpp>
