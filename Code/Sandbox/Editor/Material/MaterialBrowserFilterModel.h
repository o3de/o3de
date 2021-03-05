/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#pragma once

#include <AzCore/std/parallel/containers/concurrent_unordered_map.h>
#include <AzCore/std/parallel/atomic.h>
#include <AzCore/Jobs/Job.h>

#include <AzToolsFramework/AssetBrowser/AssetBrowserFilterModel.h>
#include <AzToolsFramework/AssetBrowser/AssetBrowserBus.h>
#include <AzToolsFramework/AssetBrowser/Search/SearchWidget.h>
#include <AzToolsFramework/MaterialBrowser/MaterialBrowserBus.h>
#include <AzFramework/Asset/AssetCatalogBus.h>


#include <ISourceControl.h>
#include <QDateTime>

namespace AZ
{
    class Job;
    class JobManager;
    class JobCancelGroup;
    class JobContext;
}

class SubMaterialSearchFilter;
class LevelMaterialSearchFilter;

//////////////////////////////////////////////////////////////////////////
// Item class for browser.
//////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////// 
struct MaterialBrowserRecordAssetBrowserData
{
    AZ::Data::AssetId assetId;
    AZStd::string relativeFilePath;
    AZStd::string fullSourcePath;
    QPersistentModelIndex modelIndex;
    QPersistentModelIndex filterModelIndex;
};

class CMaterialBrowserRecord
{
public:
    CMaterialBrowserRecord()
    {
        InitializeSourceControlAttributes();
    }

    CMaterialBrowserRecord(const CMaterialBrowserRecord &rhs)
        : m_material(rhs.m_material)
        , m_lastCachedSCCAttributes(rhs.m_lastCachedSCCAttributes)
        , m_lastCachedFileAttributes(rhs.m_lastCachedFileAttributes)
        , m_lastCheckedSCCAttributes(rhs.m_lastCheckedSCCAttributes)
        , m_assetBrowserData(rhs.m_assetBrowserData)
    {
    }

    AZ::Data::AssetId GetAssetId() const { return m_assetBrowserData.assetId; }
    AZStd::string GetRelativeFilePath() const { return m_assetBrowserData.relativeFilePath; }
    AZStd::string GetFullSourcePath() const { return m_assetBrowserData.fullSourcePath; }
    QPersistentModelIndex GetModelIndex() const { return m_assetBrowserData.modelIndex; }
    QPersistentModelIndex GetFilterModelIndex() const { return m_assetBrowserData.filterModelIndex; }
    void SetAssetBrowserData(const MaterialBrowserRecordAssetBrowserData &assetBrowserData) { m_assetBrowserData = assetBrowserData; }
    void InitializeSourceControlAttributes()
    {
        // Force an update by setting the last update time to 1 / 1 / 1
        m_lastCachedSCCAttributes = AzToolsFramework::SourceControlFileInfo();
        m_lastCachedFileAttributes = SCC_FILE_ATTRIBUTE_INVALID;
        m_lastCheckedSCCAttributes = QDate(1, 1, 1).startOfDay();
    }
public:
    _smart_ptr<CMaterial> m_material = nullptr;
    AzToolsFramework::SourceControlFileInfo m_lastCachedSCCAttributes;
    ESccFileAttributes m_lastCachedFileAttributes;
    QDateTime m_lastCheckedSCCAttributes;

private:
    MaterialBrowserRecordAssetBrowserData m_assetBrowserData;
};
Q_DECLARE_METATYPE(CMaterialBrowserRecord)


//! MaterialBrowserSourceControlEvents
//! This bus informs the material browser filter model
//! when an ASYNC source control command has completed
class MaterialBrowserSourceControlEvents
    : public AZ::EBusTraits
{
public:
    typedef AZStd::recursive_mutex MutexType;

    static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single; // there's only one listener
    static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;  // theres only one listener
    virtual ~MaterialBrowserSourceControlEvents() {}

    //! Signals the callback for the GetFileInfo source control op
    virtual void UpdateSourceControlFileInfoCallback(const AZ::Data::AssetId &assetId, const AzToolsFramework::SourceControlFileInfo& fileInfo) = 0;
    //! Updates the timestamp for when the source control status was last checked
    virtual void UpdateSourceControlLastCheckedTime(const AZ::Data::AssetId &assetId, const QDateTime &dateTime) = 0;
};

using MaterialBrowserSourceControlBus = AZ::EBus<MaterialBrowserSourceControlEvents>;

//! MaterialBrowserWidgetEvents
//! This bus is used to send events to the material browser widget
class MaterialBrowserWidgetEvents
    : public AZ::EBusTraits
{
public:
    typedef AZStd::recursive_mutex MutexType;

    static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single; // there's only one listener
    static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single; // theres only one listener
    virtual ~MaterialBrowserWidgetEvents() {}

    //! Indicates that a material has finished being processed by the asset processor
    virtual void MaterialFinishedProcessing(_smart_ptr<CMaterial> material, const QPersistentModelIndex &filterModelIndex) = 0;

    //! Indicates that a material has finished being added to the material browser
    virtual void MaterialAddFinished() = 0;

    //! Indicates that the material record update for initially populating the material browser has finished
    virtual void MaterialRecordUpdateFinished() = 0;
};

using MaterialBrowserWidgetBus = AZ::EBus<MaterialBrowserWidgetEvents>;

/**
* Get the product material assetId for a given AssetBrowserEntry
* If there is no valid product material, the material has not been processed, or there are multiple product materials
* and thus there is not an individual material that can be assumed based on the source, an invalid assetId is returned
* @param assetEntry An asset entry that may be a source or a product
* @return The assetId of the product material.
*/
AZ::Data::AssetId GetMaterialProductAssetIdFromAssetBrowserEntry(const AzToolsFramework::AssetBrowser::AssetBrowserEntry* assetEntry);

