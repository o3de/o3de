/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */



#include "EditorDefs.h"

#include "BaseLibraryManager.h"

// Editor
#include "BaseLibraryItem.h"
#include "ErrorReport.h"
#include "Undo/IUndoObject.h"

//////////////////////////////////////////////////////////////////////////
// CBaseLibraryManager implementation.
//////////////////////////////////////////////////////////////////////////
CBaseLibraryManager::CBaseLibraryManager()
{
    m_bUniqNameMap = false;
    m_bUniqGuidMap = true;
    GetIEditor()->RegisterNotifyListener(this);
}

//////////////////////////////////////////////////////////////////////////
CBaseLibraryManager::~CBaseLibraryManager()
{
    ClearAll();
    GetIEditor()->UnregisterNotifyListener(this);
}

//////////////////////////////////////////////////////////////////////////
void CBaseLibraryManager::ClearAll()
{
    // Delete all items from all libraries.
    for (int i = 0; i < m_libs.size(); i++)
    {
        m_libs[i]->RemoveAllItems();
    }

    // if we will not copy maps locally then destructors of the elements of
    // the map will operate on the already invalid map object
    // see:
    //  CBaseLibraryManager::UnregisterItem()
    //  CBaseLibraryManager::DeleteItem()
    //  CMaterial::~CMaterial()

    ItemsGUIDMap itemsGuidMap;
    ItemsNameMap itemsNameMap;

    {
        AZStd::lock_guard<AZStd::mutex> lock(m_itemsNameMapMutex);
        std::swap(itemsGuidMap, m_itemsGuidMap);
        std::swap(itemsNameMap, m_itemsNameMap);

        m_libs.clear();
    }
}

//////////////////////////////////////////////////////////////////////////
IDataBaseLibrary* CBaseLibraryManager::FindLibrary(const QString& library)
{
    const int index = FindLibraryIndex(library);
    return index == -1 ? nullptr : m_libs[index];
}

//////////////////////////////////////////////////////////////////////////
int CBaseLibraryManager::FindLibraryIndex(const QString& library)
{
    QString lib = library;
    lib.replace('\\', '/');
    for (int i = 0; i < m_libs.size(); i++)
    {
        QString _lib = m_libs[i]->GetFilename();
        _lib.replace('\\', '/');
        if (QString::compare(lib, m_libs[i]->GetName(), Qt::CaseInsensitive) == 0 || QString::compare(lib, _lib, Qt::CaseInsensitive) == 0)
        {
            return i;
        }
    }
    return -1;
}

//////////////////////////////////////////////////////////////////////////
IDataBaseItem* CBaseLibraryManager::FindItem(REFGUID guid) const
{
    CBaseLibraryItem* pMtl = stl::find_in_map(m_itemsGuidMap, guid, nullptr);
    return pMtl;
}

//////////////////////////////////////////////////////////////////////////
void CBaseLibraryManager::SplitFullItemName(const QString& fullItemName, QString& libraryName, QString& itemName)
{
    int p;
    p = fullItemName.indexOf('.');
    if (p < 0 || !QString::compare(fullItemName.mid(p + 1), "mtl", Qt::CaseInsensitive))
    {
        libraryName = "";
        itemName = fullItemName;
        return;
    }
    libraryName = fullItemName.mid(0, p);
    itemName = fullItemName.mid(p + 1);
}

//////////////////////////////////////////////////////////////////////////
IDataBaseItem* CBaseLibraryManager::FindItemByName(const QString& fullItemName)
{
    AZStd::lock_guard<AZStd::mutex> lock(m_itemsNameMapMutex);
    return stl::find_in_map(m_itemsNameMap, fullItemName, nullptr);
}

//////////////////////////////////////////////////////////////////////////
IDataBaseItem* CBaseLibraryManager::LoadItemByName(const QString& fullItemName)
{
    QString libraryName, itemName;
    SplitFullItemName(fullItemName, libraryName, itemName);

    if (!FindLibrary(libraryName))
    {
        LoadLibrary(MakeFilename(libraryName));
    }

    return FindItemByName(fullItemName);
}

//////////////////////////////////////////////////////////////////////////
IDataBaseItem* CBaseLibraryManager::FindItemByName(const char* fullItemName)
{
    return FindItemByName(QString(fullItemName));
}

