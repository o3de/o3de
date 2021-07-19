/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once
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