class MaterialBrowserFilterModel
    : public AzToolsFramework::AssetBrowser::AssetBrowserFilterModel
    , public MaterialBrowserSourceControlBus::Handler
    , public AzToolsFramework::AssetBrowser::AssetBrowserModelNotificationBus::Handler
    , public AzToolsFramework::MaterialBrowser::MaterialBrowserRequestBus::Handler
    , private AzFramework::AssetCatalogEventBus::Handler
{
public:
    AZ_CLASS_ALLOCATOR(MaterialBrowserFilterModel, AZ::SystemAllocator, 0);
    MaterialBrowserFilterModel(QObject* parent);
    ~MaterialBrowserFilterModel();
    void UpdateRecord(const QModelIndex& filterModelIndex);
    void UpdateRecord(const MaterialBrowserRecordAssetBrowserData &assetBrowserData);
    void UpdateSourceControlFileInfoCallback(const AZ::Data::AssetId &assetId, const AzToolsFramework::SourceControlFileInfo& info) override;
    void UpdateSourceControlLastCheckedTime(const AZ::Data::AssetId &assetId, const QDateTime &dateTime) override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    bool TryGetRecordFromMaterial(_smart_ptr<CMaterial> material, CMaterialBrowserRecord& record) const;
    bool TryGetRecordFromAssetId(const AZ::Data::AssetId &assetId, CMaterialBrowserRecord& record) const;
    void SetRecord(const CMaterialBrowserRecord& record);
    void SetSearchFilter(const AzToolsFramework::AssetBrowser::SearchWidget* searchWidget);
    void StartRecordUpdateJobs();
    void CancelRecordUpdateJobs();
    void ClearRecordMap();
    void GetRelativeFilePaths(AZStd::vector<MaterialBrowserRecordAssetBrowserData> &files) const;
    QModelIndex GetIndexFromMaterial(_smart_ptr<CMaterial> material) const;
    QModelIndex GetFilterModelIndex(const AZ::Data::AssetId &assetId) const;

    // AssetBrowserModelNotificationBus event handlers
    void EntryAdded(const AzToolsFramework::AssetBrowser::AssetBrowserEntry* entry) override;
    
    void OnCatalogAssetChanged(const AZ::Data::AssetId& assetId) override;
    // MaterialBrowserRequestBus
    bool HasRecord(const AZ::Data::AssetId& assetId) override;
    bool IsMultiMaterial(const AZ::Data::AssetId& assetId) override;

    void ShowOnlyLevelMaterials(bool levelOnly, bool invalidateFilterNow = false);

    public Q_SLOTS:
    void SearchFilterUpdated();

private:
    void GetRelativeFilePathsRecursive(AZStd::vector<MaterialBrowserRecordAssetBrowserData> &files, const MaterialBrowserFilterModel* model, const QModelIndex &parent = QModelIndex()) const;
    bool TryGetFilterModelIndexRecursive(QModelIndex &filterModelIndex, const AZ::Data::AssetId &assetId, const MaterialBrowserFilterModel* model, const QModelIndex &parent = QModelIndex()) const;
    void QueueDataChangedEvent(const QPersistentModelIndex &filterModelIndex);
    AZStd::concurrent_unordered_map<AZ::Data::AssetId, CMaterialBrowserRecord> m_materialRecordMap;
    QVector<QPixmap> m_imageList;
    AzToolsFramework::AssetBrowser::FilterConstType m_assetTypeFilter;
    SubMaterialSearchFilter* m_subMaterialSearchFilter;
    LevelMaterialSearchFilter* m_levelMaterialSearchFilter;
    const AzToolsFramework::AssetBrowser::SearchWidget* m_searchWidget;

    void InitializeRecordUpdateJob();
    AZ::JobManager* m_jobManager = nullptr;
    AZ::JobCancelGroup* m_jobCancelGroup = nullptr;
    AZ::JobContext* m_jobContext = nullptr;
    AZ::Job* m_mainUpdateRecordJob = nullptr;
};

/**
* Job that walks through a MaterialBrowserFilterModel to generate a list of files,
* then divides the list amongst child jobs for processing
*/
class MaterialBrowserUpdateJobCreator
    : public AZ::Job
{
public:
    AZ_CLASS_ALLOCATOR(MaterialBrowserUpdateJobCreator, AZ::ThreadPoolAllocator, 0);

    MaterialBrowserUpdateJobCreator(MaterialBrowserFilterModel* model, AZStd::vector<MaterialBrowserRecordAssetBrowserData>& files, AZ::JobContext* context = nullptr);
    void Process() override;
private:
    MaterialBrowserFilterModel* m_filterModel; 
    AZStd::vector<MaterialBrowserRecordAssetBrowserData> m_files;
};

/**
* Job that walks through a list of material files, loads them, and then populates
* the MaterialBrowserFilterModel's map of material data
*/
class MaterialBrowserUpdateJob
    : public AZ::Job
{
public:
    AZ_CLASS_ALLOCATOR(MaterialBrowserUpdateJob, AZ::ThreadPoolAllocator, 0);

    MaterialBrowserUpdateJob(MaterialBrowserFilterModel* model, AZStd::vector<MaterialBrowserRecordAssetBrowserData> &files, AZ::JobContext* context = nullptr);
    void Process() override;
private:
    MaterialBrowserFilterModel* m_filterModel;
    AZStd::vector<MaterialBrowserRecordAssetBrowserData> m_files;
};

