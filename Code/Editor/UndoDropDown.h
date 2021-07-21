/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#ifndef CRYINCLUDE_EDITOR_UNDODROPDOWN_H
#define CRYINCLUDE_EDITOR_UNDODROPDOWN_H

#pragma once
// UndoDropDown.h : header file
//

#if !defined(Q_MOC_RUN)
#include <QDialog>
#include <QItemSelectionModel>
#include "Undo/IUndoManagerListener.h"
#endif

class QAction;
class QListView;
class UndoDropDownListModel;
class QAbstractItemView;
class QContextMenuEvent;

enum class UndoRedoDirection
{
    Undo,
    Redo
};

/// Turns IUndoManagerListener callbacks into signals
class UndoStackStateAdapter
    : public QObject
    , IUndoManagerListener
{
    Q_OBJECT

public:
    explicit UndoStackStateAdapter(QObject* parent = nullptr);

    ~UndoStackStateAdapter();

    void SignalNumUndoRedo(const unsigned int& numUndo, const unsigned int& numRedo)
    {
        emit UndoAvailable(numUndo);
        emit RedoAvailable(numRedo);
    }

signals:
    void UndoAvailable(quint32 count);
    void RedoAvailable(quint32 count);
};

/// Enforces contiguous selections from the top element in the list view to any that is selected below
class UndoStackItemSelectionModel
    : public QItemSelectionModel
{
    Q_OBJECT
public:
    UndoStackItemSelectionModel(QAbstractItemView* view, QAbstractItemModel* model)
        : QItemSelectionModel(model)
        , m_view(view)
    {}

    virtual ~UndoStackItemSelectionModel() {}

public slots:
    void select(const QModelIndex& index, QItemSelectionModel::SelectionFlags command) override;
    void select(const QItemSelection& selection, QItemSelectionModel::SelectionFlags command) override;

private:
    QAbstractItemView* m_view;
};

/////////////////////////////////////////////////////////////////////////////
// CUndoDropDown dialog

class CUndoDropDown
    : public QDialog
{
    Q_OBJECT

    // Construction
public:
    CUndoDropDown(UndoRedoDirection direction, QWidget* pParent = nullptr);   // standard constructor

public slots:
    // Prepare to be shown in the popup/dropdown
    void Prepare();

    // Implementation
protected:
    void OnUndoButton();
    void OnUndoClear();
    void contextMenuEvent(QContextMenuEvent*) override;

    UndoRedoDirection m_direction;
    UndoDropDownListModel* m_model;
    QListView* m_view;
    QPushButton* m_doButton;

private slots:
    void SelectionChanged(const QItemSelection& selected, const QItemSelection& deselected);
};

#endif // CRYINCLUDE_EDITOR_UNDODROPDOWN_H
