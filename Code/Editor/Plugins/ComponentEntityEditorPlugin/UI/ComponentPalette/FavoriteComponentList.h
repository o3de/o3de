/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include "ComponentDataModel.h"
#include "FilteredComponentList.h"
#include "ComponentPaletteSettings.h"

#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/UserSettings/UserSettingsProvider.h>
#include <AzToolsFramework/ToolsComponents/ComponentMimeData.h>
#include <AzToolsFramework/ToolsComponents/ComponentAssetMimeDataContainer.h>
#endif

//! FavoriteComponentListRequest
//! Bus that provides a way for external features to record favorites
class FavoriteComponentListRequest : public AZ::EBusTraits
{
public:
    virtual void AddFavorites(const AZStd::vector<const AZ::SerializeContext::ClassData*>&) = 0;
};

using FavoriteComponentListRequestBus = AZ::EBus<FavoriteComponentListRequest>;


//! FavoritesDataModel 
//! Stores the list of component class data to display in the favorites control, offers persistence through user settings.
class FavoritesDataModel 
    : public ComponentDataModel
{
    Q_OBJECT

public:

    AZ_CLASS_ALLOCATOR(FavoritesDataModel, AZ::SystemAllocator, 0);

    FavoritesDataModel(QWidget* parent = nullptr);
    ~FavoritesDataModel() override;

    //! Add a favorite component
    //! \param classData The ClassData information for the component to store as favorite
    //! \param updateSettings Optional parameter used to determine if the persistent settings need to be updated.
    void AddFavorite(const AZ::SerializeContext::ClassData* classData, bool updateSettings = true);

    //! Remove all the specified items from the table
    //! \param indices List of indices to remove from favorites
    void Remove(const QModelIndexList& indices);

    //! Save the list of favorite components to user settings
    void SaveState();
    
    //! Load the list of favorite components from user settings
    void LoadState();

protected:

    void SetSavedStateKey(AZ::u32 key);

    // Qt handlers
    QModelIndex index(int row, int column, const QModelIndex &parent) const override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    bool dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent) override;

    // List of component class data
    QList<const AZ::SerializeContext::ClassData*> m_favorites;

    // The Palette settings and provider information for saving out the Favorites list
    AZStd::intrusive_ptr<ComponentPaletteSettings> m_settings;
    AZ::UserSettingsProvider m_provider;
    AZ::u32 m_providerId;
};


//! FavoritesList
//! User customized list of favorite components, provides persistence.
class FavoritesList 
    : public FilteredComponentList
    , FavoriteComponentListRequestBus::Handler
{
    Q_OBJECT

public:

    explicit FavoritesList(QWidget* parent = nullptr);
    ~FavoritesList() override;

    void Init() override;

protected:

    //////////////////////////////////////////////////////////////////////////
    // FavoriteComponentListRequestBus
    void AddFavorites(const AZStd::vector<const AZ::SerializeContext::ClassData*>& classDataContainer) override;
    //////////////////////////////////////////////////////////////////////////

    void rowsInserted(const QModelIndex& parent, int start, int end) override;

    // Context menu handlers
    void ShowContextMenu(const QPoint&);
    void ContextMenu_RemoveSelectedFavorites();

    // Validate data being dragged in
    void dragEnterEvent(QDragEnterEvent * event) override;
    void dragMoveEvent(QDragMoveEvent* event) override;

    //! Handler used when dropping PaletteItems into the Viewport.
    static void DragDropHandler(CViewport* viewport, int ptx, int pty, void* custom);
};
