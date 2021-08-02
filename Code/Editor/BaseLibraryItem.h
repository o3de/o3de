/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#ifndef CRYINCLUDE_EDITOR_BASELIBRARYITEM_H
#define CRYINCLUDE_EDITOR_BASELIBRARYITEM_H
#pragma once

#include "Include/IDataBaseItem.h"
#include "BaseLibrary.h"

#include <QMetaType>

class CBaseLibrary;

//////////////////////////////////////////////////////////////////////////
AZ_PUSH_DISABLE_DLL_EXPORT_BASECLASS_WARNING
/** Base class for all items contained in BaseLibraray.
*/
class EDITOR_CORE_API CBaseLibraryItem
    : public TRefCountBase<IDataBaseItem>
{
    AZ_POP_DISABLE_DLL_EXPORT_BASECLASS_WARNING
public:
    CBaseLibraryItem();
    ~CBaseLibraryItem();

    //! Set item name.
    //! Its virtual, in case you want to override it in derrived item.
    virtual void SetName(const QString& name);
    //! Get item name.
    const QString& GetName() const;

    //! Get full item name, including name of library.
    //! Name formed by adding dot after name of library
    //! eg. library Pickup and item PickupRL form full item name: "Pickups.PickupRL".
    QString GetFullName() const;

    //! Get only nameof group from prototype.
    QString GetGroupName();
    //! Get short name of prototype without group.
    QString GetShortName();

    //! Return Library this item are contained in.
    //! Item can only be at one library.
    IDataBaseLibrary* GetLibrary() const;
    void SetLibrary(CBaseLibrary* pLibrary);

    //////////////////////////////////////////////////////////////////////////
    //! Serialize library item to archive.
    virtual void Serialize(SerializeContext& ctx);

    //////////////////////////////////////////////////////////////////////////
    //! Generate new unique id for this item.
    void GenerateId();
    //! Returns GUID of this material.
    const GUID& GetGUID() const { return m_guid; }

    //! Mark library as modified.
    void SetModified(bool bModified = true);
    //! Check if library was modified.
    bool IsModified() const { return m_bModified; };

    //! Returns true if the item is registered, otherwise false
    bool IsRegistered() const { return m_bRegistered; };

    //! Validate item for errors.
    virtual void Validate() {};

    //! Get number of sub childs.
    virtual int GetChildCount() const { return 0; }
    //! Get sub child by index.
    virtual CBaseLibraryItem* GetChild([[maybe_unused]] int index) const { return nullptr; }


    //////////////////////////////////////////////////////////////////////////
    //! Gathers resources by this item.
    virtual void GatherUsedResources([[maybe_unused]] CUsedResources& resources) {};

    //! Get if stored item is enabled
    virtual bool GetIsEnabled() { return true; };


    int IsParticleItem = -1;
protected:
    void SetGUID(REFGUID guid);
    friend class CBaseLibrary;
    friend class CBaseLibraryManager;
    // Name of this prototype.
    QString m_name;
    AZ_PUSH_DISABLE_DLL_EXPORT_MEMBER_WARNING
    //! Reference to prototype library who contains this prototype.
    _smart_ptr<CBaseLibrary> m_library;
    AZ_POP_DISABLE_DLL_EXPORT_MEMBER_WARNING

    //! Every base library item have unique id.
    GUID m_guid;
    // True when item modified by editor.
    bool m_bModified;
    // True when item registered in manager.
    bool m_bRegistered = false;
};

Q_DECLARE_METATYPE(CBaseLibraryItem*);

TYPEDEF_AUTOPTR(CBaseLibraryItem);


#endif // CRYINCLUDE_EDITOR_BASELIBRARYITEM_H
