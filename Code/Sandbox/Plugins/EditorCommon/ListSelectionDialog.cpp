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
// Original file Copyright Crytek GMBH or its affiliates, used under license.

#include "EditorCommon_precompiled.h"
#include "Serialization/ClassFactory.h"
#include "Serialization/Pointers.h"
#include "Serialization/IArchive.h"
#include "ListSelectionDialog.h"
#include <IEditor.h>
#include <QDialogButtonBox>
#include <QBoxLayout>
#include <QTreeView>
#include <QStandardItemModel>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QIcon>
#include <QMenu>
#include <QEvent>
#include <QKeyEvent>
#include <QCoreApplication>
#include <QByteArray>
#include "IResourceSelectorHost.h"

#include "DeepFilterProxyModel.h"

// ---------------------------------------------------------------------------

ListSelectionDialog::ListSelectionDialog(QWidget* parent)
    : QDialog(parent)
    , m_currentColumn(0)
{
    setWindowTitle("Choose...");
    setWindowModality(Qt::ApplicationModal);

    QBoxLayout* layout = new QBoxLayout(QBoxLayout::TopToBottom);
    setLayout(layout);

    QBoxLayout* filterBox = new QBoxLayout(QBoxLayout::LeftToRight);
    layout->addLayout(filterBox);
    {
        filterBox->addWidget(new QLabel("Filter:", this), 0);
        filterBox->addWidget(m_filterEdit = new QLineEdit(this), 1);
        connect(m_filterEdit, SIGNAL(textChanged(const QString&)), this, SLOT(onFilterChanged(const QString&)));
        m_filterEdit->installEventFilter(this);
    }

    QBoxLayout* infoBox = new QBoxLayout(QBoxLayout::LeftToRight);
    layout->addLayout(infoBox);

    m_model = new QStandardItemModel();
    m_model->setColumnCount(1);
    m_model->setHeaderData(0, Qt::Horizontal, "Name", Qt::DisplayRole);

    m_filterModel = new DeepFilterProxyModel(this);
    m_filterModel->setSourceModel(m_model);
    m_filterModel->setDynamicSortFilter(true);

    m_tree = new QTreeView(this);
    //m_tree->setColumnCount(3);
    m_tree->setModel(m_filterModel);

    m_tree->header()->setStretchLastSection(false);
#if QT_VERSION >= 0x50000
    m_tree->header()->setSectionResizeMode(0, QHeaderView::Stretch);
#else
    m_tree->header()->setResizeMode(0, QHeaderView::Stretch);
#endif
    //m_tree->header()->resizeSection(0, 80);
    connect(m_tree, SIGNAL(activated(const QModelIndex&)), this, SLOT(onActivated(const QModelIndex&)));

    layout->addWidget(m_tree, 1);

    QDialogButtonBox* buttons = new QDialogButtonBox(this);
    buttons->setStandardButtons(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(buttons, SIGNAL(accepted()), this, SLOT(accept()));
    connect(buttons, SIGNAL(rejected()), this, SLOT(reject()));
    layout->addWidget(buttons, 0);
}

bool ListSelectionDialog::eventFilter(QObject* obj, QEvent* event)
{
    if (obj == m_filterEdit && event->type() == QEvent::KeyPress)
    {
        QKeyEvent* keyEvent = (QKeyEvent*)event;
        if (keyEvent->key() == Qt::Key_Down ||
            keyEvent->key() == Qt::Key_Up ||
            keyEvent->key() == Qt::Key_PageDown ||
            keyEvent->key() == Qt::Key_PageUp)
        {
            QCoreApplication::sendEvent(m_tree, event);
            return true;
        }
    }
    return QDialog::eventFilter(obj, event);
}

void ListSelectionDialog::onFilterChanged(const QString& str)
{
    m_filterModel->setFilterString(str);
    m_filterModel->invalidate();
    m_tree->expandAll();

    QModelIndex currentSource = m_filterModel->mapToSource(m_tree->selectionModel()->currentIndex());
    if (!currentSource.isValid() || !m_filterModel->matchFilter(currentSource.row(), currentSource.parent()))
    {
        QModelIndex firstMatchingIndex = m_filterModel->findFirstMatchingIndex(QModelIndex());
        if (firstMatchingIndex.isValid())
        {
            m_tree->selectionModel()->setCurrentIndex(firstMatchingIndex, QItemSelectionModel::SelectCurrent);
        }
    }
}

void ListSelectionDialog::onActivated(const QModelIndex& index)
{
    m_tree->setCurrentIndex(index);
    accept();
}

QSize ListSelectionDialog::sizeHint() const
{
    return QSize(600, 900);
}

void ListSelectionDialog::SetColumnText(int column, const char* text)
{
    if (column >= m_model->columnCount())
    {
        int oldColumnCount = m_model->columnCount();
        m_model->setColumnCount(column + 1);
        for (int i = oldColumnCount; i <= column; ++i)
        {
#if QT_VERSION >= 0x50000
            m_tree->header()->setSectionResizeMode(i, QHeaderView::Interactive);
    #else
            m_tree->header()->setResizeMode(i, QHeaderView::Interactive);
    #endif
            m_tree->header()->resizeSection(i, 40);
        }
    }
    m_model->setHeaderData(column, Qt::Horizontal, text, Qt::DisplayRole);
}

void ListSelectionDialog::SetColumnWidth(int column, int width)
{
    if (column >= m_model->columnCount())
    {
        return;
    }
    m_tree->header()->resizeSection(column, width);
}


void ListSelectionDialog::AddRow(const char* name)
{
    AddRow(name, QIcon());
}

void ListSelectionDialog::AddRow(const char* name, const QIcon& icon)
{
    QStandardItem* item = new QStandardItem(name);
    item->setEditable(false);
    item->setData(name);
    item->setIcon(icon);

    QList<QStandardItem*> items;
    items.append(item);

    m_model->appendRow(items);
    m_currentColumn = 1;
    m_firstColumnToItem[name] = item;
}

void ListSelectionDialog::AddRowColumn(const char* text)
{
    int itemCount = m_model->rowCount(QModelIndex());
    if (itemCount == 0)
    {
        return;
    }

    QStandardItem* item = new QStandardItem();
    item->setText(QString::fromLocal8Bit(text));
    if (QStandardItem* lastItem = m_model->item(itemCount - 1, 0))
    {
        item->setData(lastItem->data());
    }
    item->setEditable(false);
    m_model->setItem(itemCount - 1, m_currentColumn, item);
    ++m_currentColumn;
}

QString ListSelectionDialog::ChooseItem(const QString& currentValue)
{
    m_tree->expandAll();

    if (exec() == QDialog::Accepted && m_tree->selectionModel()->currentIndex().isValid())
    {
        QModelIndex currentIndex = m_tree->selectionModel()->currentIndex();
        QModelIndex sourceCurrentIndex = m_filterModel->mapToSource(currentIndex);
        QStandardItem* item = m_model->itemFromIndex(sourceCurrentIndex);
        if (item)
        {
            m_chosenItem = item->data().toString().toUtf8();
            return m_chosenItem.constData();
        }
    }
    return currentValue;
}

// ---------------------------------------------------------------------------

#include <moc_ListSelectionDialog.cpp>