//////////////////////////////////////////////////////////////////////////
IDataBaseItem* CBaseLibraryManager::LoadItemByName(const char* fullItemName)
{
    return LoadItemByName(QString(fullItemName));
}

//////////////////////////////////////////////////////////////////////////
IDataBaseItem* CBaseLibraryManager::CreateItem(IDataBaseLibrary* pLibrary)
{
    assert(pLibrary);

    // Add item to this library.
    TSmartPtr<CBaseLibraryItem> pItem = MakeNewItem();
    pLibrary->AddItem(pItem);
    return pItem;
}

//////////////////////////////////////////////////////////////////////////
void CBaseLibraryManager::DeleteItem(IDataBaseItem* pItem)
{
    assert(pItem);

    UnregisterItem((CBaseLibraryItem*)pItem);
    if (pItem->GetLibrary())
    {
        pItem->GetLibrary()->RemoveItem(pItem);
    }
}

//////////////////////////////////////////////////////////////////////////
IDataBaseLibrary* CBaseLibraryManager::LoadLibrary(const QString& inFilename, [[maybe_unused]] bool bReload)
{
    if (auto lib = FindLibrary(inFilename))
    {
        return lib;
    }

    TSmartPtr<CBaseLibrary> pLib = MakeNewLibrary();
    if (!pLib->Load(MakeFilename(inFilename)))
    {
        Error(QObject::tr("Failed to Load Item Library: %1").arg(inFilename).toUtf8().data());
        return nullptr;
    }

    m_libs.push_back(pLib);
    return pLib;
}

//////////////////////////////////////////////////////////////////////////
int CBaseLibraryManager::GetModifiedLibraryCount() const
{
    int count = 0;
    for (int i = 0; i < m_libs.size(); i++)
    {
        if (m_libs[i]->IsModified())
        {
            count++;
        }
    }
    return count;
}

//////////////////////////////////////////////////////////////////////////
IDataBaseLibrary* CBaseLibraryManager::AddLibrary(const QString& library, bool bIsLevelLibrary, bool bIsLoading)
{
    // Make a filename from name of library.
    QString filename = library;

    if (filename.indexOf(".xml") == -1) // if its already a filename, we don't do anything
    {
        filename.replace(' ', '_');
        if (!bIsLevelLibrary)
        {
            filename = MakeFilename(library);
        }
        else
        {
            // if its the level library it gets saved in the level and should not be concatenated with any other file name
            filename = filename + ".xml";
        }
    }

    IDataBaseLibrary* pBaseLib = FindLibrary(library);  //library name
    if (!pBaseLib)
    {
        pBaseLib = FindLibrary(filename); //library file name
    }
    if (pBaseLib)
    {
        return pBaseLib;
    }

    CBaseLibrary* lib = MakeNewLibrary();
    lib->SetName(library);
    lib->SetLevelLibrary(bIsLevelLibrary);
    lib->SetFilename(filename, !bIsLoading);
    // set modified to true, so even empty particle libraries get saved
    lib->SetModified(true);

    m_libs.push_back(lib);
    return lib;
}

//////////////////////////////////////////////////////////////////////////
QString CBaseLibraryManager::MakeFilename(const QString& library)
{
    QString filename = library;
    filename.replace(' ', '_');
    filename.replace(".xml", "");

    // make it contain the canonical libs path:
    Path::ConvertBackSlashToSlash(filename);

    QString LibsPath(GetLibsPath());
    Path::ConvertBackSlashToSlash(LibsPath);

    if (filename.left(LibsPath.length()).compare(LibsPath, Qt::CaseInsensitive) == 0)
    {
        filename = filename.mid(LibsPath.length());
    }

    return LibsPath + filename + ".xml";
}

//////////////////////////////////////////////////////////////////////////
bool CBaseLibraryManager::IsUniqueFilename(const QString& library)
{
    QString resultPath = MakeFilename(library);
    CCryFile xmlFile;
    // If we can find a file for the path
    return !xmlFile.Open(resultPath.toUtf8().data(), "rb");
}

