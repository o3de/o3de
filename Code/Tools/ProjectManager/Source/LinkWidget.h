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

#pragma once

#if !defined(Q_MOC_RUN)
#include <QLabel>
#include <QUrl>
#endif

QT_FORWARD_DECLARE_CLASS(QEvent)
QT_FORWARD_DECLARE_CLASS(QMouseEvent)
QT_FORWARD_DECLARE_CLASS(QWidget)

namespace O3DE::ProjectManager
{
    class LinkLabel
        : public QLabel
    {
        Q_OBJECT // AUTOMOC

    public:
        LinkLabel(const QString& text = {}, const QUrl& url = {}, QWidget* parent = nullptr);

        void SetUrl(const QUrl& url);

    signals:
        void clicked();

    private:
        void mousePressEvent(QMouseEvent* event) override;
        void enterEvent(QEvent* event) override;
        void leaveEvent(QEvent* event) override;
        void SetDefaultStyle();

    private:
        QUrl m_url;
    };
} // namespace O3DE::ProjectManager
