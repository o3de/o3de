/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <LinkWidget.h>
#include <ExternalLinkDialog.h>
#include <SettingsInterface.h>

#include <QDesktopServices>
#include <QEvent>
#include <QMouseEvent>
#include <QVBoxLayout>

namespace O3DE::ProjectManager
{
    LinkLabel::LinkLabel(const QString& text, const QUrl& url, int fontSize, QWidget* parent)
        : AzQtComponents::ElidingLabel(text, parent)
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
            SettingsInterface::Get()->Get(skipDialog, ISettings::ExternalLinkWarningKey);

            if (!skipDialog)
            {
                // Style does not apply if LinkLabel is parent so use parentWidget as parent instead
                ExternalLinkDialog* linkDialog = new ExternalLinkDialog(m_url, parentWidget());
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

    QUrl LinkLabel::GetUrl() const
    {
        return m_url;
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
