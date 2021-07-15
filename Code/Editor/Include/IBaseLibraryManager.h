/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#ifndef CRYINCLUDE_EDITOR_INCLUDE_IBASELIBRARYMANAGER_H
#define CRYINCLUDE_EDITOR_INCLUDE_IBASELIBRARYMANAGER_H
#pragma once

#include <IEditor.h>
#include "Include/IDataBaseItem.h"
#include "Include/IDataBaseLibrary.h"
#include "Include/IDataBaseManager.h"
#include "Util/TRefCountBase.h"

class CBaseLibraryItem;
class CBaseLibrary;

struct IBaseLibraryManager
    : public TRefCountBase<IDataBaseManager>
    , public IEditorNotifyListener
{
    //! Clear all libraries.
    virtual void ClearAll() = 0;

    //////////////////////////////////////////////////////////////////////////
    // IDocListener implementation.
    //////////////////////////////////////////////////////////////////////////
    virtual void OnEditorNotifyEvent(EEditorNotifyEvent event) = 0;

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
    virtual IDataBaseItem* LoadItemByName(const QString& fullItemName) = 0;

    virtual IDataBaseItemEnumerator* GetItemEnumerator() = 0;

    //////////////////////////////////////////////////////////////////////////
    // Set item currently selected.
    virtual void SetSelectedItem(IDataBaseItem* pItem) = 0;
    // Get currently selected item.
    virtual IDataBaseItem* GetSelectedItem() const = 0;
    virtual IDataBaseItem* GetSelectedParentItem() const = 0;

    //////////////////////////////////////////////////////////////////////////
    // Libraries.
    //////////////////////////////////////////////////////////////////////////
    //! Add Item library.
    virtual IDataBaseLibrary* AddLibrary(const QString& library, bool isLevelLibrary = false, bool bIsLoading = true) = 0;
    virtual void DeleteLibrary(const QString& library, bool forceDeleteLevel = false) = 0;
    //! Get number of libraries.
    virtual int GetLibraryCount() const = 0;
    //! Get number of modified libraries.
    virtual int GetModifiedLibraryCount() const = 0;

    //! Get Item library by index.
    virtual IDataBaseLibrary* GetLibrary(int index) const = 0;

    //! Get Level Item library.
    virtual IDataBaseLibrary* GetLevelLibrary() const = 0;

    //! Find Items Library by name.
    virtual IDataBaseLibrary* FindLibrary(const QString& library) = 0;

    //! Find the Library's index by name.
    virtual int FindLibraryIndex(const QString& library) = 0;
    
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
    virtual void Export(XmlNodeRef& node) = 0;

    //! Returns unique name base on input name.
    // Vera@conffx, add LibName parameter so we could make an unique name depends on input library.
    // Arguments:
    //    - name: name of the item
    //    - libName: The library of the item. Given the library name, the function will return a unique name in the library
    //      Default value "": The function will ignore the library name and return a unique name in the manager
    virtual QString MakeUniqueItemName(const QString& name, const QString& libName = "") = 0;
    virtual QString MakeFullItemName(IDataBaseLibrary* pLibrary, const QString& group, const QString& itemName) = 0;

    //! Root node where this library will be saved.
    virtual QString GetRootNodeName() = 0;
    //! Path to libraries in this manager.
    virtual QString GetLibsPath() = 0;

    //////////////////////////////////////////////////////////////////////////
    //! Validate library items for errors.
    virtual void Validate() = 0;

    //////////////////////////////////////////////////////////////////////////
    virtual void GatherUsedResources(CUsedResources& resources) = 0;

    virtual void AddListener(IDataBaseManagerListener* pListener) = 0;
    virtual void RemoveListener(IDataBaseManagerListener* pListener) = 0;

    //////////////////////////////////////////////////////////////////////////
    virtual void RegisterItem(CBaseLibraryItem* pItem, REFGUID newGuid) = 0;
    virtual void RegisterItem(CBaseLibraryItem* pItem) = 0;
    virtual void UnregisterItem(CBaseLibraryItem* pItem) = 0;

    // Only Used internally.
    virtual void OnRenameItem(CBaseLibraryItem* pItem, const QString& oldName) = 0;

    // Called by items to indicated that they have been modified.
    // Sends item changed event to listeners.
    virtual void OnItemChanged(IDataBaseItem* pItem) = 0;
    virtual void OnUpdateProperties(IDataBaseItem* pItem, bool bRefresh) = 0;

    //CONFETTI BEGIN
    // Used to change the library item order
    virtual void ChangeLibraryOrder(IDataBaseLibrary* lib, unsigned int newLocation) = 0;
    // simplifies the library renaming process
    virtual bool SetLibraryName(CBaseLibrary* lib, const QString& name) = 0;

    
    //Check if the file name is unique.
    //Params: library: library name. NOT the file path.
    virtual bool IsUniqueFilename(const QString& library) = 0;
    //CONFETTI END
};

#endif // CRYINCLUDE_EDITOR_INCLUDE_IBASELIBRARYMANAGER_H
