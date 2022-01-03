/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#ifndef CRYINCLUDE_EDITOR_BASELIBRARY_H
#define CRYINCLUDE_EDITOR_BASELIBRARY_H
#pragma once

#if !defined(Q_MOC_RUN)
#include "Include/IDataBaseLibrary.h"
#include "Include/IBaseLibraryManager.h"
#include "Include/EditorCoreAPI.h"
#include "Util/TRefCountBase.h"

#include <QObject>
#endif

// Ensure we don't try to dllimport when moc includes us
#if defined(Q_MOC_BUILD) && !defined(EDITOR_CORE)
#define EDITOR_CORE
#endif

/** This a base class for all Libraries used by Editor.
*/
class EDITOR_CORE_API CBaseLibrary
    : public QObject
    , public TRefCountBase<IDataBaseLibrary>
{
    Q_OBJECT

public:
    explicit CBaseLibrary(IBaseLibraryManager* pManager);
    ~CBaseLibrary();

    //! Set library name.
    virtual void SetName(const QString& name);
    //! Get library name.
    const QString& GetName() const override;

    //! Set new filename for this library.
    virtual bool SetFilename(const QString& filename, [[maybe_unused]] bool checkForUnique = true) { m_filename = filename.toLower(); return true; };
    const QString& GetFilename() const override { return m_filename; };

    bool Save() override = 0;
    bool Load(const QString& filename) override = 0;
    void Serialize(XmlNodeRef& node, bool bLoading) override = 0;

    //! Mark library as modified.
    void SetModified(bool bModified = true) override;
    //! Check if library was modified.
    bool IsModified() const override { return m_bModified; };

    //////////////////////////////////////////////////////////////////////////
    // Working with items.
    //////////////////////////////////////////////////////////////////////////
    //! Add a new prototype to library.
    void AddItem(IDataBaseItem* item, bool bRegister = true) override;
    //! Get number of known prototypes.
    int GetItemCount() const override { return static_cast<int>(m_items.size()); }
    //! Get prototype by index.
    IDataBaseItem* GetItem(int index) override;

    //! Delete item by pointer of item.
    void RemoveItem(IDataBaseItem* item) override;

    //! Delete all items from library.
    void RemoveAllItems() override;

    //! Find library item by name.
    //! Using linear search.
    IDataBaseItem* FindItem(const QString& name) override;

    //! Check if this library is local level library.
    bool IsLevelLibrary() const override { return m_bLevelLib; };

    //! Set library to be level library.
    void SetLevelLibrary(bool bEnable) override { m_bLevelLib = bEnable; };

    //////////////////////////////////////////////////////////////////////////
    //! Return manager for this library.
    IBaseLibraryManager* GetManager() override;

    // Saves the library with the main tag defined by the parameter name
    bool SaveLibrary(const char* name, bool saveEmptyLibrary = false);

    //CONFETTI BEGIN
    // Used to change the library item order
    void ChangeItemOrder(CBaseLibraryItem* item, unsigned int newLocation) override;
    //CONFETTI END

signals:
    void Modified(bool bModified);

private:
    // Add the library to the source control
    bool AddLibraryToSourceControl(const QString& fullPathName) const;

protected:

    //! Name of the library.
    QString m_name;
    //! Filename of the library.
    QString m_filename;

    //! Flag set when library was modified.
    bool m_bModified;

    // Flag set when the library is just created and it's not yet saved for the first time.
    bool m_bNewLibrary;

    //! Level library is saved within the level .ly file and is local for this level.
    bool m_bLevelLib;

    //////////////////////////////////////////////////////////////////////////
    // Manager.
    IBaseLibraryManager* m_pManager;

    AZ_PUSH_DISABLE_DLL_EXPORT_MEMBER_WARNING
    // Array of all our library items.
    std::vector<_smart_ptr<CBaseLibraryItem> > m_items;
    AZ_POP_DISABLE_DLL_EXPORT_MEMBER_WARNING
};

#endif // CRYINCLUDE_EDITOR_BASELIBRARY_H
