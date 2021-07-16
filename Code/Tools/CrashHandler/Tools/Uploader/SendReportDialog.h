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
#include <QScopedPointer>
#endif

namespace Ui {
    class SendReportDialog;
}

namespace CrashUploader
{

    class SendReportDialog : public QDialog
    {
        Q_OBJECT // AUTOMOC

    public:
        explicit SendReportDialog(QWidget* parent = nullptr);
        ~SendReportDialog() override;

        void SetReportText(const char* reportPath);
        void SetApplicationName(const char* appName);
    private:
        QScopedPointer<Ui::SendReportDialog> ui;
    };

}
