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

#include <EnvelopeInfo.h>

namespace Ui
{
    class SentryReportDialog;
}

namespace CrashHandler
{
    class SentryReportDialog : public QDialog
    {
        Q_OBJECT // AUTOMOC

    public:
        explicit SentryReportDialog(QWidget* parent = nullptr);
        ~SentryReportDialog() override;

        // Populates the metadata chip row and the event-id readout from a parsed crash envelope.
        void SetEnvelopeInfo(const EnvelopeInfo& info);

        QString GetUserComments() const;
        // Named GetContactName (not GetUserName) to avoid colliding with the <windows.h>
        // GetUserName -> GetUserNameA/W macro that pollutes any TU that includes it.
        QString GetContactName() const;
        QString GetUserEmail() const;
        bool WantsRestart() const { return m_wantsRestart; }

        //! True only when a level was actually recovered and the user left the box ticked. The
        //! save itself already happened during the crash; this governs whether the restart
        //! reopens it.
        bool WantsLevelRestored() const;

    private:
        void ApplyTheme();

        QScopedPointer<Ui::SentryReportDialog> ui;
        bool m_wantsRestart = false;
    };
}
