/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#if !defined(Q_MOC_RUN)
#include <QtGui/qstandarditemmodel.h>
#include <QFileIconProvider>
#include <QThread>
#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/std/containers/unordered_map.h>

#include <AzFramework/Asset/AssetCatalogBus.h>
#include <AzToolsFramework/UI/SearchWidget/SearchCriteriaWidget.hxx>
#endif

///////////////////////////////////////////////////////////////////////////////
struct DatabaseEntry
{
public:
    DatabaseEntry(AZ::Data::AssetId assetID, const char *assetPath)
        : m_id(assetID)
        , m_path(assetPath)
    {}
    AZ::Data::AssetId m_id;
    QString m_path;
};

///////////////////////////////////////////////////////////////////////////////
class AssetCatalogEntry
    : public QStandardItem
{
public:
    //  This will be easier to store in the data, so that filters don't have to cast the item to get to it.
    enum Roles
    {
        FileIconRole = Qt::DecorationRole,
        FilePathRole = Qt::UserRole + 1,
        VisibilityRole = Qt::UserRole + 2,
        FolderRole = Qt::UserRole + 3
    };

    AssetCatalogEntry() {}
    AZ_CLASS_ALLOCATOR(AssetCatalogEntry, AZ::SystemAllocator, 0);

    bool operator<(const QStandardItem& other) const override;

public:
    AZ::Data::AssetId   m_assetId;      ///<  The unique ID of the asset in the asset database.
    AZ::Data::AssetType m_assetType;    ///<  The type of the asset is used to validate on certain drop targets, like the PropertyAssetCtrl.
    AZ::Uuid            m_classId;      ///<  If valid, the component that should be created when this asset is dragged onto creation-capable windows.
};

///////////////////////////////////////////////////////////////////////////////
class AssetCatalogModel
    : public QStandardItemModel
    , public AzFramework::AssetCatalogEventBus::Handler
{
    Q_OBJECT

public:
    AZ_CLASS_ALLOCATOR(AssetCatalogModel, AZ::SystemAllocator, 0);

    AssetCatalogModel(QObject* parent = 0);
    ~AssetCatalogModel() override;

    QString RootPath() const { return m_rootPath; }
    void LoadDatabase();

    QString FileName(const QModelIndex& index) const;
    QString FilePath(const QModelIndex& index) const;
    AssetCatalogEntry* AssetData(const QModelIndex& index) const;

    //! Finds an asset. On success, returns a pointer to the item.
    //! \retrun A valid pointer on success, nullptr on fail.
    AssetCatalogEntry* FindAsset(QString assetPath);

    //  QAbstractItemModel overrides
    QVariant    data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QMimeData* mimeData(const QModelIndexList& indexes) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    // AzFramework::AssetCatalogEventBus::Handler
    void OnCatalogAssetAdded(const AZ::Data::AssetId& assetId) override;
    void OnCatalogAssetRemoved(const AZ::Data::AssetId& assetId, const AZ::Data::AssetInfo& assetInfo) override;

Q_SIGNALS:
    void LoadComplete();
    void SetTotalProgress(int value);
    void UpdateProgress(int value);

public Q_SLOTS:
    void SearchCriteriaChanged(QStringList& criteriaList, AzToolsFramework::FilterOperatorType filterOperator);
    void ProcessAssets();
    void StartProcessingAssets();
    void StopProcessingAssets();

protected:
    //! Adds an asset and returns a pointer to that new asset.
    AssetCatalogEntry* AddAsset(QString assetPath, AZ::Data::AssetId id);
    //! Removes an asset. On success, returns a pointer to the parent item. On failure, returns nullptr.
    AssetCatalogEntry* RemoveAsset(QString assetPath);

    void BuildFilter(QStringList& criteriaList, AzToolsFramework::FilterOperatorType filterOperator);
    void InvalidateFilter();
    void SetFilterRegExp(const AZStd::string& filterType, const QRegExp& regExp);
    void ClearFilterRegExp(const AZStd::string& filterType = AZStd::string());

    AZ::Data::AssetType GetAssetType(const QString &filename) const;
    QStandardItem* GetPath(QString& path, bool createIfNeeded, QStandardItem* parent = nullptr);

    void ApplyFilter(QStandardItem* parent);

    AZStd::unordered_map<AZ::Data::AssetType, QIcon> m_assetTypeToIcon;
    AZStd::unordered_map<AZ::Uuid, AZ::Uuid> m_assetTypeToComponent;
    AZStd::unordered_map<AZStd::string, AZStd::vector<AZ::Uuid>> m_extensionToAssetType;

    QFileIconProvider   m_iconProvider;
    QString             m_rootPath;

    AzToolsFramework::FilterByCategoryMap m_filtersRegExp;

    static const int ASSET_CATALOG_BATCH_SIZE = 50;

    AZStd::vector<DatabaseEntry*> m_fileCache;   //  scratch space to get the registry data out of the AssetDatabase in quick fashion.
    int                          m_fileCacheCurrentIndex;
    bool                         m_canProcessAssets;
};

///////////////////////////////////////////////////////////////////////////////
class AssetCatalogModelWorkerThread
    : public QThread
{
    Q_OBJECT

public:
    AssetCatalogModelWorkerThread(AssetCatalogModel* catalog, QThread* returnThread);

    void startJob();

public Q_SLOTS:
    void ReturnToThread();

protected:
    void run() override;
    //  These are pointers that this object will not own.
    QThread* m_returnThread;
    AssetCatalogModel* m_catalog;

};
