/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#pragma once

#include <QDialog>

namespace WhiteBox
{
    //! Shared floating tool behavior. All styling comes from the editor's Qt theme.
    class WhiteBoxModelingWindow : public QDialog
    {
    public:
        WhiteBoxModelingWindow(const QString& title, QWidget* parent);
        void ShowNearCursor();
        //! Dismiss without invoking an operation's Cancel action when editing context changes.
        void Dismiss();

    protected:
        void SetDragHandle(QWidget* widget);
        void mousePressEvent(QMouseEvent* event) override;
        void mouseMoveEvent(QMouseEvent* event) override;
        void mouseReleaseEvent(QMouseEvent* event) override;

    private:
        QWidget* m_dragHandle = nullptr; //!< Owned by this dialog.
        bool m_dragging = false;
        QPoint m_dragOffset;
    };
}
