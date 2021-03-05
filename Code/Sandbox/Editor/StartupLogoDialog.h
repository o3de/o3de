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
    const int                               m_enforcedWidth = 600;
    const int                               m_enforcedHeight = 300;
};

