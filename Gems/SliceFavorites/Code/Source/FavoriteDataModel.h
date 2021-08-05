/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/XML/rapidxml.h>

#include <AzFramework/Asset/AssetCatalogBus.h>
#include <AzToolsFramework/AssetBrowser/AssetBrowserBus.h>

#include <AzQtComponents/Buses/DragAndDrop.h>

#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzToolsFramework/AssetBrowser/AssetBrowserBus.h>

AZ_PUSH_DISABLE_WARNING(4251 4800, "-Wunknown-warning-option") // (qwidget.h) 'uint': forcing value to bool 'true' or 'false' (performance warning)
#include <QAbstractItemModel>
#include <QList>
#include <QMenu>
AZ_POP_DISABLE_WARNING
#include <QModelIndex>
#include <QSettings>

#include <SliceFavorites/SliceFavoritesBus.h>
#endif

class QTreeView;
class QMimeData;

namespace AzToolsFramework
{
    namespace AssetBrowser
    {
        class ProductAssetBrowserEntry;
    }
}

namespace SliceFavorites
{
    class FavoriteData
    {
    public:

        static QString GetMimeType() { return "sliceFavorites/favoriteData"; }

        typedef enum
        {
            DataType_Unknown = 0,
            DataType_Folder,
            DataType_Favorite
        } FavoriteType;

        typedef enum
        {
            Default = 0,
            Slice,
            DynamicSlice
        } FavoriteSubType;

        FavoriteData()
            : m_type(DataType_Unknown)
            , m_subType(Default)
        {
        }

        FavoriteData(FavoriteType type, FavoriteSubType subType = FavoriteSubType::Default)
            : m_type(type)
            , m_subType(subType)
        {
        }

        FavoriteData(const QString& name, FavoriteType type = DataType_Favorite, FavoriteSubType subType = FavoriteSubType::Default)
            : m_name(name)
            , m_type(type)
            , m_subType(subType)
        {
        }

        FavoriteData(const QString& name, const AZ::Data::AssetId assetId, FavoriteType type = DataType_Favorite, FavoriteSubType subType = FavoriteSubType::Default)
            : m_name(name)
            , m_assetId(assetId)
            , m_type(type)
            , m_subType(subType)
        {
        }

        ~FavoriteData();

        int LoadFromXML(AZ::rapidxml::xml_node<char>* node, const FavoriteData* root);
        int AddToXML(AZ::rapidxml::xml_node<char>* node, AZ::rapidxml::xml_document<char>* xmlDoc) const;

        QString m_name;
        AZ::Data::AssetId m_assetId;
        FavoriteType m_type;
        FavoriteSubType m_subType = FavoriteSubType::Default;

        QList<FavoriteData*> m_children;
        FavoriteData* m_parent = nullptr;

        void appendChild(FavoriteData* child);
        FavoriteData* child(int row);
        int childCount() const;
        int columnCount() const;
        QVariant data(int role) const;
        int row() const;
        FavoriteData* parentItem() { return m_parent; }

        int GetNumFoldersInHierarchy() const;
        int GetNumFavoritesInHierarchy() const;

        void Reset();

    private:
        int GetNumOfType(FavoriteType type) const;

        QString GenerateTooltip() const;

        bool IsAssetUnique(AZ::Data::AssetId assetId, const FavoriteData* root);
    };

