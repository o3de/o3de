/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#pragma once

#include <ACETypes.h>
#include <ATLControlsModel.h>

#include <QTreeWidget>
#include <QTreeView>
#include <QDropEvent>
#include <QStandardItem>
#include <QMimeData>
#include <QSortFilterProxyModel>

namespace AudioControls
{
    class CATLControl;
    class CATLControlsModel;
}

//-----------------------------------------------------------------------------------------------//
class QFolderItem
    : public QStandardItem
{
public:
    explicit QFolderItem(const QString& sName);
};

//-----------------------------------------------------------------------------------------------//
class QAudioControlItem
    : public QStandardItem
{
public:
    QAudioControlItem(const QString& sName, AudioControls::CATLControl* pControl);
};

//-----------------------------------------------------------------------------------------------//
class QAudioControlSortProxy
    : public QSortFilterProxyModel
{
public:
    explicit QAudioControlSortProxy(QObject* pParent = 0);
    bool setData(const QModelIndex& index, const QVariant& value, int role /* = Qt::EditRole */) override;

protected:
    bool lessThan(const QModelIndex& left, const QModelIndex& right) const override;
};

//-----------------------------------------------------------------------------------------------//
class QAudioControlsTreeView
    : public QTreeView
{
public:
    explicit QAudioControlsTreeView(QWidget* pParent = 0);
    void scrollTo(const QModelIndex& index, ScrollHint hint = QAbstractItemView::EnsureVisible) override;
    bool IsEditing();
};
