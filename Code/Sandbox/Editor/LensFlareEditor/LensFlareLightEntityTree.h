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

#ifndef CRYINCLUDE_EDITOR_LENSFLAREEDITOR_LENSFLARELIGHTENTITYTREE_H
#define CRYINCLUDE_EDITOR_LENSFLAREEDITOR_LENSFLARELIGHTENTITYTREE_H
#pragma once

#if !defined(Q_MOC_RUN)
#include "ILensFlareListener.h"

#include <QScopedPointer>
#include <QTreeWidget>
#include "Objects/EntityObject.h"
#endif

class LensFlareLightEntityModel;

class CLensFlareLightEntityTree
    : public QTreeView
{
    Q_OBJECT

public:
    CLensFlareLightEntityTree(QWidget* pParent = nullptr);
    ~CLensFlareLightEntityTree();

    void OnLensFlareDeleteItem(CLensFlareItem* pLensFlareItem);
    void OnLensFlareChangeItem(CLensFlareItem* pLensFlareItem);

protected:
    void OnTvnItemDoubleClicked(const QModelIndex& index);

    QScopedPointer<LensFlareLightEntityModel> m_model;
};

class LensFlareLightEntityModel
    : public QAbstractListModel
    , public ILensFlareChangeItemListener
{
    Q_OBJECT

public:
    LensFlareLightEntityModel(QObject* pParent = nullptr);
    ~LensFlareLightEntityModel();

    int rowCount(const QModelIndex& parent = {}) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;

    void OnLensFlareDeleteItem(CLensFlareItem* pLensFlareItem);
    void OnLensFlareChangeItem(CLensFlareItem* pLensFlareItem);

protected:
    void OnObjectEvent(CBaseObject* pObject, int nEvent);

    bool AddLightEntity(CEntityObject* pEntity);

    std::vector<CEntityObject*> m_lightEntities;
    _smart_ptr<CLensFlareItem> m_pLensFlareItem;
};

Q_DECLARE_METATYPE(CEntityObject*)

#endif // CRYINCLUDE_EDITOR_LENSFLAREEDITOR_LENSFLARELIGHTENTITYTREE_H
