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
#pragma once
////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2002.
// -------------------------------------------------------------------------
//  File name:   newterraindialog.h
//  Version:     v1.00
//  Created:     24/7/2002 by Timur.
//  Compilers:   Visual Studio.NET
//  Description:
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////
#ifndef CRYINCLUDE_EDITOR_NEWTERRAINDIALOG_H
#define CRYINCLUDE_EDITOR_NEWTERRAINDIALOG_H

#if !defined(Q_MOC_RUN)
#include <QScopedPointer>

#include <vector>

#include <QDialog>
#endif

namespace Ui
{
    class CNewTerrainDialog;

    enum TerrainDialogConstants
    {
        START_TERRAIN_RESOLUTION_POWER_OF_TWO = 7,
        START_TERRAIN_RESOLUTION = 1 << START_TERRAIN_RESOLUTION_POWER_OF_TWO,
        MAXIMUM_TERRAIN_POWER_OF_TWO = 16,
        MAXIMUM_TERRAIN_RESOLUTION = 1 << MAXIMUM_TERRAIN_POWER_OF_TWO,
        POWER_OFFSET = (MAXIMUM_TERRAIN_POWER_OF_TWO - START_TERRAIN_RESOLUTION_POWER_OF_TWO),
        START_TERRAIN_UNITS = 1
    };
}

class CNewTerrainDialog
    : public QDialog
{
    Q_OBJECT

public:
    CNewTerrainDialog(QWidget* pParent = nullptr);   // standard constructor
    ~CNewTerrainDialog();

    int GetTerrainResolution() const;
    int GetTerrainUnits() const;

    void IsResize(bool bIsResize);


protected:
    void UpdateData(bool fromUi = true);
    void OnInitDialog();

    void UpdateTerrainUnits();
    void UpdateTerrainInfo();

    void showEvent(QShowEvent* event) override;

protected slots:
    void OnComboBoxSelectionTerrainResolution();
    void OnComboBoxSelectionTerrainUnits();

public:
    int m_terrainResolutionIndex;
    int m_terrainUnitsIndex;
    bool m_bUpdate;

    QScopedPointer<Ui::CNewTerrainDialog> ui;
    bool m_initialized;
};
#endif // CRYINCLUDE_EDITOR_NEWTERRAINDIALOG_H
