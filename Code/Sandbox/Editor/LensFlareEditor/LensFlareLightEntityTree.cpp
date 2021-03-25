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

#include "EditorDefs.h"

#include "LensFlareLightEntityTree.h"

// Editor
#include "Viewport.h"
#include "Include/IObjectManager.h"
#include "Objects/SelectionGroup.h"
#include "LensFlareItem.h"
#include "LensFlareEditor.h"


CLensFlareLightEntityTree::CLensFlareLightEntityTree(QWidget* pParent)
    : QTreeView(pParent)
    , m_model(new LensFlareLightEntityModel)
{
    setHeaderHidden(true);

    setModel(m_model.data());

    connect(this, &QTreeView::doubleClicked, this, &CLensFlareLightEntityTree::OnTvnItemDoubleClicked);
}

CLensFlareLightEntityTree::~CLensFlareLightEntityTree()
{
}

void CLensFlareLightEntityTree::OnLensFlareChangeItem(CLensFlareItem* pLensFlareItem)
{
    m_model->OnLensFlareChangeItem(pLensFlareItem);
}

void CLensFlareLightEntityTree::OnLensFlareDeleteItem(CLensFlareItem* pLensFlareItem)
{
    m_model->OnLensFlareDeleteItem(pLensFlareItem);
}

void CLensFlareLightEntityTree::OnTvnItemDoubleClicked(const QModelIndex& index)
{
    CEntityObject* pObject = index.data(Qt::UserRole).value<CEntityObject*>();

    if (!qobject_cast<CEntityObject*>(pObject))
    {
        return;
    }

    CSelectionGroup* pSelection = GetIEditor()->GetObjectManager()->GetSelection();
    if (pSelection)
    {
        int iSelectionCount(pSelection->GetCount());
        if (iSelectionCount == 1)
        {
            if (pSelection->GetObject(0) == pObject)
            {
                CViewport* vp = GetIEditor()->GetActiveView();
                if (vp)
                {
                    vp->CenterOnSelection();
                }
                return;
            }
        }
    }

    GetIEditor()->GetObjectManager()->ClearSelection();
    GetIEditor()->GetObjectManager()->SelectObject(pObject);
}

LensFlareLightEntityModel::LensFlareLightEntityModel(QObject* pParent)
    : QAbstractListModel(pParent)
{
    CLensFlareEditor::GetLensFlareEditor()->RegisterLensFlareItemChangeListener(this);
    GetIEditor()->GetObjectManager()->AddObjectEventListener(this);
}

LensFlareLightEntityModel::~LensFlareLightEntityModel()
{
    CLensFlareEditor::GetLensFlareEditor()->UnregisterLensFlareItemChangeListener(this);
    GetIEditor()->GetObjectManager()->RemoveObjectEventListener(this);
}

void LensFlareLightEntityModel::OnLensFlareChangeItem(CLensFlareItem* pLensFlareItem)
{
    beginResetModel();

    m_pLensFlareItem = pLensFlareItem;
    m_lightEntities.clear();

    if (m_pLensFlareItem)
    {
        std::vector<CEntityObject*> lightEntities;
        LensFlareUtil::GetLightEntityObjects(lightEntities);

        for (int i = 0, iEntitySize(lightEntities.size()); i < iEntitySize; ++i)
        {
            AddLightEntity(lightEntities[i]);
        }
    }

    endResetModel();
}

void LensFlareLightEntityModel::OnLensFlareDeleteItem([[maybe_unused]] CLensFlareItem* pLensFlareItem)
{
    beginResetModel();

    m_lightEntities.clear();
    m_pLensFlareItem = NULL;

    endResetModel();
}

void LensFlareLightEntityModel::OnObjectEvent(CBaseObject* pObject, int nEvent)
{
    if (!qobject_cast<CEntityObject*>(pObject))
    {
        return;
    }

    switch (nEvent)
    {
    case CBaseObject::ON_RENAME:
    case CBaseObject::ON_DELETE:
    {
        auto it = std::find(std::begin(m_lightEntities), std::end(m_lightEntities), pObject);
        if (it == std::end(m_lightEntities))
        {
            return;
        }
        int row = std::distance(std::begin(m_lightEntities), it);
        if (nEvent == CBaseObject::ON_RENAME)
        {
            emit dataChanged(index(row, 0), index(row, 0));
        }
        else
        {
            beginRemoveRows({}, row, row);
            m_lightEntities.erase(it);
            endRemoveRows();
        }
    }
    break;

    case CBaseObject::ON_ADD:
        if (AddLightEntity((CEntityObject*)pObject))
        {
            int row = m_lightEntities.size() - 1;
            beginInsertRows({}, row, row);
            endInsertRows();
        }
        break;
    }
}

bool LensFlareLightEntityModel::AddLightEntity(CEntityObject* pEntity)
{
    if (pEntity == NULL || m_pLensFlareItem == NULL)
    {
        return false;
    }

    QString lensFlareName = pEntity->GetEntityPropertyString(CEntityObject::s_LensFlarePropertyName);
    if (lensFlareName.isEmpty())
    {
        return false;
    }

    QString fullName = m_pLensFlareItem->GetFullName();
    if (fullName.isEmpty())
    {
        return false;
    }

    if (QString::compare(lensFlareName, fullName, Qt::CaseInsensitive))
    {
        return false;
    }

    m_lightEntities.push_back(pEntity);

    return true;
}

int LensFlareLightEntityModel::rowCount(const QModelIndex&) const
{
    return m_lightEntities.size();
}

QVariant LensFlareLightEntityModel::data(const QModelIndex& index, int role) const
{
    const int row = index.row();

    if (row < 0 || row >= m_lightEntities.size())
    {
        return {};
    }

    auto pEntity = m_lightEntities[row];

    switch (role)
    {
    case Qt::DisplayRole:
        return pEntity->GetName();

    case Qt::UserRole:
        return QVariant::fromValue<CEntityObject*>(pEntity);
    }

    return {};
}

#include <LensFlareEditor/moc_LensFlareLightEntityTree.cpp>
