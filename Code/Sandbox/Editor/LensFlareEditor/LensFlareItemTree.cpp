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

#include "LensFlareItemTree.h"

// Editor
#include "LensFlareItem.h"
#include "ViewManager.h"
#include "Objects/EntityObject.h"


CLensFlareItemTree::CLensFlareItemTree(QWidget* pParent)
    : QTreeView(pParent)
{
    setHeaderHidden(true);
    setSelectionMode(QAbstractItemView::SingleSelection);
    setContextMenuPolicy(Qt::CustomContextMenu);
    setDragEnabled(true);
    setAcceptDrops(true);
    setDropIndicatorShown(true);
    setDragDropMode(DragDrop);
}

CLensFlareItemTree::~CLensFlareItemTree()
{
}

void CLensFlareItemTree::startDrag(Qt::DropActions supportedDropActions)
{
    QTreeView::startDrag(supportedDropActions);
}

void CLensFlareItemTree::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton)
    {
        CUndo undo(tr("Changed lens flare item").toUtf8());
        QTreeView::mousePressEvent(event);
    }
    else
    {
        QTreeView::mousePressEvent(event);
    }
}

void CLensFlareItemTree::AssignLensFlareToLightEntity(CViewport* pViewport) const
{
    QModelIndexList selected = selectionModel()->selectedIndexes();
    if (selected.isEmpty())
    {
        return;
    }

    CLensFlareItem* pLensFlareItem = static_cast<CLensFlareItem*>(selected.first().data(Qt::UserRole).value<CBaseLibraryItem*>());

    QPoint viewportPos = QCursor::pos();
    pViewport->ScreenToClient(viewportPos);
    HitContext hit;
    if (!pViewport->HitTest(viewportPos, hit))
    {
        return;
    }

    if (hit.object && qobject_cast<CEntityObject*>(hit.object))
    {
        CEntityObject* pEntity = (CEntityObject*)hit.object;
        if (pEntity->IsLight())
        {
            CUndo undo(tr("Assign a lens flare item to a light entity").toUtf8());
            pEntity->ApplyOptics(pLensFlareItem->GetFullName(), pLensFlareItem->GetOptics());
        }
    }
}

#include <LensFlareEditor/moc_LensFlareItemTree.cpp>
