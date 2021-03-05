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
