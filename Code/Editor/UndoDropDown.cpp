/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include "EditorDefs.h"

#include "UndoDropDown.h"

#include <vector>               // for std::vector
#include <algorithm>            // for std::reverse

// Qt
#include <QVBoxLayout>
#include <QListView>
#include <QAbstractListModel>
#include <QPushButton>

// Editor
#include "Undo/Undo.h"          // for CUndoManager



/////////////////////////////////////////////////////////////////////////////
// Turns listener callbacks into signals

UndoStackStateAdapter::UndoStackStateAdapter(QObject* parent /* = nullptr */)
    : QObject(parent)
{
    GetIEditor()->GetUndoManager()->AddListener(this);
}

UndoStackStateAdapter::~UndoStackStateAdapter()
{
    GetIEditor()->GetUndoManager()->RemoveListener(this);
}

/////////////////////////////////////////////////////////////////////////////
// The model that holds the list of undo/redo actions

class UndoDropDownListModel
    : public QAbstractListModel
    , IUndoManagerListener
{
public:
    UndoDropDownListModel(CUndoManager& manager, UndoRedoDirection direction, QObject* parent = nullptr)
        : QAbstractListModel(parent)
        , m_manager(manager)
        , m_direction(direction)
    {
        m_stackNames = m_direction == UndoRedoDirection::Undo ? m_manager.GetUndoStackNames() : m_manager.GetRedoStackNames();
        m_manager.AddListener(this);
    }

    ~UndoDropDownListModel() override
    {
        m_manager.RemoveListener(this);
    }

    int rowCount(const QModelIndex& parent = QModelIndex()) const override
    {
        if (parent.model() != this && parent != QModelIndex())
        {
            return 0;
        }

        return static_cast<int>(m_stackNames.size());
    }

    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override
    {
        if (index.row() < 0 || index.row() >= rowCount() || role != Qt::DisplayRole)
        {
            return {};
        }

        return m_stackNames[index.row()];
    }

    void SignalNumUndoRedo(const unsigned int& numUndo, const unsigned int& numRedo) override
    {
        std::vector<QString> fresh;
        if (UndoRedoDirection::Undo == m_direction && m_stackNames.size() != numUndo)
        {
            fresh = m_manager.GetUndoStackNames();
        }
        else if (UndoRedoDirection::Redo == m_direction && m_stackNames.size() != numRedo)
        {
            fresh = m_manager.GetRedoStackNames();
        }
        else
        {
            return;
        }

        std::reverse(fresh.begin(), fresh.end());

        if (fresh.size() < m_stackNames.size())
        {
            beginRemoveRows(QModelIndex(), static_cast<int>(fresh.size()), static_cast<int>(m_stackNames.size() - 1));
            m_stackNames = fresh;
            endRemoveRows();
        }
        else
        {
            beginInsertRows(QModelIndex(), static_cast<int>(m_stackNames.size()), static_cast<int>(fresh.size() - 1));
            m_stackNames = fresh;
            endInsertRows();
        }
    }

private:
    CUndoManager& m_manager;
    UndoRedoDirection m_direction;
    std::vector<QString> m_stackNames;
};

/////////////////////////////////////////////////////////////////////////////
// Custom selection model we use for the list of undo/redo actions

void UndoStackItemSelectionModel::select(const QModelIndex& index, QItemSelectionModel::SelectionFlags command)
{
    if (!model())
    {
        return;
    }

    if (index.isValid())
    {
        QItemSelectionModel::select({ model()->index(0, 0, {}), index }, command);
    }
    clearCurrentIndex();
}

void UndoStackItemSelectionModel::select([[maybe_unused]] const QItemSelection& selection, QItemSelectionModel::SelectionFlags command)
{
    QPoint mouse = m_view->mapFromGlobal(QCursor::pos());
    QModelIndex first = model()->index(0, 0, {});
    QModelIndex last = model()->index(model()->rowCount() - 1, 0, {});
    QModelIndex underMouse = m_view->indexAt(mouse);

    if (underMouse.isValid())
    {
        last = underMouse;
    }

    QItemSelectionModel::select({ first, last }, command);
    clearCurrentIndex();
}

/////////////////////////////////////////////////////////////////////////////
// CUndoDropDown dialog

CUndoDropDown::CUndoDropDown(UndoRedoDirection direction, QWidget* pParent /* = nullptr */)
    : QDialog(pParent)
    , m_direction(direction)
{
    auto layout = new QVBoxLayout(this);
    setLayout(layout);

    // Model & view
    m_model = new UndoDropDownListModel(*GetIEditor()->GetUndoManager(), direction, this);
    m_view = new QListView(this);
    m_view->setModel(m_model);
    m_view->setSelectionModel(new UndoStackItemSelectionModel(m_view, m_model));
    m_view->setSelectionMode(QAbstractItemView::ContiguousSelection);
    layout->addWidget(m_view);

    // The buttons along the bottom of the dropdown
    auto buttonBox = new QHBoxLayout();

    m_doButton = new QPushButton(this);
    buttonBox->addWidget(m_doButton);

    buttonBox->addStretch(1);

    auto clearButton = new QPushButton(this);
    clearButton->setText(tr("Clear"));

    buttonBox->addWidget(clearButton);

    layout->addLayout(buttonBox);

    // Connections
    connect(m_doButton, &QPushButton::clicked, this, &CUndoDropDown::OnUndoButton);
    connect(clearButton, &QPushButton::clicked, this, &CUndoDropDown::OnUndoClear);

    connect(m_view->selectionModel(), &QItemSelectionModel::selectionChanged, this, &CUndoDropDown::SelectionChanged);

    setMinimumWidth(450);
}

void CUndoDropDown::Prepare()
{
    m_view->selectionModel()->select(m_model->index(0, 0, {}), QItemSelectionModel::ClearAndSelect);
    m_view->setFocus();
    show();
}

void CUndoDropDown::OnUndoButton()
{
    int numSelected = m_view->selectionModel()->selectedIndexes().size();
    if (m_direction == UndoRedoDirection::Undo)
    {
        GetIEditor()->GetUndoManager()->Undo(numSelected);
    }
    else
    {
        GetIEditor()->GetUndoManager()->Redo(numSelected);
    }
    accept();
}

void CUndoDropDown::OnUndoClear()
{
    if (m_direction == UndoRedoDirection::Undo)
    {
        GetIEditor()->GetUndoManager()->ClearUndoStack();
    }
    else
    {
        GetIEditor()->GetUndoManager()->ClearRedoStack();
    }
    accept();
}

void CUndoDropDown::SelectionChanged([[maybe_unused]] const QItemSelection& selected, [[maybe_unused]] const QItemSelection& deselected)
{
    QString label = QString("%1 %2 action%3")
            .arg(m_direction == UndoRedoDirection::Undo ? tr("Undo") : tr("Redo"))
            .arg(m_view->selectionModel()->selectedIndexes().size())
            .arg(m_view->selectionModel()->selectedIndexes().size() > 1 ? "s" : "");
    m_doButton->setText(label);
}

void CUndoDropDown::contextMenuEvent(QContextMenuEvent*)
{
    // Inhibit QDialog::contextMenuEvent() as this will trigger the "What's this" popup.
    // That happens because we're a child of a QMenu, and menus have Qt::WA_CustomWhatsThis
    // which make the popup show even if QWidget::whatsThis() text is empty.
}

#include <moc_UndoDropDown.cpp>
