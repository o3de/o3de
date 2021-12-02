/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#pragma once

#if !defined(Q_MOC_RUN)
#include <QWidget>
#include <QString>
#include <QPixmap>
#endif

/////////////////////////////////////////////////////////////////////////////
// CStartupLogoDialog dialog

namespace Ui
{
    class StartupLogoDialog;
}

class CStartupLogoDialog
    : public QWidget
    , public IInitializeUIInfo
{
    Q_OBJECT

public:
    CStartupLogoDialog(QString versionText, QString richTextCopyrightNotice, QWidget* pParent = nullptr);   // standard constructor
    ~CStartupLogoDialog();

    void SetInfoText(const char* text) override;

    // Static way to call SetInfoText on the single instance of CStartupLogoDialog
    static void SetText(const char* text);

    static CStartupLogoDialog* instance() { return s_pLogoWindow; }

private:

    void SetInfo(const char* text);
    void paintEvent(QPaintEvent* event) override;

    static CStartupLogoDialog*              s_pLogoWindow;

    QScopedPointer<Ui::StartupLogoDialog>   m_ui;

    QPixmap                                 m_backgroundImage;
    const int                               m_enforcedWidth = 668;
    const int                               m_enforcedHeight = 368;
};

