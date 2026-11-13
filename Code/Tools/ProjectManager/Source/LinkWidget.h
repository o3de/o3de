/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzQtComponents/Components/Widgets/ElidingLabel.h>

#include <QLabel>
#include <QUrl>

QT_FORWARD_DECLARE_CLASS(QEvent)
QT_FORWARD_DECLARE_CLASS(QMouseEvent)
QT_FORWARD_DECLARE_CLASS(QWidget)

namespace O3DE::ProjectManager
{
    class LinkLabel
        : public AzQtComponents::ElidingLabel
    {
        Q_OBJECT

    public:
        LinkLabel(const QString& text = {}, const QUrl& url = {}, int fontSize = 10, QWidget* parent = nullptr);

        QUrl GetUrl() const;
        void SetUrl(const QUrl& url);

        //! The font size in pixels the link was made with (its sheet in the legacy GUI): the flattened GUI's text elements read it
        int GetFontSize() const;

        //! The dynamic property that marks a link under the pointer in the flattened GUI, where the legacy GUI sets the sheet of the
        //! hover state (`text-decoration: underline`): the hover text elements of FlatElements.cpp match on it
        static constexpr const char* HoverProperty = "linkHover";

    signals:
        void clicked();

    private:
        void mousePressEvent(QMouseEvent* event) override;
        void enterEvent(QEnterEvent* event) override;
        void leaveEvent(QEvent* event) override;
        void SetDefaultStyle();
        void SetHoverState(bool hovered);

    private:
        QUrl m_url;
        int m_fontSize;
    };
} // namespace O3DE::ProjectManager