//////////////////////////////////////////////////////////////////////////
void CBaseLibraryManager::DeleteLibrary(const QString& library, bool forceDeleteLevel)
{
    for (int i = 0; i < m_libs.size(); i++)
    {
        if (QString::compare(library, m_libs[i]->GetName(), Qt::CaseInsensitive) == 0)
        {
            CBaseLibrary* pLibrary = m_libs[i];
            // Check if not level library, they cannot be deleted.
            if (!pLibrary->IsLevelLibrary() || forceDeleteLevel)
            {
                for (int j = 0; j < pLibrary->GetItemCount(); j++)
                {
                    UnregisterItem((CBaseLibraryItem*)pLibrary->GetItem(j));
                }
                pLibrary->RemoveAllItems();

                if (pLibrary->IsLevelLibrary())
                {
                    m_pLevelLibrary = nullptr;
                }
                m_libs.erase(m_libs.begin() + i);
            }
            break;
        }
    }
}

//////////////////////////////////////////////////////////////////////////
IDataBaseLibrary* CBaseLibraryManager::GetLibrary(int index) const
{
    assert(index >= 0 && index < m_libs.size());
    return m_libs[index];
};

//////////////////////////////////////////////////////////////////////////
IDataBaseLibrary* CBaseLibraryManager::GetLevelLibrary() const
{
    IDataBaseLibrary* pLevelLib = nullptr;

    for (int i = 0; i < GetLibraryCount(); i++)
    {
        if (GetLibrary(i)->IsLevelLibrary())
        {
            pLevelLib = GetLibrary(i);
            break;
        }
    }


    return pLevelLib;
}

