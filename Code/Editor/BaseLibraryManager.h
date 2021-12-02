/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */



#ifndef CRYINCLUDE_EDITOR_BASELIBRARYMANAGER_H
#define CRYINCLUDE_EDITOR_BASELIBRARYMANAGER_H
#pragma once

#include "Include/IBaseLibraryManager.h"
#include "Include/IDataBaseItem.h"
#include "Include/IDataBaseLibrary.h"
#include "Include/IDataBaseManager.h"
#include "Util/TRefCountBase.h"
#include "Util/GuidUtil.h"
#include "BaseLibrary.h"
#include "Util/smartptr.h"
#include <EditorDefs.h>
#include <QtUtil.h>

AZ_PUSH_DISABLE_DLL_EXPORT_BASECLASS_WARNING
/** Manages all Libraries and Items.
*/
class SANDBOX_API  CBaseLibraryManager
    : public IBaseLibraryManager
{
AZ_POP_DISABLE_DLL_EXPORT_BASECLASS_WARNING
public:
    CBaseLibraryManager();
    ~CBaseLibraryManager();

    //! Clear all libraries.
    void ClearAll() override;

    //////////////////////////////////////////////////////////////////////////
    // IDocListener implementation.
    //////////////////////////////////////////////////////////////////////////
    void OnEditorNotifyEvent(EEditorNotifyEvent event) override;

    //////////////////////////////////////////////////////////////////////////
    // Library items.
    //////////////////////////////////////////////////////////////////////////
    //! Make a new item in specified library.
    IDataBaseItem* CreateItem(IDataBaseLibrary* pLibrary) override;
    //! Delete item from library and manager.
    void DeleteItem(IDataBaseItem* pItem) override;

    //! Find Item by its GUID.
    IDataBaseItem* FindItem(REFGUID guid) const override;
    IDataBaseItem* FindItemByName(const QString& fullItemName) override;
    IDataBaseItem* LoadItemByName(const QString& fullItemName) override;
    virtual IDataBaseItem* FindItemByName(const char* fullItemName);
    virtual IDataBaseItem* LoadItemByName(const char* fullItemName);

    IDataBaseItemEnumerator* GetItemEnumerator() override;

    //////////////////////////////////////////////////////////////////////////
    // Set item currently selected.
    void SetSelectedItem(IDataBaseItem* pItem) override;
    // Get currently selected item.
    IDataBaseItem* GetSelectedItem() const override;
    IDataBaseItem* GetSelectedParentItem() const override;

    //////////////////////////////////////////////////////////////////////////
    // Libraries.
    //////////////////////////////////////////////////////////////////////////
    //! Add Item library.
    IDataBaseLibrary* AddLibrary(const QString& library, bool bIsLevelLibrary = false, bool bIsLoading = true) override;
    void DeleteLibrary(const QString& library, bool forceDeleteLevel = false) override;
    //! Get number of libraries.
    int GetLibraryCount() const override { return static_cast<int>(m_libs.size()); };
    //! Get number of modified libraries.
    int GetModifiedLibraryCount() const override;

    //! Get Item library by index.
    IDataBaseLibrary* GetLibrary(int index) const override;

    //! Get Level Item library.
    IDataBaseLibrary* GetLevelLibrary() const override;

    //! Find Items Library by name.
    IDataBaseLibrary* FindLibrary(const QString& library) override;

    //! Find Items Library's index by name.
    int FindLibraryIndex(const QString& library) override;
    
    //! Load Items library.
    IDataBaseLibrary* LoadLibrary(const QString& filename, bool bReload = false) override;

    //! Save all modified libraries.
    void SaveAllLibs() override;

    //! Serialize property manager.
    void Serialize(XmlNodeRef& node, bool bLoading) override;

    //! Export items to game.
    void Export([[maybe_unused]] XmlNodeRef& node) override {};

    //! Returns unique name base on input name.
    QString MakeUniqueItemName(const QString& name, const QString& libName = "") override;
    QString MakeFullItemName(IDataBaseLibrary* pLibrary, const QString& group, const QString& itemName) override;

    //! Root node where this library will be saved.
    QString GetRootNodeName() override = 0;
    //! Path to libraries in this manager.
    QString GetLibsPath() override = 0;

    //////////////////////////////////////////////////////////////////////////
    //! Validate library items for errors.
    void Validate() override;

    //////////////////////////////////////////////////////////////////////////
    void GatherUsedResources(CUsedResources& resources) override;

    void AddListener(IDataBaseManagerListener* pListener) override;
    void RemoveListener(IDataBaseManagerListener* pListener) override;

    //////////////////////////////////////////////////////////////////////////
    void RegisterItem(CBaseLibraryItem* pItem, REFGUID newGuid) override;
    void RegisterItem(CBaseLibraryItem* pItem) override;
    void UnregisterItem(CBaseLibraryItem* pItem) override;

    // Only Used internally.
    void OnRenameItem(CBaseLibraryItem* pItem, const QString& oldName) override;

    // Called by items to indicated that they have been modified.
    // Sends item changed event to listeners.
    void OnItemChanged(IDataBaseItem* pItem) override;
    void OnUpdateProperties(IDataBaseItem* pItem, bool bRefresh) override;

    QString MakeFilename(const QString& library);
    bool IsUniqueFilename(const QString& library) override;

    //CONFETTI BEGIN
    // Used to change the library item order
    void ChangeLibraryOrder(IDataBaseLibrary* lib, unsigned int newLocation) override;

    bool SetLibraryName(CBaseLibrary* lib, const QString& name) override;

protected:
    void SplitFullItemName(const QString& fullItemName, QString& libraryName, QString& itemName);
    void NotifyItemEvent(IDataBaseItem* pItem, EDataBaseItemEvent event);
    void SetRegisteredFlag(CBaseLibraryItem* pItem, bool bFlag);

    //////////////////////////////////////////////////////////////////////////
    // Must be overriden.
    //! Makes a new Item.
    virtual CBaseLibraryItem* MakeNewItem() = 0;
    virtual CBaseLibrary* MakeNewLibrary() = 0;
    //////////////////////////////////////////////////////////////////////////

    virtual void ReportDuplicateItem(CBaseLibraryItem* pItem, CBaseLibraryItem* pOldItem);

protected:
    bool m_bUniqGuidMap;
    bool m_bUniqNameMap;

    AZ_PUSH_DISABLE_DLL_EXPORT_MEMBER_WARNING
    //! Array of all loaded entity items libraries.
    std::vector<_smart_ptr<CBaseLibrary> > m_libs;

    // There is always one current level library.
    TSmartPtr<CBaseLibrary> m_pLevelLibrary;

    // GUID to item map.
    typedef std::map<GUID, _smart_ptr<CBaseLibraryItem>, guid_less_predicate> ItemsGUIDMap;
    ItemsGUIDMap m_itemsGuidMap;

    // Case insensitive name to items map.
    typedef std::map<QString, _smart_ptr<CBaseLibraryItem>, stl::less_stricmp<QString>> ItemsNameMap;
    ItemsNameMap m_itemsNameMap;
    AZStd::mutex m_itemsNameMapMutex;

    std::vector<IDataBaseManagerListener*> m_listeners;

    // Currently selected item.
    _smart_ptr<CBaseLibraryItem> m_pSelectedItem;
    _smart_ptr<CBaseLibraryItem> m_pSelectedParent;
    AZ_POP_DISABLE_DLL_EXPORT_MEMBER_WARNING
};

//////////////////////////////////////////////////////////////////////////
template <class TMap>
class CDataBaseItemEnumerator
    : public IDataBaseItemEnumerator
{
    TMap* m_pMap;
    typename TMap::iterator m_iterator;

public:
    CDataBaseItemEnumerator(TMap* pMap)
    {
        assert(pMap);
        m_pMap = pMap;
        m_iterator = m_pMap->begin();
    }
    void Release() override { delete this; };
    IDataBaseItem* GetFirst() override
    {
        m_iterator = m_pMap->begin();
        if (m_iterator == m_pMap->end())
        {
            return 0;
        }
        return m_iterator->second;
    }
    IDataBaseItem* GetNext() override
    {
        if (m_iterator != m_pMap->end())
        {
            m_iterator++;
        }
        if (m_iterator == m_pMap->end())
        {
            return 0;
        }
        return m_iterator->second;
    }
};

#endif // CRYINCLUDE_EDITOR_BASELIBRARYMANAGER_H
