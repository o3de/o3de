/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include "EditorDefs.h"

#include "BaseLibrary.h"
#include "BaseLibraryItem.h"
#include "Include/IBaseLibraryManager.h"
#include <Util/PathUtil.h>
#include <IFileUtil.h>

//////////////////////////////////////////////////////////////////////////
// CBaseLibrary implementation.
//////////////////////////////////////////////////////////////////////////
CBaseLibrary::CBaseLibrary(IBaseLibraryManager* pManager)
    : m_pManager(pManager)
    , m_bModified(false)
    , m_bLevelLib(false)
    , m_bNewLibrary(true)
{
}

//////////////////////////////////////////////////////////////////////////
CBaseLibrary::~CBaseLibrary()
{
    m_items.clear();
}

//////////////////////////////////////////////////////////////////////////
IBaseLibraryManager* CBaseLibrary::GetManager()
{
    return m_pManager;
}

//////////////////////////////////////////////////////////////////////////
void CBaseLibrary::RemoveAllItems()
{
    AddRef();
    for (int i = 0; i < m_items.size(); i++)
    {
        // Unregister item in case it was registered.  It is ok if it wasn't.  This is still safe to call.
        m_pManager->UnregisterItem(m_items[i]);
        // Clear library item.
        m_items[i]->m_library = nullptr;
    }
    m_items.clear();
    Release();
}

//////////////////////////////////////////////////////////////////////////
void CBaseLibrary::SetName(const QString& name)
{
    //the fullname of the items in the library will be changed due to library's name change
    //so we need unregistered them and register them after their name changed.
    for (int i = 0; i < m_items.size(); i++)
    {
        m_pManager->UnregisterItem(m_items[i]);
    }

    m_name = name;

    for (int i = 0; i < m_items.size(); i++)
    {
        m_pManager->RegisterItem(m_items[i]);
    }

    SetModified();
}

//////////////////////////////////////////////////////////////////////////
const QString& CBaseLibrary::GetName() const
{
    return m_name;
}

//////////////////////////////////////////////////////////////////////////
bool CBaseLibrary::Save()
{
    return true;
}

//////////////////////////////////////////////////////////////////////////
bool CBaseLibrary::Load(const QString& filename)
{
    m_filename = filename;
    SetModified(false);
    m_bNewLibrary = false;
    return true;
}

//////////////////////////////////////////////////////////////////////////
void CBaseLibrary::SetModified(bool bModified)
{
    if (bModified != m_bModified)
    {
        m_bModified = bModified;
        emit Modified(bModified);
    }
}

//////////////////////////////////////////////////////////////////////////
void CBaseLibrary::AddItem(IDataBaseItem* item, bool bRegister)
{

    CBaseLibraryItem* pLibItem = (CBaseLibraryItem*)item;
    // Check if item is already assigned to this library.
    if (pLibItem->m_library != this)
    {
        pLibItem->m_library = this;
        m_items.push_back(pLibItem);
        SetModified();
        if (bRegister)
        {
            m_pManager->RegisterItem(pLibItem);
        }
    }
}

//////////////////////////////////////////////////////////////////////////
IDataBaseItem* CBaseLibrary::GetItem(int index)
{
    assert(index >= 0 && index < m_items.size());
    return m_items[index];
}

//////////////////////////////////////////////////////////////////////////
void CBaseLibrary::RemoveItem(IDataBaseItem* item)
{

    for (int i = 0; i < m_items.size(); i++)
    {
        if (m_items[i] == item)
        {
            // Unregister item in case it was registered.  It is ok if it wasn't.  This is still safe to call.
            m_pManager->UnregisterItem(m_items[i]);
            m_items.erase(m_items.begin() + i);
            SetModified();
            break;
        }
    }
}

//////////////////////////////////////////////////////////////////////////
IDataBaseItem* CBaseLibrary::FindItem(const QString& name)
{
    for (int i = 0; i < m_items.size(); i++)
    {
        if (QString::compare(m_items[i]->GetName(), name, Qt::CaseInsensitive) == 0)
        {
            return m_items[i];
        }
    }
    return nullptr;
}

bool CBaseLibrary::AddLibraryToSourceControl(const QString& fullPathName) const
{
    IEditor* pEditor = GetIEditor();
    IFileUtil* pFileUtil = pEditor ? pEditor->GetFileUtil() : nullptr;
    if (pFileUtil)
    {
        return pFileUtil->CheckoutFile(fullPathName.toUtf8().data(), nullptr);
    }

    return false;
}

bool CBaseLibrary::SaveLibrary(const char* name, bool saveEmptyLibrary)
{
    assert(name != nullptr);
    if (name == nullptr)
    {
        CryFatalError("The library you are attempting to save has no name specified.");
        return false;
    }

    QString fileName(GetFilename());
    if (fileName.isEmpty() && !saveEmptyLibrary)
    {
        return false;
    }

    fileName = Path::GamePathToFullPath(fileName);

    XmlNodeRef root = GetIEditor()->GetSystem()->CreateXmlNode(name);
    Serialize(root, false);
    bool bRes = XmlHelpers::SaveXmlNode(GetIEditor()->GetFileUtil(), root, fileName.toUtf8().data());
    if (m_bNewLibrary)
    {
        AddLibraryToSourceControl(fileName);
        m_bNewLibrary = false;
    }
    if (!bRes)
    {
        QByteArray filenameUtf8 = fileName.toUtf8();
        AZStd::string strMessage = AZStd::string::format("The file %s is read-only and the save of the library couldn't be performed. Try to remove the \"read-only\" flag or check-out the file and then try again.", filenameUtf8.data());
        CryMessageBox(strMessage.c_str(), "Saving Error", MB_OK | MB_ICONWARNING);
    }
    return bRes;
}

//CONFETTI BEGIN
void CBaseLibrary::ChangeItemOrder(CBaseLibraryItem* item, unsigned int newLocation)
{
    std::vector<_smart_ptr<CBaseLibraryItem> > temp;
    for (unsigned int i = 0; i < m_items.size(); i++)
    {
        if (i == newLocation)
        {
            temp.push_back(_smart_ptr<CBaseLibraryItem>(item));
        }
        if (m_items[i] != item)
        {
            temp.push_back(m_items[i]);
        }
    }
    // If newLocation is greater than the original size, append the item to end of the list
    if (newLocation >= m_items.size())
    {
        temp.push_back(_smart_ptr<CBaseLibraryItem>(item));
    }
    m_items = temp;
}
//CONFETTI END

#include <moc_BaseLibrary.cpp>
