/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#pragma once
#ifndef CRYINCLUDE_EDITOR_DIMENSIONSDIALOG_H
#define CRYINCLUDE_EDITOR_DIMENSIONSDIALOG_H

#if !defined(Q_MOC_RUN)
#include <QScopedPointer>

#include <QDialog>
#endif

class QButtonGroup;

namespace Ui {
    class CDimensionsDialog;
}

class CDimensionsDialog
    : public QDialog
{
    Q_OBJECT

public:
    CDimensionsDialog(QWidget* pParent = nullptr);   // standard constructor
    ~CDimensionsDialog();

    UINT GetDimensions();
    void SetDimensions(unsigned int iWidth);

protected:
    void UpdateData(bool fromUi = true);    // DDX/DDV support

private:
    QButtonGroup* m_group;

    QScopedPointer<Ui::CDimensionsDialog> ui;
};

#endif // CRYINCLUDE_EDITOR_DIMENSIONSDIALOG_H