    class FavoriteDataModel 
        : public QAbstractItemModel
        , private AzToolsFramework::EditorEvents::Bus::Handler
        , private AzToolsFramework::AssetBrowser::AssetBrowserInteractionNotificationBus::Handler
        , private AzFramework::AssetCatalogEventBus::Handler
        , private AzQtComponents::DragAndDropEventsBus::Handler
        , private AzToolsFramework::AssetBrowser::AssetBrowserComponentNotificationBus::Handler
    {
        Q_OBJECT

    public:
        explicit FavoriteDataModel(QObject *parent = nullptr);
        ~FavoriteDataModel();

        QVariant data(const QModelIndex &index, int role) const override;
        Qt::ItemFlags flags(const QModelIndex &index) const override;
        QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
        QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const override;
        QModelIndex parent(const QModelIndex &index) const override;
        int rowCount(const QModelIndex &parent = QModelIndex()) const override;
        int columnCount(const QModelIndex &parent = QModelIndex()) const override;
        bool moveRows(const QModelIndex& sourceParent, int sourceRow, int count, const QModelIndex& destinationParent, int destinationChild) override;

        size_t GetNumFavorites();
        void EnumerateFavorites(const AZStd::function<void(const AZ::Data::AssetId& assetId)>& callback);

        QMenu* GetFavoritesMenu() { return m_favoritesMenu.get(); }

        QModelIndex AddNewFolder(const QModelIndex& parent);

        void RemoveFavorite(const QModelIndex& indexToRemove);

        QModelIndex GetModelIndexForParent(const FavoriteData* child) const;
        QModelIndex GetModelIndexForFavorite(const FavoriteData* favorite) const;

        bool IsDescendentOf(QModelIndex index, QModelIndex potentialAncestor);

        FavoriteData* GetFavoriteDataFromModelIndex(const QModelIndex& modelIndex) const;

        void CountFoldersAndFavoritesFromIndices(const QModelIndexList& indices, int& numFolders, int& numFavorites);
        int GetNumFavoritesAndFolders();

        void ClearAll();

        bool HasFavoritesOrFolders() const;

        int ImportFavorites(const QString& importFileName);
        int ExportFavorites(const QString& exportFileName) const;

        void AddFavorite(const AzToolsFramework::AssetBrowser::ProductAssetBrowserEntry* product, const QModelIndex parent = QModelIndex());

    Q_SIGNALS:
        void DataModelChanged();
        void ExpandIndex(const QModelIndex& index, bool expanded);
        void DisplayWarning(const QString& title, const QString& message);
    public Q_SLOTS:
        void ProcessRemovedAssets();
    private:

        //////////////////////////////////////////////////////////////////////////
        // AzQtComponents::DragAndDropEventsBus::Handler
        //////////////////////////////////////////////////////////////////////////
        void DragEnter(QDragEnterEvent* event, AzQtComponents::DragAndDropContextBase& context) override;
        void DragMove(QDragMoveEvent* event, AzQtComponents::DragAndDropContextBase& context) override;
        void DragLeave(QDragLeaveEvent* event) override;
        void Drop(QDropEvent* event, AzQtComponents::DragAndDropContextBase& context) override;

        ////////////////////////////////////////////////////////////////////////
        // AssetCatalogBus::Handler overrides
        void OnCatalogAssetRemoved(const AZ::Data::AssetId& assetId, const AZ::Data::AssetInfo& assetInfo) override;
        void OnAssetBrowserComponentReady() override;
        ////////////////////////////////////////////////////////////////////////

        ////////////////////////////////////////////////////////////////////////
        // AztoolsFramework::EditorEvents::Bus::Handler overrides
        void PopulateEditorGlobalContextMenu_SliceSection(QMenu* menu, const AZ::Vector2& point, int flags) override;
        ////////////////////////////////////////////////////////////////////////

        ////////////////////////////////////////////////////////////////////////
        // AzToolsFramework::AssetBrowser::AssetBrowserInteractionNotificationBus::Handler
        void AddContextMenuActions(QWidget* /*caller*/, QMenu* menu, const AZStd::vector<AzToolsFramework::AssetBrowser::AssetBrowserEntry*>& entries) override;
        void NotifyRegisterViews() override;
        ////////////////////////////////////////////////////////////////////////

        AZStd::unique_ptr<FavoriteData> m_rootItem = nullptr;
        AZStd::unique_ptr<QMenu> m_favoritesMenu = nullptr;

        using FavoriteMap = AZStd::unordered_map<AZ::Data::AssetId, FavoriteData*>;
        using FavoriteList = QList<FavoriteData*>;

        FavoriteMap m_favoriteMap;

        bool IsFavorite(const AzToolsFramework::AssetBrowser::ProductAssetBrowserEntry* product) const;
        void RemoveFavorite(const AzToolsFramework::AssetBrowser::ProductAssetBrowserEntry* product);

        void RemoveFavorite(const AZ::Data::AssetId& assetId);

        void UpdateFavorites();
        void LoadFavorites();
        void SaveFavorites();
        void WriteChildren(QSettings& settings, FavoriteList& currentList);
        void ReadChildren(QSettings& settings, FavoriteList& currentList);
        void BuildChildToParentMap();
        void UpdateChildren(FavoriteData* parent);
        void RebuildMenu();
        void AddFavoriteToMenu(const FavoriteData* favorite, QMenu* menu);
        void RemoveFavorite(const FavoriteData* toRemove);
        void RemoveFromFavoriteMap(const FavoriteData* toRemove, bool removeChildren = true);

        FavoriteData* GetFavoriteDataFromAssetId(const AZ::Data::AssetId& assetId) const;

        QString GetProjectName();

        QStringList mimeTypes() const override;
        QMimeData* mimeData(const QModelIndexList& indexes) const override;

        void GetSelectedIndicesFromMimeData(QList<FavoriteData*>& results, const QByteArray& buffer) const;

        const AzToolsFramework::AssetBrowser::ProductAssetBrowserEntry* GetSliceProductFromBrowserEntry(AzToolsFramework::AssetBrowser::AssetBrowserEntry* entry) const;
        bool IsSliceAssetType(const AZ::Data::AssetType& type) const;

        bool dropMimeData(const QMimeData* data, Qt::DropAction action, int row, int column, const QModelIndex& parent) override;
        bool canDropMimeData(const QMimeData* data, Qt::DropAction action, int row, int column, const QModelIndex& parent) const override;

        bool setData(const QModelIndex& index, const QVariant& value, int role = Qt::EditRole) override;

        bool CanAcceptDragAndDropEvent(QDropEvent* event, AzQtComponents::DragAndDropContextBase& context) const;

        AZStd::vector<AZ::Data::AssetId> m_removedAssets;
    };
}
