/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "SentryReportDialog.h"
#include "UI/ui_sentry_submit_report.h"

#include <QApplication>
#include <QCheckBox>
#include <QPalette>
#include <QPushButton>

namespace CrashHandler
{
    namespace
    {
        // Accent purple matches Sentry's own crash-reporter reference
        // (github.com/getsentry/sentry-desktop-crash-reporter); the rest of the palette is ours.
        constexpr const char* AccentPurple = "#8866FF";
        constexpr const char* AccentPurpleHover = "#7554FF";
        constexpr const char* AccentPurplePressed = "#5A3FCC";

        const char* DarkStyleSheet()
        {
            return R"(
                QDialog { background-color: #1B1B1F; color: #F1F1F3; }
                QLabel { color: #F1F1F3; }
                QLabel#header_label { font-weight: 300; }
                QLabel#feedback_label { color: #C9C9CE; }
                QLabel[chip="true"] { color: #B7B4C4; }
                QLabel#event_id_label { color: #7A7883; font-family: Consolas, monospace; }
                QPlainTextEdit, QLineEdit {
                    background-color: #26262C;
                    color: #F1F1F3;
                    border: none;
                    border-bottom: 2px solid #45454E;
                    padding: 6px;
                }
                QPlainTextEdit:focus, QLineEdit:focus { border-bottom: 2px solid #8866FF; }
                QPushButton#close_button {
                    background-color: transparent;
                    color: #F1F1F3;
                    border: 1px solid #45454E;
                    border-radius: 4px;
                    padding: 6px 18px;
                }
                QPushButton#close_button:hover { border-color: #6B6B76; }
            )";
        }

        const char* LightStyleSheet()
        {
            return R"(
                QDialog { background-color: #FFFFFF; color: #1B1B1F; }
                QLabel { color: #1B1B1F; }
                QLabel#header_label { font-weight: 300; }
                QLabel#feedback_label { color: #4A4A52; }
                QLabel[chip="true"] { color: #55535F; }
                QLabel#event_id_label { color: #8A8894; font-family: Consolas, monospace; }
                QPlainTextEdit, QLineEdit {
                    background-color: #FFFFFF;
                    color: #1B1B1F;
                    border: 1px solid #D8D7DE;
                    border-bottom: 2px solid #D8D7DE;
                    padding: 6px;
                }
                QPlainTextEdit:focus, QLineEdit:focus { border-bottom: 2px solid #8866FF; }
                QPushButton#close_button {
                    background-color: transparent;
                    color: #1B1B1F;
                    border: 1px solid #D8D7DE;
                    border-radius: 4px;
                    padding: 6px 18px;
                }
                QPushButton#close_button:hover { border-color: #B8B6C0; }
            )";
        }

        bool IsDarkTheme()
        {
            return QApplication::palette().color(QPalette::Window).lightness() < 128;
        }
    }

    SentryReportDialog::SentryReportDialog(QWidget* parent)
        : QDialog(parent)
        , ui(new Ui::SentryReportDialog)
    {
        ui->setupUi(this);

        for (QLabel* chip : { ui->exception_chip, ui->release_chip, ui->os_chip, ui->environment_chip })
        {
            chip->setProperty("chip", true);
        }

        ApplyTheme();

        QString restartStyle = QString(R"(
            QPushButton#restart_button {
                background-color: %1;
                color: #FFFFFF;
                border: none;
                border-radius: 4px;
                padding: 6px 18px;
            }
            QPushButton#restart_button:hover { background-color: %2; }
            QPushButton#restart_button:pressed { background-color: %3; }
        )").arg(AccentPurple, AccentPurpleHover, AccentPurplePressed);
        setStyleSheet(styleSheet() + restartStyle);

        connect(ui->close_button, &QPushButton::clicked, this, [this]()
        {
            m_wantsRestart = false;
            accept();
        });
        connect(ui->restart_button, &QPushButton::clicked, this, [this]()
        {
            m_wantsRestart = true;
            accept();
        });
    }

    SentryReportDialog::~SentryReportDialog() = default;

    void SentryReportDialog::ApplyTheme()
    {
        setStyleSheet(IsDarkTheme() ? DarkStyleSheet() : LightStyleSheet());
    }

    void SentryReportDialog::SetEnvelopeInfo(const EnvelopeInfo& info)
    {
        ui->exception_chip->setText(
            info.exceptionType.empty() ? QString() : QString::fromStdString(info.exceptionType));
        ui->release_chip->setText(
            info.release.empty() ? QString() : QString::fromStdString(info.release));

        QString os = QString::fromStdString(info.osName);
        if (!info.osVersion.empty())
        {
            os += " " + QString::fromStdString(info.osVersion);
        }
        ui->os_chip->setText(os);

        ui->environment_chip->setText(
            info.environment.empty() ? QString() : QString::fromStdString(info.environment));

        if (!info.eventId.empty())
        {
            ui->event_id_label->setText(QString::fromStdString(info.eventId).left(8));
        }

        // Only offer the restore when the emergency save actually produced a file - showing a
        // dead checkbox would imply work was recovered when none was.
        const bool hasRecovery = !info.recoveredLevelPath.empty();
        ui->restore_level_checkbox->setVisible(hasRecovery);
        if (hasRecovery)
        {
            ui->restore_level_checkbox->setToolTip(
                QString::fromStdString(info.recoveredLevelPath));
        }
    }

    bool SentryReportDialog::WantsLevelRestored() const
    {
        return ui->restore_level_checkbox->isVisible() && ui->restore_level_checkbox->isChecked();
    }

    QString SentryReportDialog::GetUserComments() const
    {
        return ui->comments_edit->toPlainText();
    }

    QString SentryReportDialog::GetContactName() const
    {
        return ui->name_edit->text();
    }

    QString SentryReportDialog::GetUserEmail() const
    {
        return ui->email_edit->text();
    }
}
