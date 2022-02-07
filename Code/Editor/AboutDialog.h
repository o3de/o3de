/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#pragma once

#if !defined(Q_MOC_RUN)
#include <QDialog>
#include <QString>
#include <QPixmap>
#endif

namespace Ui {
    class CAboutDialog;
}

class CAboutDialog
    : public QDialog
{
    Q_OBJECT

public:
    CAboutDialog(QString versionText, QString richTextCopyrightNotice, QWidget* pParent = nullptr);
    ~CAboutDialog();

private:

    void OnCustomerAgreement();

    void mouseReleaseEvent(QMouseEvent* event) override;
    void paintEvent(QPaintEvent* event) override;

    QScopedPointer<Ui::CAboutDialog>    m_ui;
    QPixmap                             m_backgroundImage;

    const int m_imageWidth = 668;
    const int m_imageHeight = 368;
    const int m_enforcedWidth = 600;
    const int m_enforcedHeight = 300;
};