//////////////////////////////////////////////////////////////////////////
void CBaseLibraryManager::SaveAllLibs()
{
    for (int i = 0; i < GetLibraryCount(); i++)
    {
        // Check if library is modified.
        IDataBaseLibrary* pLibrary = GetLibrary(i);

        //Level library is saved when the level is saved
        if (pLibrary->IsLevelLibrary())
        {
            continue;
        }
        if (pLibrary->IsModified())
        {
            if (pLibrary->Save())
            {
                pLibrary->SetModified(false);
            }
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CBaseLibraryManager::Serialize(XmlNodeRef& node, bool bLoading)
{
    static const char* const LEVEL_LIBRARY_TAG = "LevelLibrary";

    QString rootNodeName = GetRootNodeName();
    if (bLoading)
    {
        XmlNodeRef libs = node->findChild(rootNodeName.toUtf8().data());
        if (libs)
        {
            for (int i = 0; i < libs->getChildCount(); i++)
            {
                // Load only library name.
                XmlNodeRef libNode = libs->getChild(i);
                if (strcmp(libNode->getTag(), LEVEL_LIBRARY_TAG) == 0)
                {
                    if (!m_pLevelLibrary)
                    {
                        QString libName;
                        libNode->getAttr("Name", libName);
                        m_pLevelLibrary = static_cast<CBaseLibrary*>(AddLibrary(libName, true));
                    }
                    m_pLevelLibrary->Serialize(libNode, bLoading);
                }
                else
                {
                    QString libName;
                    if (libNode->getAttr("Name", libName))
                    {
                        // Load this library.
                        if (!FindLibrary(libName))
                        {
                            LoadLibrary(MakeFilename(libName));
                        }
                    }
                }
            }
        }
    }
    else
    {
        // Save all libraries.
        XmlNodeRef libs = node->newChild(rootNodeName.toUtf8().data());
        for (int i = 0; i < GetLibraryCount(); i++)
        {
            IDataBaseLibrary* pLib = GetLibrary(i);
            if (pLib->IsLevelLibrary())
            {
                // Level libraries are saved in in level.
                XmlNodeRef libNode = libs->newChild(LEVEL_LIBRARY_TAG);
                pLib->Serialize(libNode, bLoading);
            }
            else
            {
                // Save only library name.
                XmlNodeRef libNode = libs->newChild("Library");
                libNode->setAttr("Name", pLib->GetName().toUtf8().data());
            }
        }
        SaveAllLibs();
    }
}

//////////////////////////////////////////////////////////////////////////
QString CBaseLibraryManager::MakeUniqueItemName(const QString& srcName, const QString& libName)
{
    // unlikely we'll ever encounter more than 16
    std::vector<AZStd::string> possibleDuplicates;
    possibleDuplicates.reserve(16);

    // search for strings in the database that might have a similar name (ignore case)
    IDataBaseItemEnumerator* pEnum = GetItemEnumerator();
    for (IDataBaseItem* pItem = pEnum->GetFirst(); pItem != nullptr; pItem = pEnum->GetNext())
    {
        //Check if the item is in the target library first.
        IDataBaseLibrary* itemLibrary = pItem->GetLibrary();
        QString itemLibraryName;
        if (itemLibrary)
        {
            itemLibraryName = itemLibrary->GetName();
        }

        // Item is not in the library so there cannot be a naming conflict.
        if (!libName.isEmpty() && !itemLibraryName.isEmpty() && itemLibraryName != libName)
        {
            continue;
        }

        const QString& name = pItem->GetName();
        if (name.startsWith(srcName, Qt::CaseInsensitive))
        {
            possibleDuplicates.push_back(AZStd::string(name.toUtf8().data()));
        }
    }
    pEnum->Release();

    if (possibleDuplicates.empty())
    {
        return srcName;
    }

    std::sort(possibleDuplicates.begin(), possibleDuplicates.end(), [](const AZStd::string& strOne, const AZStd::string& strTwo)
        {
            // I can assume size sorting since if the length is different, either one of the two strings doesn't
            // closely match the string we are trying to duplicate, or it's a bigger number (X1 vs X10)
            if (strOne.size() != strTwo.size())
            {
                return strOne.size() < strTwo.size();
            }
            else
            {
                return azstricmp(strOne.c_str(), strTwo.c_str()) < 0;
            }
        }
        );

    int num = 0;
    QString returnValue = srcName;
    while (num < possibleDuplicates.size() && QString::compare(possibleDuplicates[num].c_str(), returnValue, Qt::CaseInsensitive) == 0)
    {
        returnValue = QStringLiteral("%1%2%3").arg(srcName).arg("_").arg(num);
        ++num;
    }

    return returnValue;
}

//////////////////////////////////////////////////////////////////////////
void CBaseLibraryManager::Validate()
{
    IDataBaseItemEnumerator* pEnum = GetItemEnumerator();
    for (IDataBaseItem* pItem = pEnum->GetFirst(); pItem != nullptr; pItem = pEnum->GetNext())
    {
        pItem->Validate();
    }
    pEnum->Release();
}

//////////////////////////////////////////////////////////////////////////
void CBaseLibraryManager::RegisterItem(CBaseLibraryItem* pItem, REFGUID newGuid)
{
    assert(pItem);

    bool bNotify = false;

    if (m_bUniqGuidMap)
    {
        REFGUID oldGuid = pItem->GetGUID();
        if (!GuidUtil::IsEmpty(oldGuid))
        {
            m_itemsGuidMap.erase(oldGuid);
        }
        if (GuidUtil::IsEmpty(newGuid))
        {
            return;
        }
        CBaseLibraryItem* pOldItem = stl::find_in_map(m_itemsGuidMap, newGuid, nullptr);
        if (!pOldItem)
        {
            pItem->m_guid = newGuid;
            m_itemsGuidMap[newGuid] = pItem;
            pItem->m_bRegistered = true;
            bNotify = true;
        }
        else
        {
            if (pOldItem != pItem)
            {
                ReportDuplicateItem(pItem, pOldItem);
            }
        }
    }

    if (m_bUniqNameMap)
    {
        QString fullName = pItem->GetFullName();
        if (!pItem->GetName().isEmpty())
        {
            CBaseLibraryItem* pOldItem = static_cast<CBaseLibraryItem*>(FindItemByName(fullName));
            if (!pOldItem)
            {
                AZStd::lock_guard<AZStd::mutex> lock(m_itemsNameMapMutex);
                m_itemsNameMap[fullName] = pItem;
                pItem->m_bRegistered = true;
                bNotify = true;
            }
            else
            {
                if (pOldItem != pItem)
                {
                    ReportDuplicateItem(pItem, pOldItem);
                }
            }
        }
    }

    // Notify listeners.
    if (bNotify)
    {
        NotifyItemEvent(pItem, EDB_ITEM_EVENT_ADD);
    }
}

//////////////////////////////////////////////////////////////////////////
void CBaseLibraryManager::RegisterItem(CBaseLibraryItem* pItem)
{
    assert(pItem);

    bool bNotify = false;

    if (m_bUniqGuidMap)
    {
        if (GuidUtil::IsEmpty(pItem->GetGUID()))
        {
            return;
        }
        CBaseLibraryItem* pOldItem = stl::find_in_map(m_itemsGuidMap, pItem->GetGUID(), nullptr);
        if (!pOldItem)
        {
            m_itemsGuidMap[pItem->GetGUID()] = pItem;
            pItem->m_bRegistered = true;
            bNotify = true;
        }
        else
        {
            if (pOldItem != pItem)
            {
                ReportDuplicateItem(pItem, pOldItem);
            }
        }
    }

    if (m_bUniqNameMap)
    {
        QString fullName = pItem->GetFullName();
        if (!fullName.isEmpty())
        {
            CBaseLibraryItem* pOldItem = static_cast<CBaseLibraryItem*>(FindItemByName(fullName));
            if (!pOldItem)
            {
                AZStd::lock_guard<AZStd::mutex> lock(m_itemsNameMapMutex);
                m_itemsNameMap[fullName] = pItem;
                pItem->m_bRegistered = true;
                bNotify = true;
            }
            else
            {
                if (pOldItem != pItem)
                {
                    ReportDuplicateItem(pItem, pOldItem);
                }
            }
        }
    }

    // Notify listeners.
    if (bNotify)
    {
        NotifyItemEvent(pItem, EDB_ITEM_EVENT_ADD);
    }
}

//////////////////////////////////////////////////////////////////////////
void CBaseLibraryManager::SetRegisteredFlag(CBaseLibraryItem* pItem, bool bFlag)
{
    pItem->m_bRegistered = bFlag;
}

//////////////////////////////////////////////////////////////////////////
void CBaseLibraryManager::ReportDuplicateItem(CBaseLibraryItem* pItem, CBaseLibraryItem* pOldItem)
{
    QString sLibName;
    if (pOldItem->GetLibrary())
    {
        sLibName = pOldItem->GetLibrary()->GetName();
    }
    CErrorRecord err;
    err.pItem = pItem;
    err.error = QStringLiteral("Item %1 with duplicate GUID to loaded item %2 ignored").arg(pItem->GetFullName(), pOldItem->GetFullName());
    GetIEditor()->GetErrorReport()->ReportError(err);
}

//////////////////////////////////////////////////////////////////////////
void CBaseLibraryManager::UnregisterItem(CBaseLibraryItem* pItem)
{
    // Notify listeners.
    NotifyItemEvent(pItem, EDB_ITEM_EVENT_DELETE);

    if (!pItem)
    {
        return;
    }

    if (m_bUniqGuidMap)
    {
        m_itemsGuidMap.erase(pItem->GetGUID());
    }
    if (m_bUniqNameMap && !pItem->GetFullName().isEmpty())
    {
        AZStd::lock_guard<AZStd::mutex> lock(m_itemsNameMapMutex);
        auto findIter = m_itemsNameMap.find(pItem->GetFullName());
        if (findIter != m_itemsNameMap.end())
        {
            _smart_ptr<CBaseLibraryItem> item = findIter->second;
            m_itemsNameMap.erase(findIter);
        }
    }

    pItem->m_bRegistered = false;
}

//////////////////////////////////////////////////////////////////////////
QString CBaseLibraryManager::MakeFullItemName(IDataBaseLibrary* pLibrary, const QString& group, const QString& itemName)
{
    assert(pLibrary);
    QString name = pLibrary->GetName() + ".";
    if (!group.isEmpty())
    {
        name += group + ".";
    }
    name += itemName;
    return name;
}

//////////////////////////////////////////////////////////////////////////
void CBaseLibraryManager::GatherUsedResources(CUsedResources& resources)
{
    IDataBaseItemEnumerator* pEnum = GetItemEnumerator();
    for (IDataBaseItem* pItem = pEnum->GetFirst(); pItem != nullptr; pItem = pEnum->GetNext())
    {
        pItem->GatherUsedResources(resources);
    }
    pEnum->Release();
}

//////////////////////////////////////////////////////////////////////////
IDataBaseItemEnumerator* CBaseLibraryManager::GetItemEnumerator()
{
    if (m_bUniqNameMap)
    {
        return new CDataBaseItemEnumerator<ItemsNameMap>(&m_itemsNameMap);
    }
    else
    {
        return new CDataBaseItemEnumerator<ItemsGUIDMap>(&m_itemsGuidMap);
    }
}

//////////////////////////////////////////////////////////////////////////
void CBaseLibraryManager::OnEditorNotifyEvent(EEditorNotifyEvent event)
{
    switch (event)
    {
    case eNotify_OnBeginNewScene:
        SetSelectedItem(nullptr);
        ClearAll();
        break;
    case eNotify_OnBeginSceneOpen:
        SetSelectedItem(nullptr);
        ClearAll();
        break;
    case eNotify_OnCloseScene:
        SetSelectedItem(nullptr);
        ClearAll();
        break;
    }
}

//////////////////////////////////////////////////////////////////////////
void CBaseLibraryManager::OnRenameItem(CBaseLibraryItem* pItem, const QString& oldName)
{
    m_itemsNameMapMutex.lock();
    if (!oldName.isEmpty())
    {
        m_itemsNameMap.erase(oldName);
    }
    if (!pItem->GetFullName().isEmpty())
    {
        m_itemsNameMap[pItem->GetFullName()] = pItem;
    }
    m_itemsNameMapMutex.unlock();

    OnItemChanged(pItem);
}

//////////////////////////////////////////////////////////////////////////
void CBaseLibraryManager::AddListener(IDataBaseManagerListener* pListener)
{
    stl::push_back_unique(m_listeners, pListener);
}

//////////////////////////////////////////////////////////////////////////
void CBaseLibraryManager::RemoveListener(IDataBaseManagerListener* pListener)
{
    stl::find_and_erase(m_listeners, pListener);
}

//////////////////////////////////////////////////////////////////////////
void CBaseLibraryManager::NotifyItemEvent(IDataBaseItem* pItem, EDataBaseItemEvent event)
{
    // Notify listeners.
    if (!m_listeners.empty())
    {
        for (int i = 0; i < m_listeners.size(); i++)
        {
            m_listeners[i]->OnDataBaseItemEvent(pItem, event);
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CBaseLibraryManager::OnItemChanged(IDataBaseItem* pItem)
{
    NotifyItemEvent(pItem, EDB_ITEM_EVENT_CHANGED);
}

//////////////////////////////////////////////////////////////////////////
void CBaseLibraryManager::OnUpdateProperties(IDataBaseItem* pItem, bool bRefresh)
{
    NotifyItemEvent(pItem, bRefresh ? EDB_ITEM_EVENT_UPDATE_PROPERTIES
        : EDB_ITEM_EVENT_UPDATE_PROPERTIES_NO_EDITOR_REFRESH);
}

//////////////////////////////////////////////////////////////////////////
void CBaseLibraryManager::SetSelectedItem(IDataBaseItem* pItem)
{
    if (m_pSelectedItem == pItem)
    {
        return;
    }
    m_pSelectedItem = (CBaseLibraryItem*)pItem;
    NotifyItemEvent(m_pSelectedItem, EDB_ITEM_EVENT_SELECTED);
}

//////////////////////////////////////////////////////////////////////////
IDataBaseItem* CBaseLibraryManager::GetSelectedItem() const
{
    return m_pSelectedItem;
}

//////////////////////////////////////////////////////////////////////////
IDataBaseItem* CBaseLibraryManager::GetSelectedParentItem() const
{
    return m_pSelectedParent;
}

void CBaseLibraryManager::ChangeLibraryOrder(IDataBaseLibrary* lib, unsigned int newLocation)
{
    if (!lib || newLocation >= m_libs.size() || lib == m_libs[newLocation])
    {
        return;
    }

    for (int i = 0; i < m_libs.size(); i++)
    {
        if (lib == m_libs[i])
        {
            _smart_ptr<CBaseLibrary> curLib = m_libs[i];
            m_libs.erase(m_libs.begin() + i);
            m_libs.insert(m_libs.begin() + newLocation, curLib);
            return;
        }
    }
}

bool CBaseLibraryManager::SetLibraryName(CBaseLibrary* lib, const QString& name)
{
    // SetFilename will validate if the name is duplicate with exist libraries.
    if (lib->SetFilename(MakeFilename(name)))
    {
        lib->SetName(name);
        return true;
    }
    return false;
}
