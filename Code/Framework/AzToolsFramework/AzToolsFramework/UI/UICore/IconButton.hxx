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

#include <QPushButton>

namespace AzToolsFramework
{

    class IconButton
        : public QPushButton
    {
        Q_OBJECT // AUTOMOC
        Q_PROPERTY(QIcon highlightedIcon READ highlightedIcon WRITE setHighlightedIcon)
    public:

        IconButton(QWidget *EntityPropertyEditorUI)
            : QPushButton(EntityPropertyEditorUI)
            , m_currentIconCacheKey(0)
            , m_mouseOver(false)
        {
            setAttribute(Qt::WA_Hover, true);
        }

        QIcon highlightedIcon() const { return m_highlightedIcon; }
        void setHighlightedIcon(const QIcon& icon)
        {
            m_highlightedIcon = icon;
            update();
        }

    protected:

        void enterEvent(QEvent *event) override;
        void leaveEvent(QEvent *event) override;

        void paintEvent(QPaintEvent* event) override;

    private:

        qint64 m_currentIconCacheKey;
        QSize m_currentIconCacheKeySize;
        QPixmap m_iconPixmap;
        QPixmap m_highlightedIconPixmap;
        QIcon m_highlightedIcon;

        bool m_mouseOver;
    };
}