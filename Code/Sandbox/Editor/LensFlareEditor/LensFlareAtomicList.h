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

#ifndef CRYINCLUDE_EDITOR_LENSFLAREEDITOR_LENSFLAREATOMICLIST_H
#define CRYINCLUDE_EDITOR_LENSFLAREEDITOR_LENSFLAREATOMICLIST_H
#pragma once

#if !defined(Q_MOC_RUN)
#include "Controls/ImageListCtrl.h"
#endif

#if !defined(Q_MOC_RUN)
#include <QAbstractItemModel>
#include <QScopedPointer>
#endif

class CLensFlareEditor;
class QLensFlareAtomicListModel;

class CLensFlareAtomicList
    : public CImageListCtrl
{
    Q_OBJECT
public:
    CLensFlareAtomicList(QWidget* parent = nullptr);
    virtual ~CLensFlareAtomicList();

    void FillAtomicItems();

protected:
    void updateGeometries() override;

private:
    QScopedPointer<QLensFlareAtomicListModel> m_model;
};

class QLensFlareAtomicListModel
    : public QAbstractListModel
{
    struct Item;
    Q_OBJECT
public:
    QLensFlareAtomicListModel(QObject* parent = nullptr);
    ~QLensFlareAtomicListModel();

    void Clear();
    void Populate();

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    bool setData(const QModelIndex& index, const QVariant& value, int role = Qt::EditRole) override;
    Qt::ItemFlags flags(const QModelIndex& index) const override;

    QStringList mimeTypes() const override;
    QMimeData* mimeData(const QModelIndexList& indexes) const override;
    bool dropMimeData(const QMimeData* data, Qt::DropAction action, int row, int column, const QModelIndex& parent) override;
    Qt::DropActions supportedDragActions() const override;

    EFlareType FlareTypeFromIndex(QModelIndex index) const;

protected:
    QModelIndex InsertItem(const FlareInfo& flareInfo);
    Item* ItemFromIndex(QModelIndex index) const;

private:
    QVector<Item*> m_items;
};
#endif // CRYINCLUDE_EDITOR_LENSFLAREEDITOR_LENSFLAREATOMICLIST_H
