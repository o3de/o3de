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
