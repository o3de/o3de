/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "WidgetItem.h"
#include <QIcon>
#include <QLineEdit>
#include <QLabel>
#include <QListWidget>

WidgetItem::WidgetItem(QWidget* parent)
    : QWidget(parent)
{
    ui.setupUi(this);
    ui.verticalLayout->setContentsMargins(0, 0, 0, 0);
    ui.horizontalLayout->setContentsMargins(0, 0, 0, 0);
    setWindowFlags(Qt::FramelessWindowHint);
    ui.label->setStyleSheet("QLineEdit{background:rgb(68,68,68);border-width:0;border-style:outset;color:white;}");
    ui.label->setEnabled(false);
    connect(ui.label, &QLineEdit::editingFinished, [this]()
    {
        if (!m_focus)
        {
            return;
        }
        Q_EMIT editingFinished(ui.label, m_listItem);
        ui.label->setCursorPosition(0);
    });
    m_lineEdit = ui.label;
    m_focus = false;
    ui.label->installEventFilter(this);
}

void WidgetItem::OnSelected() const
{
    ui.icon->setStyleSheet("border:2px solid yellow; border-radius:2px");
}

void WidgetItem::OnRelease() const
{
    ui.icon->setStyleSheet("border-width:0");
}

void WidgetItem::Init(QString text, QString iconPath, QListWidgetItem *listItem)
{
    m_listItem = listItem;
    ui.label->setText(text);
    ui.label->setCursorPosition(0);
    ui.icon->setPixmap(QPixmap(iconPath).scaled(65, 65, Qt::IgnoreAspectRatio));
}

bool WidgetItem::eventFilter(QObject* obj, QEvent* event)
{
    if (obj == m_lineEdit)
    {
        if (event->type() == QEvent::FocusIn && m_focus)
        {
            ui.label->setStyleSheet("QLineEdit{color:rgb(68,68,68);}");
        }
        else if (event->type() == QEvent::FocusOut)
        {
            ui.label->setStyleSheet("QLineEdit{background:rgb(68,68,68);border-width:0;border-style:outset;color:white;}");
            ui.label->setCursorPosition(0);
        }
    }
    return QWidget::eventFilter(obj, event);
}

void WidgetItem::SetFocusFlag(bool flag)
{
    m_focus = flag;
}

PortalItemDelegate::PortalItemDelegate(QWidget* parent)
    : QStyledItemDelegate(parent)
{
}

PortalItemDelegate::~PortalItemDelegate()
{
}

void PortalItemDelegate::updateEditorGeometry(QWidget* editor, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    editor->setGeometry(option.rect);
    index.data(Qt::DisplayRole);
}
