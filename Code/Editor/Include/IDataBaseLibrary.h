/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#ifndef CRYINCLUDE_EDITOR_INCLUDE_IDATABASELIBRARY_H
#define CRYINCLUDE_EDITOR_INCLUDE_IDATABASELIBRARY_H
#pragma once


struct IDataBaseManager;
struct IDataBaseItem;

class QString;
class XmlNodeRef;

//////////////////////////////////////////////////////////////////////////
// Description:
//      Interface to access specific library of editor data base.
//      Ex. Archetype library, Material Library.
// See Also:
//     IDataBaseItem,IDataBaseManager
//////////////////////////////////////////////////////////////////////////
struct IDataBaseLibrary
{
    // Description:
    //      Return IDataBaseManager interface to the manager for items stored in this library.
    virtual IDataBaseManager* GetManager() = 0;

    // Description:
    //      Return library name.
    virtual const QString& GetName() const = 0;

    // Description:
    //      Return filename where this library is stored.
    virtual const QString& GetFilename() const = 0;

    // Description:
    //      Save contents of library to file.
    virtual bool Save() = 0;

    // Description:
    //      Load library from file.
    // Arguments:
    //      filename - Full specified library filename (relative to root game folder).
    virtual bool Load(const QString& filename) = 0;

    // Description:
    //      Serialize library parameters and items to/from XML node.
    virtual void Serialize(XmlNodeRef& node, bool bLoading) = 0;

    // Description:
    //      Marks library as modified, indicates that some item in library was modified.
    virtual void SetModified(bool bModified = true) = 0;

    // Description:
    //      Check if library parameters or any items where modified.
    //      If any item was modified library may need saving before closing editor.
    virtual bool IsModified() const = 0;

    // Description:
    //      Check if this library is not shared and internal to current level.
    virtual bool IsLevelLibrary() const = 0;

    // Description:
    //      Make this library accessible only from current Level. (not shared)
    virtual void SetLevelLibrary(bool bEnable) = 0;

    // Description:
    //      Associate a new item with the library.
    //      Watch out if item was already in another library.
    virtual void AddItem(IDataBaseItem* pItem, bool bRegister = true) = 0;

    // Description:
    //      Return number of items in library.
    virtual int GetItemCount() const = 0;

    // Description:
    //      Get item by index.
    // See Also:
    //    GetItemCount
    // Arguments:
    //      index - Index from 0 to GetItemCount()
    virtual IDataBaseItem* GetItem(int index) = 0;

    // Description:
    //      Remove item from library, does not destroy item,
    //      only unliks it from this library, to delete item use IDataBaseManager.
    // See Also:
    //     AddItem
    virtual void RemoveItem(IDataBaseItem* item) = 0;

    // Description:
    //      Remove all items from library, does not destroy items,
    //      only unliks them from this library, to delete item use IDataBaseManager.
    // See Also:
    //      RemoveItem,AddItem
    virtual void RemoveAllItems() = 0;

    // Description:
    //      Find item in library by name.
    //      This function usually uses linear search so it is not particularry fast.
    // See Also:
    //      GetItem
    virtual IDataBaseItem* FindItem(const QString& name) = 0;


    //CONFETTI BEGIN
    // Used to change the library item order
    virtual void ChangeItemOrder(CBaseLibraryItem* item, unsigned int newLocation) = 0;
    //CONFETTI END
};

#endif // CRYINCLUDE_EDITOR_INCLUDE_IDATABASELIBRARY_H
