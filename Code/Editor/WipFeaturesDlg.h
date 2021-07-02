/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


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
