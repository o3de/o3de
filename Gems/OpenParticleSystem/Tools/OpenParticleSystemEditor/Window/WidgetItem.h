/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once
#include <QWidget>
#include <QStyledItemDelegate>
#include "ui_WidgetItem.h"

class QListWidgetItem;
class WidgetItem : public QWidget
{
    Q_OBJECT
public:
    WidgetItem(QWidget* parent = Q_NULLPTR);
    ~WidgetItem() = default;

Q_SIGNALS:
    void editingFinished(QLineEdit *lineEdit, QListWidgetItem *listItem);

protected:
    bool eventFilter(QObject* obj, QEvent* e);

public:
    void OnSelected() const;
    void OnRelease() const;
    void Init(QString text, QString iconPath, QListWidgetItem* listItem);
    void SetFocusFlag(bool flag);
    QLineEdit* m_lineEdit;

private:
    Ui::WidgetItem ui;
    QListWidgetItem* m_listItem = nullptr;
    bool m_focus;
};

class PortalItemDelegate : public QStyledItemDelegate
{
    Q_OBJECT
public:
    PortalItemDelegate(QWidget* parent = nullptr);
    virtual ~PortalItemDelegate();
    void updateEditorGeometry(QWidget* editor, const QStyleOptionViewItem& option, const QModelIndex& index) const;
};

