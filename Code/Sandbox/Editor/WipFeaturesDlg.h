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
#ifndef CRYINCLUDE_EDITOR_WIPFEATURESDLG_H
#define CRYINCLUDE_EDITOR_WIPFEATURESDLG_H

#if !defined(Q_MOC_RUN)
#include <QDialog>
#endif

#ifdef USE_WIP_FEATURES_MANAGER

// CWipFeaturesDlg dialog

namespace Ui
{
    class WipFeaturesDlg;
}

class CWipFeaturesDlg
    : public QDialog
{
    Q_OBJECT
public:
    CWipFeaturesDlg(QWidget* pParent = NULL);   // standard constructor
    virtual ~CWipFeaturesDlg();

private:
    void OnInitDialog();
    void OnBnClickedButtonShow();
    void OnBnClickedButtonHide();
    void OnBnClickedButtonEnable();
    void OnBnClickedButtonDisable();
    void OnBnClickedButtonSafemode();
    void OnBnClickedButtonNormalmode();

private:
    QScopedPointer<Ui::WipFeaturesDlg> m_ui;
};

#else

class CWipFeaturesDlg
    : public QDialog
{
    Q_OBJECT
};

#endif // USE_WIP_FEATURES_MANAGER

#endif // CRYINCLUDE_EDITOR_WIPFEATURESDLG_H
