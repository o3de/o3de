#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

set(FILES
    Application/ToolsApplication.cpp
    Application/ToolsApplication.h

    ActionManager/Action/ActionManager.cpp
    ActionManager/Action/ActionManager.h
    ActionManager/Action/EditorAction.cpp
    ActionManager/Action/EditorAction.h
    ActionManager/Action/EditorActionContext.cpp
    ActionManager/Action/EditorActionContext.h
    ActionManager/Action/EditorWidgetAction.cpp
    ActionManager/Action/EditorWidgetAction.h
    ActionManager/HotKey/HotKeyManager.cpp
    ActionManager/HotKey/HotKeyManager.h
    ActionManager/HotKey/HotKeyWidgetRegistrationHelper.cpp
    ActionManager/HotKey/HotKeyWidgetRegistrationHelper.h
    ActionManager/Menu/EditorMenu.cpp
    ActionManager/Menu/EditorMenu.h
    ActionManager/Menu/EditorMenuBar.cpp
    ActionManager/Menu/EditorMenuBar.h
    ActionManager/Menu/MenuManager.cpp
    ActionManager/Menu/MenuManager.h
    ActionManager/ToolBar/EditorToolBar.cpp
    ActionManager/ToolBar/EditorToolBar.h
    ActionManager/ToolBar/EditorToolBarArea.cpp
    ActionManager/ToolBar/EditorToolBarArea.h
    ActionManager/ToolBar/ToolBarManager.cpp
    ActionManager/ToolBar/ToolBarManager.h
    ActionManager/ActionManagerSystemComponent.cpp
    ActionManager/ActionManagerSystemComponent.h

    Application/EditorEntityManager.cpp
    Application/EditorEntityManager.h
    Application/Ticker.cpp
    Application/Ticker.h
    
    Archive/ArchiveComponent.cpp
    Archive/ArchiveComponent.h
    
    Asset/AssetBundler.cpp
    Asset/AssetBundler.h
    Asset/AssetDebugInfo.cpp
    Asset/AssetDebugInfo.h
    Asset/AssetProcessorMessages.cpp
    Asset/AssetProcessorMessages.h
    Asset/AssetSeedManager.h
    Asset/AssetSeedManager.cpp
    Asset/AssetSystemComponent.cpp
    Asset/AssetSystemComponent.h
    Asset/AssetUtils.cpp
    Asset/AssetUtils.h

    AssetBrowser/AssetPicker/AssetPickerDialog.cpp
    AssetBrowser/AssetPicker/AssetPickerDialog.h
    AssetBrowser/AssetPicker/AssetPickerDialog.ui
    AssetBrowser/Entries/AssetBrowserEntryCache.cpp
    AssetBrowser/Entries/AssetBrowserEntryCache.h
    AssetBrowser/Entries/AssetBrowserEntryUtils.cpp
    AssetBrowser/Entries/AssetBrowserEntryUtils.h
    AssetBrowser/Entries/FolderAssetBrowserEntry.cpp
    AssetBrowser/Entries/FolderAssetBrowserEntry.h
    AssetBrowser/Entries/ProductAssetBrowserEntry.cpp
    AssetBrowser/Entries/ProductAssetBrowserEntry.h
    AssetBrowser/Entries/RootAssetBrowserEntry.cpp
    AssetBrowser/Entries/RootAssetBrowserEntry.h
    AssetBrowser/Entries/SourceAssetBrowserEntry.cpp
    AssetBrowser/Entries/SourceAssetBrowserEntry.h
    AssetBrowser/Favorites/AssetBrowserFavoriteItem.cpp
    AssetBrowser/Favorites/AssetBrowserFavoriteItem.h
    AssetBrowser/Favorites/AssetBrowserFavoritesManager.cpp
    AssetBrowser/Favorites/AssetBrowserFavoritesManager.h
    AssetBrowser/Favorites/AssetBrowserFavoritesModel.cpp
    AssetBrowser/Favorites/AssetBrowserFavoritesModel.h
    AssetBrowser/Favorites/AssetBrowserFavoritesView.cpp
    AssetBrowser/Favorites/AssetBrowserFavoritesView.h
    AssetBrowser/Favorites/EntryAssetBrowserFavoriteItem.cpp
    AssetBrowser/Favorites/EntryAssetBrowserFavoriteItem.h
    AssetBrowser/Favorites/FavoritesEntryDelegate.cpp
    AssetBrowser/Favorites/FavoritesEntryDelegate.h
    AssetBrowser/Favorites/SearchAssetBrowserFavoriteItem.cpp
    AssetBrowser/Favorites/SearchAssetBrowserFavoriteItem.h
    AssetBrowser/Previewer/EmptyPreviewer.cpp
    AssetBrowser/Previewer/EmptyPreviewer.h
    AssetBrowser/Previewer/EmptyPreviewer.ui
    AssetBrowser/Previewer/Previewer.cpp
    AssetBrowser/Previewer/Previewer.h
    AssetBrowser/Previewer/PreviewerFrame.cpp
    AssetBrowser/Previewer/PreviewerFrame.h
    AssetBrowser/Search/Filter.cpp
    AssetBrowser/Search/Filter.h
    AssetBrowser/Search/FilterByWidget.cpp
    AssetBrowser/Search/FilterByWidget.h
    AssetBrowser/Search/FilterByWidget.ui
    AssetBrowser/Search/SearchAssetTypeSelectorWidget.cpp
    AssetBrowser/Search/SearchAssetTypeSelectorWidget.h
    AssetBrowser/Search/SearchAssetTypeSelectorWidget.ui
    AssetBrowser/Search/SearchParametersWidget.cpp
    AssetBrowser/Search/SearchParametersWidget.h
    AssetBrowser/Search/SearchParametersWidget.ui
    AssetBrowser/Search/SearchWidget.cpp
    AssetBrowser/Search/SearchWidget.h
    AssetBrowser/Search/SearchWidget.ui
    AssetBrowser/Thumbnails/FolderThumbnail.cpp
    AssetBrowser/Thumbnails/FolderThumbnail.h
    AssetBrowser/Thumbnails/ProductThumbnail.cpp
    AssetBrowser/Thumbnails/ProductThumbnail.h
    AssetBrowser/Thumbnails/SourceThumbnail.cpp
    AssetBrowser/Thumbnails/SourceThumbnail.h
    
    AssetBrowser/Views/AssetBrowserFolderWidget.cpp
    AssetBrowser/Views/AssetBrowserFolderWidget.h
    AssetBrowser/Views/AssetBrowserFolderWidget.qrc
    AssetBrowser/Views/AssetBrowserListView.cpp
    AssetBrowser/Views/AssetBrowserListView.h
    AssetBrowser/Views/AssetBrowserTableView.cpp
    AssetBrowser/Views/AssetBrowserTableView.h
    AssetBrowser/Views/AssetBrowserThumbnailView.cpp
    AssetBrowser/Views/AssetBrowserThumbnailView.h
    AssetBrowser/Views/AssetBrowserTreeViewDialog.cpp
    AssetBrowser/Views/AssetBrowserTreeViewDialog.h
    AssetBrowser/Views/AssetBrowserViewUtils.cpp
    AssetBrowser/Views/AssetBrowserViewUtils.h
    AssetBrowser/Views/EntryDelegate.cpp
    AssetBrowser/Views/EntryDelegate.h
    
    AssetBrowser/AssetBrowserComponent.cpp
    AssetBrowser/AssetBrowserComponent.h
    AssetBrowser/AssetBrowserEntityInspectorWidget.cpp
    AssetBrowser/AssetBrowserEntityInspectorWidget.h
    AssetBrowser/AssetBrowserFilterModel.cpp
    AssetBrowser/AssetBrowserFilterModel.h
    AssetBrowser/AssetBrowserListModel.cpp
    AssetBrowser/AssetBrowserListModel.h
    AssetBrowser/AssetBrowserModel.cpp
    AssetBrowser/AssetBrowserModel.h
    AssetBrowser/AssetBrowserTableViewProxyModel.cpp
    AssetBrowser/AssetBrowserTableViewProxyModel.h
    AssetBrowser/AssetBrowserThumbnailViewProxyModel.cpp
    AssetBrowser/AssetBrowserThumbnailViewProxyModel.h
    AssetBrowser/AssetBrowserTreeToTableProxyModel.cpp
    AssetBrowser/AssetBrowserTreeToTableProxyModel.h
    AssetBrowser/AssetEntryChangeset.cpp
    AssetBrowser/AssetEntryChangeset.h
    AssetBrowser/AssetSelectionModel.cpp
    AssetBrowser/AssetSelectionModel.h

    AzToolsFrameworkModule.cpp
    AzToolsFrameworkModule.h
)

# Prevent the following files from being grouped in UNITY builds
set(SKIP_UNITY_BUILD_INCLUSION_FILES
    # The following files are skipped from unity to avoid duplicated symbols related to an ebus
    AzToolsFrameworkModule.cpp
    Application/ToolsApplication.cpp
#    UI/PropertyEditor/PropertyEntityIdCtrl.cpp
#    UI/PropertyEditor/PropertyManagerComponent.cpp
#    ViewportSelection/EditorDefaultSelection.cpp
#    ViewportSelection/EditorInteractionSystemComponent.cpp
#    ViewportSelection/EditorTransformComponentSelection.cpp
)
