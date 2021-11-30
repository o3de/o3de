/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <LinkWidget.h>
#include <ExternalLinkDialog.h>
#include <ProjectManagerSettings.h>

#include <AzCore/Settings/SettingsRegistry.h>

#include <QDesktopServices>
#include <QEvent>
#include <QMouseEvent>
#include <QVBoxLayout>

namespace O3DE::ProjectManager
{
    LinkLabel::LinkLabel(const QString& text, const QUrl& url, int fontSize, QWidget* parent)
        : QLabel(text, parent)
        , m_url(url)
        , m_fontSize(fontSize)
    {
        SetDefaultStyle();
    }

    void LinkLabel::mousePressEvent([[maybe_unused]] QMouseEvent* event)
    {
        if (m_url.isValid())
        {
            // Check if user request not to be shown external link warning dialog
            bool skipDialog = false;
            auto settingsRegistry = AZ::SettingsRegistry::Get();

            if (settingsRegistry)
            {
                QString settingsKey = GetExternalLinkWarningKey();
                settingsRegistry->Get(skipDialog, settingsKey.toStdString().c_str());
            }

            if (!skipDialog)
            {
                // Style does not apply if LinkLabel is parent so use parentWidget as parent instead
                ExternalLinkDialog* linkDialog = new ExternalLinkDialog(m_url.toString(), parentWidget());
                if (linkDialog->exec() == QDialog::Accepted)
                {
                    QDesktopServices::openUrl(m_url);
                }
            }
            else
            {
                QDesktopServices::openUrl(m_url);
            }
        }

        emit clicked();
    }

    void LinkLabel::enterEvent([[maybe_unused]] QEvent* event)
    {
        setStyleSheet(QString("font-size: %1px; color: #94D2FF; text-decoration: underline;").arg(m_fontSize));
    }

    void LinkLabel::leaveEvent([[maybe_unused]] QEvent* event)
    {
        SetDefaultStyle();
    }

    void LinkLabel::SetUrl(const QUrl& url)
    {
        m_url = url;
    }

    void LinkLabel::SetDefaultStyle()
    {
        setStyleSheet(QString("font-size: %1px; color: #94D2FF;").arg(m_fontSize));
    }
} // namespace O3DE::ProjectManager
