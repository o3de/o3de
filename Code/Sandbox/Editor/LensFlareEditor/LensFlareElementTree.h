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

#ifndef CRYINCLUDE_EDITOR_LENSFLAREEDITOR_LENSFLAREELEMENTTREE_H
#define CRYINCLUDE_EDITOR_LENSFLAREEDITOR_LENSFLAREELEMENTTREE_H
#pragma once

#if !defined(Q_MOC_RUN)
#include "LensFlareElement.h"
#include "LensFlareUtil.h"
#include "ILensFlareListener.h"

#include <QAbstractItemModel>
#include <QTreeView>
#endif

class CLensFlareItem;
class LensFlareElementTreeModel;
class QMouseEvent;

class CLensFlareElementTree
    : public QTreeView
{
    Q_OBJECT

public:

    CLensFlareElementTree(QWidget* pParent = nullptr);

    void RegisterListener(ILensFlareChangeElementListener* pListener)
    {
        if (pListener)
        {
            m_LensFlaresElementListeners.push_back(pListener);
        }
    }

    void UnregisterListener(ILensFlareChangeElementListener* pListener)
    {
        if (pListener == NULL)
        {
            return;
        }
        std::vector<ILensFlareChangeElementListener*>::iterator ii  = m_LensFlaresElementListeners.begin();
        for (; ii != m_LensFlaresElementListeners.end(); )
        {
            if (*ii == pListener)
            {
                ii = m_LensFlaresElementListeners.erase(ii);
            }
            else
            {
                ++ii;
            }
        }
    }

    CLensFlareElement::LensFlareElementPtr FindLensFlareElement(IOpticsElementBasePtr pElement) const;

    void OnInternalVariableChange(IVariable* pVar);

    CLensFlareElement* GetCurrentLensFlareElement() const;
    void UpdateProperty();
    void UpdateClipboard(const char* type, bool bPasteAtSameLevel);

    void SelectTreeItemByName(const QString& name);

    CLensFlareItem* GetLensFlareItem() const;
    void InvalidateLensFlareItem();

protected:
    void CallChangeListeners();

    void OnDataChanged(const QModelIndex& index);
    void OnRowsInserted(const QModelIndex& index, int first, int last);
    void OnRowsRemoved(const QModelIndex& index, int first, int last);

    void OnNotifyTreeRClick();
    void OnTvnSelchangedTree(const QItemSelection& selected, const QItemSelection& deselected);

    XmlNodeRef CreateXML(const char* type) const;
    bool GetClipboardList(const char* type, std::vector<LensFlareUtil::SClipboardData>& outList) const;

    //! Only the basic operations like following methods must have routine for undoing.
    void ElementChanged(CLensFlareElement* pPrevLensFlareElement);
    /////////////////////////////////////////////////////////////////////////////////////////

    void OnAddGroup();
    void OnCopy();
    void OnCut();
    void OnPaste();
    void OnClone();
    void OnRenameItem();
    void OnRemoveItem();
    void OnRemoveAll();
    void OnItemUp();
    void OnItemDown();
    void mousePressEvent(QMouseEvent* event);
    void keyPressEvent(QKeyEvent* event);

    void Cut(bool bPasteAtSameLevel);

private:
    void SelectItem(const QModelIndex& index);

    std::vector<ILensFlareChangeElementListener*> m_LensFlaresElementListeners;

    QScopedPointer<LensFlareElementTreeModel> m_model;
};

class LensFlareElementTreeModel
    : public QAbstractItemModel
    , public ILensFlareChangeItemListener
{
    Q_OBJECT

public:
    LensFlareElementTreeModel(QObject* pParent = nullptr);
    ~LensFlareElementTreeModel();

    int rowCount(const QModelIndex& parent = {}) const override;
    int columnCount(const QModelIndex& parent = {}) const override;

    Qt::ItemFlags flags(const QModelIndex& index) const override;
    QVariant data(const QModelIndex& index, int role) const override;
    bool setData(const QModelIndex& index, const QVariant& value, int role = Qt::EditRole);

    QModelIndex index(int row, int column, const QModelIndex& parent = {}) const override;
    QModelIndex parent(const QModelIndex& index) const override;

    bool removeRows(int row, int count, const QModelIndex& parent = {}) override;

    QStringList mimeTypes() const override;
    QMimeData* mimeData(const QModelIndexList& indexes) const override;
    bool dropMimeData(const QMimeData* data, Qt::DropAction action, int row, int column, const QModelIndex& parent) override;
    Qt::DropActions supportedDragActions() const override;
    Qt::DropActions supportedDropActions() const override;

    CLensFlareElement* GetLensFlareElement(const QModelIndex& index) const;

    QModelIndex GetTreeItemByName(const QString& name) const;

    void Paste(const QModelIndex& index, XmlNodeRef xmlNode);
    void Cut(const QModelIndex& index, bool bPasteAtSameLevel);

    CLensFlareItem* GetLensFlareItem() const
    {
        return m_pLensFlareItem;
    }

    void InvalidateLensFlareItem();

    void RemoveAllElements();
    bool AddElement(const QModelIndex& index, int row, EFlareType flareType);

    bool MoveElement(CLensFlareElement* pElement, int row, const QModelIndex& parentIndex);

private:
    QModelIndex GetRootItem() const;
    IOpticsElementBasePtr GetOpticsElementByTreeItem(const QModelIndex& index) const;

    void OnLensFlareChangeItem(CLensFlareItem* pLensFlareItem);
    void OnLensFlareDeleteItem(CLensFlareItem* pLensFlareItem);

    void UpdateLensFlareItem(CLensFlareItem* pLensFlareItem);
    CLensFlareElement* UpdateLensFlareElementsRecusively(IOpticsElementBasePtr pOptics);
    CLensFlareElement* CreateElement(IOpticsElementBasePtr pOptics);
    void Reload();

    void MakeValidElementName(const QString& seedName, QString& outValidName) const;
    bool IsExistElement(const QString& name) const;
    CLensFlareElement::LensFlareElementPtr InsertAtomicElement(int nIndex, EFlareType flareType, CLensFlareElement* pParentElement);
    CLensFlareElement::LensFlareElementPtr AddElement(IOpticsElementBasePtr pOptics, CLensFlareElement* pParentElement);
    CLensFlareElement* FindLensFlareElement(IOpticsElementBasePtr pOptics) const;

    QModelIndex GetTreeItemByOpticsElement(const IOpticsElementBasePtr pOptics) const;
    void EnableElement(CLensFlareElement* pLensFlareElement, bool bEnable);
    void RenameElement(CLensFlareElement* pLensFlareElement, const QString& newName);
    void StoreUndo(const QString& undoDescription = "");

    CLensFlareElement::LensFlareElementPtr m_pRootElement;
    CLensFlareItem* m_pLensFlareItem;
};

Q_DECLARE_METATYPE(CLensFlareElement*)

#endif // CRYINCLUDE_EDITOR_LENSFLAREEDITOR_LENSFLAREELEMENTTREE_H
