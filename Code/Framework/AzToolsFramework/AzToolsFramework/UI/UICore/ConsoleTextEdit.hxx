/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include <QPlainTextEdit>
#include <QPointer>
#include <QScopedPointer>

#endif

class QMenu;

namespace AzToolsFramework
{
    class ConsoleTextEdit : public QPlainTextEdit
    {
        Q_OBJECT
        Q_PROPERTY(bool searchEnabled READ searchEnabled WRITE setSearchEnabled)
    public:
        explicit ConsoleTextEdit(QWidget* parent = nullptr);
        ~ConsoleTextEdit();

        virtual bool event(QEvent* theEvent) override;

        bool searchEnabled();
        void setSearchEnabled(bool enabled);
    Q_SIGNALS:
        void searchBarRequested();

    private:
        void showContextMenu(const QPoint& pt);

        QScopedPointer<QMenu> m_contextMenu;
        QPointer<QAction> m_searchAction;
    };
} // namespace AzToolsFramework
