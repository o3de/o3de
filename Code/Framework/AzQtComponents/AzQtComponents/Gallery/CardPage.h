/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#if !defined(Q_MOC_RUN)
#include <QWidget>
#include <QScopedPointer>
#endif

class QAction;

namespace Ui {
    class CardPage;
}

class CardPage : public QWidget
{
    Q_OBJECT //AUTOMOC

public:
    explicit CardPage(QWidget* parent = nullptr);
    ~CardPage() override;

private:
    void addNotification();

    void toggleWarning(bool checked);
    void toggleContentChanged(bool checked);
    void toggleSelectedChanged(bool selected);
    void showContextMenu(const QPoint& point);

    QScopedPointer<Ui::CardPage> ui;

    QAction* m_addNotification = nullptr;
    QAction* m_clearNotification = nullptr;
    QAction* m_warningAction = nullptr;
    QAction* m_contentChangedAction = nullptr;
    QAction* m_selectedChangedAction = nullptr;
};


