/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include "EditorDefs.h"

#include "BaseLibraryItem.h"
#include "BaseLibrary.h"
#include "BaseLibraryManager.h"
#include "Undo/IUndoObject.h"

#include <AzCore/Math/Uuid.h>

//undo object for multi-changes inside library item. such as set all variables to default values.
//For example: change particle emitter shape will lead to multiple variable changes
class CUndoBaseLibraryItem
    : public IUndoObject
{
public:
    CUndoBaseLibraryItem(IBaseLibraryManager *libMgr, CBaseLibraryItem* libItem, bool ignoreChild)
        : m_libMgr(libMgr)
    {
        assert(libItem);
        assert(libMgr);

        m_itemPath = libItem->GetFullName();

        //serialize the lib item to undo
        m_undoCtx.node = GetIEditor()->GetSystem()->CreateXmlNode("Undo");
        m_undoCtx.bIgnoreChilds = ignoreChild;
        m_undoCtx.bLoading = false; //saving
        m_undoCtx.bUniqName = false; //don't generate new name
        m_undoCtx.bCopyPaste = true; //so it won't override guid
        m_undoCtx.bUndo = true;
        libItem->Serialize(m_undoCtx);

        //evaluate size
        XmlString xmlStr = m_undoCtx.node->getXML();
        m_size = sizeof(CUndoBaseLibraryItem);
        m_size += static_cast<int>(xmlStr.GetAllocatedMemory());
        m_size += m_itemPath.length();
    }


protected:
    int GetSize() override
    {
        return m_size;
    }

    void Undo(bool bUndo) override
    {
        //find the libItem
        IDataBaseItem *libItem = m_libMgr->FindItemByName(m_itemPath);
        if (libItem == nullptr)
        {
            //the undo stack is not reliable any more..
            assert(false);
            return;
        }

        //save for redo
        if (bUndo)
        {
            m_redoCtx.node = GetIEditor()->GetSystem()->CreateXmlNode("Redo");
            m_redoCtx.bIgnoreChilds = m_undoCtx.bIgnoreChilds;
            m_redoCtx.bLoading = false; //saving
            m_redoCtx.bUniqName = false;
            m_redoCtx.bCopyPaste = true;
            m_redoCtx.bUndo = true;
            libItem->Serialize(m_redoCtx);

            XmlString xmlStr = m_redoCtx.node->getXML();
            m_size += static_cast<int>(xmlStr.GetAllocatedMemory());
        }

        //load previous saved data
        m_undoCtx.bLoading = true;
        libItem->Serialize(m_undoCtx);
    }

    void Redo() override
    {
        //find the libItem
        IDataBaseItem *libItem = m_libMgr->FindItemByName(m_itemPath);
        if (libItem == nullptr || m_redoCtx.node == nullptr)
        {
            //the undo stack is not reliable any more..
            assert(false);
            return;
        }

        m_redoCtx.bLoading = true;
        libItem->Serialize(m_redoCtx);
    }

private:
    QString m_itemPath;
    IDataBaseItem::SerializeContext m_undoCtx; //saved before operation
    IDataBaseItem::SerializeContext m_redoCtx; //saved after operation so used for redo
    IBaseLibraryManager* m_libMgr;
    int m_size;
};

//////////////////////////////////////////////////////////////////////////
// CBaseLibraryItem implementation.
//////////////////////////////////////////////////////////////////////////
CBaseLibraryItem::CBaseLibraryItem()
{
    m_library = nullptr;
    GenerateId();
    m_bModified = false;
}

CBaseLibraryItem::~CBaseLibraryItem()
{
}

//////////////////////////////////////////////////////////////////////////
QString CBaseLibraryItem::GetFullName() const
{
    QString name;
    if (m_library)
    {
        name = m_library->GetName() + ".";
    }
    name += m_name;
    return name;
}

//////////////////////////////////////////////////////////////////////////
QString CBaseLibraryItem::GetGroupName()
{
    QString str = GetName();
    int p = str.lastIndexOf('.');
    if (p >= 0)
    {
        return str.mid(0, p);
    }
    return "";
}

//////////////////////////////////////////////////////////////////////////
QString CBaseLibraryItem::GetShortName()
{
    QString str = GetName();
    int p = str.lastIndexOf('.');
    if (p >= 0)
    {
        return str.mid(p + 1);
    }
    p = str.lastIndexOf('/');
    if (p >= 0)
    {
        return str.mid(p + 1);
    }
    return str;
}

//////////////////////////////////////////////////////////////////////////
void CBaseLibraryItem::SetName(const QString& name)
{
    assert(m_library);
    if (name == m_name)
    {
        return;
    }
    QString oldName = GetFullName();
    m_name = name;
    ((CBaseLibraryManager*)m_library->GetManager())->OnRenameItem(this, oldName);
}

//////////////////////////////////////////////////////////////////////////
const QString& CBaseLibraryItem::GetName() const
{
    return m_name;
}

//////////////////////////////////////////////////////////////////////////
void CBaseLibraryItem::GenerateId()
{
    GUID guid = AZ::Uuid::CreateRandom();
    SetGUID(guid);
}

//////////////////////////////////////////////////////////////////////////
void CBaseLibraryItem::SetGUID(REFGUID guid)
{
    if (m_library)
    {
        ((CBaseLibraryManager*)m_library->GetManager())->RegisterItem(this, guid);
    }
    m_guid = guid;
}

//////////////////////////////////////////////////////////////////////////
void CBaseLibraryItem::Serialize(SerializeContext& ctx)
{
    assert(m_library);

    XmlNodeRef node = ctx.node;
    if (ctx.bLoading)
    {
        QString name = m_name;
        // Loading
        node->getAttr("Name", name);

        if (!ctx.bUniqName)
        {
            SetName(name);
        }
        else
        {
            SetName(GetLibrary()->GetManager()->MakeUniqueItemName(name));
        }

        if (!ctx.bCopyPaste)
        {
            GUID guid;
            if (node->getAttr("Id", guid))
            {
                SetGUID(guid);
            }
        }
    }
    else
    {
        // Saving.
        node->setAttr("Name", m_name.toUtf8().data());
        node->setAttr("Id", m_guid);
        node->setAttr("Library", GetLibrary()->GetName().toUtf8().data());
    }
    m_bModified = false;
}

//////////////////////////////////////////////////////////////////////////
IDataBaseLibrary* CBaseLibraryItem::GetLibrary() const
{
    return m_library;
}

//////////////////////////////////////////////////////////////////////////
void CBaseLibraryItem::SetLibrary(CBaseLibrary* pLibrary)
{
    m_library = pLibrary;
}

//! Mark library as modified.
void CBaseLibraryItem::SetModified(bool bModified)
{
    m_bModified = bModified;
    if (m_bModified && m_library != nullptr)
    {
        m_library->SetModified(bModified);
    }
}
