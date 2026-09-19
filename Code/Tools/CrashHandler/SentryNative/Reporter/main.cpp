/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <EnvelopeInfo.h>
#include <SentryReportDialog.h>

#include <sentry.h>

#include <QApplication>
#include <QCoreApplication>
#include <QFileInfo>
#include <QProcess>
#include <QString>

namespace
{
    //! The crash report itself is already on its way to Sentry by the time this process starts;
    //! feedback is a separate envelope linked to that event, so this init only needs a transport.
    void SubmitFeedback(
        const CrashHandler::EnvelopeInfo& info,
        const QString& comments,
        const QString& name,
        const QString& email)
    {
        const QByteArray dsn = qgetenv("SENTRY_DSN");
        if (dsn.isEmpty() || comments.isEmpty() || info.eventId.empty())
        {
            return;
        }

        sentry_options_t* options = sentry_options_new();
        sentry_options_set_dsn(options, dsn.constData());
        if (sentry_init(options) != 0)
        {
            return;
        }

        const QByteArray nameUtf8 = name.toUtf8();
        const QByteArray emailUtf8 = email.toUtf8();
        sentry_uuid_t eventId = sentry_uuid_from_string(info.eventId.c_str());
        sentry_value_t feedback = sentry_value_new_feedback(
            comments.toUtf8().constData(),
            emailUtf8.isEmpty() ? nullptr : emailUtf8.constData(),
            nameUtf8.isEmpty() ? nullptr : nameUtf8.constData(),
            &eventId);
        sentry_capture_feedback(feedback);

        sentry_close();
    }

    void RestartApplication(const CrashHandler::EnvelopeInfo& info, bool restoreLevel)
    {
        // The reporter is staged next to the executable that crashed, so its own directory is the
        // right place to relaunch from.
        const QString exePath = QCoreApplication::applicationDirPath() + "/Editor.exe";
        if (!QFileInfo::exists(exePath))
        {
            return;
        }

        QStringList args;
        if (!info.projectPath.empty())
        {
            args << "--project-path" << QString::fromStdString(info.projectPath);
        }
        // The Editor opens any positional argument that carries a level extension, so the
        // recovered file needs no dedicated switch.
        if (restoreLevel && !info.recoveredLevelPath.empty())
        {
            args << QString::fromStdString(info.recoveredLevelPath);
        }
        QProcess::startDetached(exePath, args);
    }
}

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);

    CrashHandler::EnvelopeInfo info;
    if (argc > 1)
    {
        info = CrashHandler::ParseEnvelope(argv[1]);
    }
    info.recoveredLevelPath = CrashHandler::ConsumeRecoveryMarker();

    CrashHandler::SentryReportDialog dialog;
    dialog.SetEnvelopeInfo(info);
    dialog.exec();

    SubmitFeedback(info, dialog.GetUserComments(), dialog.GetContactName(), dialog.GetUserEmail());

    if (dialog.WantsRestart())
    {
        RestartApplication(info, dialog.WantsLevelRestored());
    }

    return 0;
}
