/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <LinkWidget.h>
#include <QDesktopServices>
#include <QEvent>
#include <QMouseEvent>
#include <QVBoxLayout>

namespace O3DE::ProjectManager
{
    LinkLabel::LinkLabel(const QString& text, const QUrl& url, QWidget* parent)
        : QLabel(text, parent)
        , m_url(url)
    {
        SetDefaultStyle();
    }

    void LinkLabel::mousePressEvent([[maybe_unused]] QMouseEvent* event)
    {
        if (m_url.isValid())
        {
            QDesktopServices::openUrl(m_url);
        }

        emit clicked();
    }

    void LinkLabel::enterEvent([[maybe_unused]] QEvent* event)
    {
        setStyleSheet("font-size: 10px; color: #94D2FF; text-decoration: underline;");
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
        setStyleSheet("font-size: 10px; color: #94D2FF;");
    }
} // namespace O3DE::ProjectManager
