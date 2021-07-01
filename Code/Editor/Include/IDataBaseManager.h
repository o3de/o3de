/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#ifndef CRYINCLUDE_EDITOR_INCLUDE_IDATABASEMANAGER_H
#define CRYINCLUDE_EDITOR_INCLUDE_IDATABASEMANAGER_H
#pragma once

#include <QString>

struct IDataBaseItem;
struct IDataBaseLibrary;
class CUsedResources;

enum EDataBaseItemEvent
{
    EDB_ITEM_EVENT_ADD,
    EDB_ITEM_EVENT_DELETE,
    EDB_ITEM_EVENT_CHANGED,
    EDB_ITEM_EVENT_SELECTED,
    EDB_ITEM_EVENT_UPDATE_PROPERTIES,
    EDB_ITEM_EVENT_UPDATE_PROPERTIES_NO_EDITOR_REFRESH
};

//////////////////////////////////////////////////////////////////////////
// Description:
//    Callback class to intercept item creation and deletion events.
//////////////////////////////////////////////////////////////////////////
struct IDataBaseManagerListener
{
    virtual void OnDataBaseItemEvent(IDataBaseItem* pItem, EDataBaseItemEvent event) = 0;
};

//////////////////////////////////////////////////////////////////////////
// Description:
//    his interface is used to enumerate al items registered to the database manager.
//////////////////////////////////////////////////////////////////////////
struct IDataBaseItemEnumerator
{
    virtual ~IDataBaseItemEnumerator() = default;
    
    virtual void Release() = 0;
    virtual IDataBaseItem* GetFirst() = 0;
    virtual IDataBaseItem* GetNext() = 0;
};

//////////////////////////////////////////////////////////////////////////
//
// Interface to the collection of all items or specific type
// in data base libraries.
//
//////////////////////////////////////////////////////////////////////////
struct IDataBaseManager
{
    //! Clear all libraries.
    virtual void ClearAll() = 0;

    //////////////////////////////////////////////////////////////////////////
    // Library items.
    //////////////////////////////////////////////////////////////////////////
    //! Make a new item in specified library.
    virtual IDataBaseItem* CreateItem(IDataBaseLibrary* pLibrary) = 0;
    //! Delete item from library and manager.
    virtual void DeleteItem(IDataBaseItem* pItem) = 0;

    //! Find Item by its GUID.
    virtual IDataBaseItem* FindItem(REFGUID guid) const = 0;
    virtual IDataBaseItem* FindItemByName(const QString& fullItemName) = 0;

    virtual IDataBaseItemEnumerator* GetItemEnumerator() = 0;

    // Select one item in DB.
    virtual void SetSelectedItem(IDataBaseItem* pItem) = 0;

    //////////////////////////////////////////////////////////////////////////
    // Libraries.
    //////////////////////////////////////////////////////////////////////////
    //! Add Item library.   Set isLevelLibrary to true if its the "level" library which gets saved inside the level
    virtual IDataBaseLibrary* AddLibrary(const QString& library, bool isLevelLibrary = false, bool bIsLoading = true) = 0;
    virtual void DeleteLibrary(const QString& library, bool forceDeleteLibrary = false) = 0;
    //! Get number of libraries.
    virtual int GetLibraryCount() const = 0;
    //! Get Item library by index.
    virtual IDataBaseLibrary* GetLibrary(int index) const = 0;

    //! Find Items Library by name.
    virtual IDataBaseLibrary* FindLibrary(const QString& library) = 0;

    //! Load Items library.
#ifdef LoadLibrary
#undef LoadLibrary
#endif
    virtual IDataBaseLibrary* LoadLibrary(const QString& filename, bool bReload = false) = 0;

    //! Save all modified libraries.
    virtual void SaveAllLibs() = 0;

    //! Serialize property manager.
    virtual void Serialize(XmlNodeRef& node, bool bLoading) = 0;

    //! Export items to game.
    virtual void Export([[maybe_unused]] XmlNodeRef& node) {};

    //! Returns unique name base on input name.
    virtual QString MakeUniqueItemName(const QString& name, const QString& libName = "") = 0;
    virtual QString MakeFullItemName(IDataBaseLibrary* pLibrary, const QString& group, const QString& itemName) = 0;

    //! Root node where this library will be saved.
    virtual QString GetRootNodeName() = 0;
    //! Path to libraries in this manager.
    virtual QString GetLibsPath() = 0;

    //////////////////////////////////////////////////////////////////////////
    //! Validate library items for errors.
    virtual void Validate() = 0;

    // Description:
    //      Collects names of all resource files used by managed items.
    // Arguments:
    //      resources - Structure where all filenames are collected.
    virtual void GatherUsedResources(CUsedResources& resources) = 0;

    //////////////////////////////////////////////////////////////////////////
    // Register listeners.
    virtual void AddListener(IDataBaseManagerListener* pListener) = 0;
    virtual void RemoveListener(IDataBaseManagerListener* pListener) = 0;
};

#endif // CRYINCLUDE_EDITOR_INCLUDE_IDATABASEMANAGER_H
