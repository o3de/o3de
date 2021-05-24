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
        setStyleSheet("font-size: 9pt; color: #94D2FF; text-decoration: underline;");
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
        setStyleSheet("font-size: 9pt; color: #94D2FF;");
    }
} // namespace O3DE::ProjectManager
