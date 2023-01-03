/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include "IEditor.h"
#include <QDialog>
#include <QPixmap>
#include <QString>
#endif

namespace Ui
{
    class StartupLogoDialog;
}

class CStartupLogoDialog
    : public QDialog
    , public IInitializeUIInfo
{
    Q_OBJECT
public:
    static constexpr int EnforcedWidth = 668;
    static constexpr int EnforcedHeight = 368;

    enum DialogType
    {
        Loading,
        About
    };

    CStartupLogoDialog(
        DialogType dialogType, QString versionText, QString richTextCopyrightNotice, QWidget* pParent = nullptr); // standard constructor
    ~CStartupLogoDialog();

    void SetInfoText(const char* text) override;

    // Static way to call SetInfoText on the single instance of CStartupLogoDialog
    static void SetText(const char* text);

    static CStartupLogoDialog* instance() { return s_pLogoWindow; }

protected:
    virtual void focusOutEvent([[maybe_unused]] QFocusEvent* event) override;

private:
    void SetInfo(const char* text);
    void paintEvent(QPaintEvent* event) override;

    static CStartupLogoDialog* s_pLogoWindow;

    QScopedPointer<Ui::StartupLogoDialog> m_ui;
    DialogType m_dialogType;
    QPixmap m_backgroundImage;
};
