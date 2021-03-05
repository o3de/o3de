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

#include "EditorDefs.h"

#include "MaterialBrowserFilterModel.h"

// AzFramework
#include <AzCore/Jobs/JobFunction.h>

// AzToolsFramework
#include <AzToolsFramework/API/EditorAssetSystemAPI.h>
#include <AzToolsFramework/AssetBrowser/EBusFindAssetTypeByName.h>
#include <AzToolsFramework/AssetBrowser/AssetBrowserModel.h>
#include <AzToolsFramework/AssetBrowser/AssetBrowserEntry.h>

// Editor
#include "MaterialBrowserSearchFilters.h"
#include "MaterialManager.h"


namespace
{
    // how often to re-query source control status of an item after querying it, in seconds.
    // note that using a source control operation on an item invalidates the cache and will immediately
    // refresh its status regardless of this value, so it can be pretty high.
    qint64 g_timeRefreshSCCStatus = 60;
}

#define IDC_MATERIAL_TREECTRL 3

//////////////////////////////////////////////////////////////////////////
#define ITEM_IMAGE_SHARED_MATERIAL   0
#define ITEM_IMAGE_SHARED_MATERIAL_SELECTED 1
#define ITEM_IMAGE_FOLDER            2
#define ITEM_IMAGE_FOLDER_OPEN       3
#define ITEM_IMAGE_MATERIAL          4
#define ITEM_IMAGE_MATERIAL_SELECTED 5
#define ITEM_IMAGE_MULTI_MATERIAL    6
#define ITEM_IMAGE_MULTI_MATERIAL_SELECTED 7


#define ITEM_IMAGE_OVERLAY_CGF          8
#define ITEM_IMAGE_OVERLAY_INPAK        9
#define ITEM_IMAGE_OVERLAY_READONLY     10
#define ITEM_IMAGE_OVERLAY_ONDISK       11
#define ITEM_IMAGE_OVERLAY_LOCKED       12
#define ITEM_IMAGE_OVERLAY_CHECKEDOUT   13
#define ITEM_IMAGE_OVERLAY_NO_CHECKOUT  14
//////////////////////////////////////////////////////////////////////////

AZ::Data::AssetId GetMaterialProductAssetIdFromAssetBrowserEntry(const AzToolsFramework::AssetBrowser::AssetBrowserEntry* assetEntry)
{
    using namespace AzToolsFramework::AssetBrowser;

    if (assetEntry->GetEntryType() == AssetBrowserEntry::AssetEntryType::Source ||
        assetEntry->GetEntryType() == AssetBrowserEntry::AssetEntryType::Product)
    {
        AZStd::vector<const ProductAssetBrowserEntry*> products;
        assetEntry->GetChildrenRecursively<ProductAssetBrowserEntry>(products);

        // Cache the material asset type because this function is called for every submaterial when searching.
        // AssetType is a POD struct so it should be fine to leave it as a static that gets destroyed during library unload/program termination.
        static AZ::Data::AssetType materialAssetType = AZ::Data::AssetType::CreateNull();
        if (materialAssetType.IsNull())
        {
            EBusFindAssetTypeByName materialAssetTypeResult("Material");
            AZ::AssetTypeInfoBus::BroadcastResult(materialAssetTypeResult, &AZ::AssetTypeInfo::GetAssetType);
            AZ_Assert(materialAssetTypeResult.Found(), "Could not find asset type for material asset");
            materialAssetType = materialAssetTypeResult.GetAssetType();
        }

        for (const auto* product : products)
        {
            if (product->GetAssetType() == materialAssetType)
            {
                return product->GetAssetId();
            }
        }
    }
    return AZ::Data::AssetId();
}

MaterialBrowserFilterModel::MaterialBrowserFilterModel(QObject* parent)
    : AzToolsFramework::AssetBrowser::AssetBrowserFilterModel(parent)
{
    using namespace AzToolsFramework::AssetBrowser;
    using namespace AzToolsFramework::MaterialBrowser;

    MaterialBrowserSourceControlBus::Handler::BusConnect();
    AssetBrowserModelNotificationBus::Handler::BusConnect();
    MaterialBrowserRequestBus::Handler::BusConnect();
    AzFramework::AssetCatalogEventBus::Handler::BusConnect();

    m_imageList.push_back(QPixmap(":/MaterialBrowser/images/material_00.png"));
    m_imageList.push_back(QPixmap(":/MaterialBrowser/images/material_01.png"));
    m_imageList.push_back(QPixmap(":/MaterialBrowser/images/material_02.png"));
    m_imageList.push_back(QPixmap(":/MaterialBrowser/images/material_03.png"));
    m_imageList.push_back(QPixmap(":/MaterialBrowser/images/material_04.png"));
    m_imageList.push_back(QPixmap(":/MaterialBrowser/images/material_05.png"));
    m_imageList.push_back(QPixmap(":/MaterialBrowser/images/material_06.png"));
    m_imageList.push_back(QPixmap(":/MaterialBrowser/images/material_07.png"));
    m_imageList.push_back(QPixmap(":/MaterialBrowser/images/filestatus_00.png"));
    m_imageList.push_back(QPixmap(":/MaterialBrowser/images/filestatus_01.png"));
    m_imageList.push_back(QPixmap(":/MaterialBrowser/images/filestatus_02.png"));
    m_imageList.push_back(QPixmap(":/MaterialBrowser/images/filestatus_03.png"));
    m_imageList.push_back(QPixmap(":/MaterialBrowser/images/filestatus_04.png"));
    m_imageList.push_back(QPixmap(":/MaterialBrowser/images/filestatus_05.png"));
    m_imageList.push_back(QPixmap(":/MaterialBrowser/images/filestatus_06.png"));


    // Create an asset type filter for materials
    AssetTypeFilter* assetTypeFilter = new AssetTypeFilter();
    assetTypeFilter->SetAssetType("Material");
    assetTypeFilter->SetFilterPropagation(AssetBrowserEntryFilter::PropagateDirection::Down); //this will make sure folders that contain materials are displayed
    m_assetTypeFilter = FilterConstType(assetTypeFilter);

    // Create search filters. SetSearchFilter stores these filters so that they are deleted when this object is deleted.
    m_subMaterialSearchFilter = new SubMaterialSearchFilter(this);
    m_levelMaterialSearchFilter = new LevelMaterialSearchFilter(this);

    InitializeRecordUpdateJob();
}

MaterialBrowserFilterModel::~MaterialBrowserFilterModel()
{
    SAFE_DELETE(m_jobCancelGroup);
    SAFE_DELETE(m_jobContext);
    SAFE_DELETE(m_jobManager);

    AzToolsFramework::AssetBrowser::AssetBrowserModelNotificationBus::Handler::BusDisconnect();
    MaterialBrowserSourceControlBus::Handler::BusDisconnect();
    AzToolsFramework::MaterialBrowser::MaterialBrowserRequestBus::Handler::BusDisconnect();
    AzFramework::AssetCatalogEventBus::Handler::BusDisconnect();
}

void MaterialBrowserFilterModel::UpdateRecord(const QModelIndex& filterModelIndex)
{
    using namespace AzToolsFramework::AssetBrowser;

    if (filterModelIndex.isValid())
    {
        QModelIndex modelIndex = mapToSource(filterModelIndex);

        AssetBrowserEntry* assetEntry = static_cast<AssetBrowserEntry*>(modelIndex.internalPointer());

        if (assetEntry->GetEntryType() == AssetBrowserEntry::AssetEntryType::Source ||
            assetEntry->GetEntryType() == AssetBrowserEntry::AssetEntryType::Product)
        {
            AZStd::vector<const ProductAssetBrowserEntry*> products;
            assetEntry->GetChildrenRecursively<ProductAssetBrowserEntry>(products);

            for (const auto* product : products)
            {
                if (!m_assetTypeFilter->Match(product))
                {
                    continue;
                }
                MaterialBrowserRecordAssetBrowserData assetBrowserData;
                assetBrowserData.assetId = product->GetAssetId();
                assetBrowserData.relativeFilePath = product->GetRelativePath();
                assetBrowserData.fullSourcePath = product->GetFullPath();
                assetBrowserData.modelIndex = modelIndex;
                assetBrowserData.filterModelIndex = filterModelIndex;
                UpdateRecord(assetBrowserData);
            }
        }
    }
}

void MaterialBrowserFilterModel::UpdateRecord(const MaterialBrowserRecordAssetBrowserData& assetBrowserData)
{
    CMaterialBrowserRecord record;
    record.SetAssetBrowserData(assetBrowserData);
    record.m_material = GetIEditor()->GetMaterialManager()->LoadMaterial(record.GetRelativeFilePath().c_str());
    SetRecord(record);
}

void MaterialBrowserFilterModel::UpdateSourceControlFileInfoCallback(const AZ::Data::AssetId& assetId, const AzToolsFramework::SourceControlFileInfo& info)
{
    CMaterialBrowserRecord record;
    bool found = m_materialRecordMap.find(assetId, &record);
    if (found)
    {
        // Update the cached source control attributes for the record
        record.m_lastCachedSCCAttributes = info;
        record.m_lastCachedFileAttributes = static_cast<ESccFileAttributes>(CFileUtil::GetAttributes(record.GetFullSourcePath().c_str(), false));
        record.m_lastCheckedSCCAttributes = QDateTime::currentDateTime();

        // Update the record
        m_materialRecordMap.erase(assetId);
        m_materialRecordMap.insert(AZStd::pair<AZ::Data::AssetId, CMaterialBrowserRecord>(record.GetAssetId(), record));

        QueueDataChangedEvent(record.GetFilterModelIndex());
    }
}

void MaterialBrowserFilterModel::UpdateSourceControlLastCheckedTime(const AZ::Data::AssetId& assetId, const QDateTime& dateTime)
{
    CMaterialBrowserRecord record;
    bool found = m_materialRecordMap.find(assetId, &record);
    if (found)
    {
        // Update the record
        record.m_lastCheckedSCCAttributes = dateTime;
        m_materialRecordMap.erase(assetId);
        m_materialRecordMap.insert(AZStd::pair<AZ::Data::AssetId, CMaterialBrowserRecord>(record.GetAssetId(), record));
    }
}

QVariant MaterialBrowserFilterModel::data(const QModelIndex& index, int role /* = Qt::DisplayRole*/) const
{
    // Should return data from an AssetBrowserEntry, or get material specific info from that data
    if (index.isValid())
    {
        // Use the base AssetBrowserFilterModel::data function for display role
        if (role != AzToolsFramework::AssetBrowser::AssetBrowserModel::Roles::EntryRole)
        {
            QModelIndex modelIndex = mapToSource(index);
            AzToolsFramework::AssetBrowser::AssetBrowserEntry* assetEntry = static_cast<AzToolsFramework::AssetBrowser::AssetBrowserEntry*>(modelIndex.internalPointer());

            AZ::Data::AssetId assetId = GetMaterialProductAssetIdFromAssetBrowserEntry(assetEntry);
            if (assetId.IsValid())
            {
                CMaterialBrowserRecord record;
                bool found = m_materialRecordMap.find(assetId, &record);

                if (role == Qt::UserRole)
                {
                    if (found)
                    {
                        return QVariant::fromValue(record);
                    }
                    else
                    {
                        return QVariant();
                    }
                }
            }
        }
    }
    // If the role is Qt::DisplayRole, the item a folder, or the material has not been parsed yet, fall back on the default data result from the underlying AssetBrowserModel
    return AzToolsFramework::AssetBrowser::AssetBrowserFilterModel::data(index, role);
}

void MaterialBrowserFilterModel::GetRelativeFilePaths(AZStd::vector<MaterialBrowserRecordAssetBrowserData>& files) const
{
    GetRelativeFilePathsRecursive(files, this);
}

void MaterialBrowserFilterModel::GetRelativeFilePathsRecursive(AZStd::vector<MaterialBrowserRecordAssetBrowserData>& files,
    const MaterialBrowserFilterModel* model, const QModelIndex& parent) const
{
    using namespace AzToolsFramework::AssetBrowser;

    for (int r = 0; r < model->rowCount(parent); ++r)
    {
        QModelIndex index = model->index(r, 0, parent);
        QModelIndex modelIndex = model->mapToSource(index);

        if (model->hasChildren(index))
        {
            GetRelativeFilePathsRecursive(files, model, index);
        }
        else
        {
            AssetBrowserEntry* assetEntry = static_cast<AssetBrowserEntry*>(modelIndex.internalPointer());

            AZStd::vector<const ProductAssetBrowserEntry*> products;
            assetEntry->GetChildrenRecursively<ProductAssetBrowserEntry>(products);

            for (const auto* product : products)
            {
                if (!m_assetTypeFilter->Match(product))
                {
                    continue;
                }
                MaterialBrowserRecordAssetBrowserData item;
                item.assetId = product->GetAssetId();
                item.fullSourcePath = product->GetFullPath();
                item.relativeFilePath = product->GetRelativePath();
                item.modelIndex = modelIndex;
                item.filterModelIndex = index;
                files.push_back(item);
            }
        }
    }
}

QModelIndex MaterialBrowserFilterModel::GetIndexFromMaterial(_smart_ptr<CMaterial> material) const
{
    CMaterialBrowserRecord record;
    bool found = TryGetRecordFromMaterial(material, record);
    if (found)
    {
        return record.GetFilterModelIndex();
    }
    return QModelIndex();
}

QModelIndex MaterialBrowserFilterModel::GetFilterModelIndex(const AZ::Data::AssetId& assetId) const
{
    QModelIndex filterModelIndex;
    if (TryGetFilterModelIndexRecursive(filterModelIndex, assetId, this))
    {
        return filterModelIndex;
    }

    return QModelIndex();
}

bool MaterialBrowserFilterModel::TryGetFilterModelIndexRecursive(QModelIndex& filterModelIndex,
    const AZ::Data::AssetId& assetId, const MaterialBrowserFilterModel* model, const QModelIndex& parent) const
{
    using namespace AzToolsFramework::AssetBrowser;

    // Walk through the filter model to find the product entry with the corresponding assetId
    for (int r = 0; r < model->rowCount(parent); ++r)
    {
        QModelIndex index = model->index(r, 0, parent);
        QModelIndex modelIndex = model->mapToSource(index);

        if (model->hasChildren(index))
        {
            if (TryGetFilterModelIndexRecursive(filterModelIndex, assetId, model, index))
            {
                return true;
            }
        }
        else
        {
            // Check to see if the current entry is the one we're looking for
            AssetBrowserEntry* assetEntry = static_cast<AssetBrowserEntry*>(modelIndex.internalPointer());

            AZStd::vector<const ProductAssetBrowserEntry*> products;
            assetEntry->GetChildrenRecursively<ProductAssetBrowserEntry>(products);

            for (const auto* product : products)
            {
                if (assetId == product->GetAssetId())
                {
                    filterModelIndex = index;
                    return true;
                }
            }
        }
    }

    return false;
}

void MaterialBrowserFilterModel::EntryAdded(const AzToolsFramework::AssetBrowser::AssetBrowserEntry* entry)
{
    // If the entry is a product material
    if (entry->GetEntryType() == AzToolsFramework::AssetBrowser::AssetBrowserEntry::AssetEntryType::Product && m_assetTypeFilter->Match(entry))
    {
        // capture the data here, so that entry cannot disappear before we actually get to the job.
        MaterialBrowserRecordAssetBrowserData assetBrowserData;
        assetBrowserData.assetId = GetMaterialProductAssetIdFromAssetBrowserEntry(entry);
        assetBrowserData.relativeFilePath = entry->GetRelativePath();
        assetBrowserData.fullSourcePath = entry->GetFullPath();
        assetBrowserData.filterModelIndex = GetFilterModelIndex(assetBrowserData.assetId);

        // Create a job to add/update the entry in the underlying map
        AZ::Job* updateEntryJob = AZ::CreateJobFunction([this, assetBrowserData]()
                {
                    CMaterialBrowserRecord record;
                    record.SetAssetBrowserData(assetBrowserData);
                    record.m_material = GetIEditor()->GetMaterialManager()->LoadMaterialWithFullSourcePath(record.GetRelativeFilePath().c_str(), record.GetFullSourcePath().c_str());
                    SetRecord(record);
                    MaterialBrowserWidgetBus::Broadcast(&MaterialBrowserWidgetEvents::MaterialAddFinished);
                }, true, m_jobContext);

        // Start the job immediately
        AZ::Job* currentJob = m_jobContext->GetJobManager().GetCurrentJob();
        if (currentJob)
        {
            // Suspend the current job until the new job completes so that
            // if a new material is created by the user it's ready to use sooner
            currentJob->StartAsChild(updateEntryJob);
            currentJob->WaitForChildren();
        }
        else
        {
            updateEntryJob->Start();
        }
    }
}


void MaterialBrowserFilterModel::OnCatalogAssetChanged(const AZ::Data::AssetId& assetId)
{
    CMaterialBrowserRecord record;
    bool found = m_materialRecordMap.find(assetId, &record);
    if (found)
    {
        record.m_material->Reload();
        //Send out event to notify UI update if it's currently selected
        MaterialBrowserWidgetBus::Broadcast(&MaterialBrowserWidgetEvents::MaterialFinishedProcessing, record.m_material, record.GetFilterModelIndex());
    }
}

bool MaterialBrowserFilterModel::HasRecord(const AZ::Data::AssetId& assetId) 
{
    return m_materialRecordMap.find(assetId);
}

bool MaterialBrowserFilterModel::IsMultiMaterial(const AZ::Data::AssetId& assetId) 
{
    CMaterialBrowserRecord record;
    bool found = m_materialRecordMap.find(assetId, &record);
    if (found)
    {
        if (record.m_material && record.m_material->IsMultiSubMaterial())
        {
            return true;
        }
    }
    return false;
}

bool MaterialBrowserFilterModel::TryGetRecordFromMaterial(_smart_ptr<CMaterial> material, CMaterialBrowserRecord& record) const
{
    if (material)
    {
        // Get the relative path for the material
        bool pathFound = false;
        AZStd::string relativePath;
        AzToolsFramework::AssetSystemRequestBus::BroadcastResult(pathFound, &AzToolsFramework::AssetSystem::AssetSystemRequest::GetRelativeProductPathFromFullSourceOrProductPath, material->GetFilename().toUtf8().data(), relativePath);
        AZ_Assert(pathFound, "Failed to get engine relative path from %s", material->GetFilename().toUtf8().data());

        // Get the assetId from the relative path
        AZ::Data::AssetId assetId;

        AZ::Data::AssetCatalogRequestBus::BroadcastResult(assetId, &AZ::Data::AssetCatalogRequests::GetAssetIdByPath, relativePath.c_str(), GetIEditor()->GetMaterialManager()->GetMaterialAssetType(), false);

        return TryGetRecordFromAssetId(assetId, record);
    }

    return false;
}

bool MaterialBrowserFilterModel::TryGetRecordFromAssetId(const AZ::Data::AssetId& assetId, CMaterialBrowserRecord& record) const
{
    bool recordFound = m_materialRecordMap.find(assetId, &record);
    return recordFound;
}

void MaterialBrowserFilterModel::SetRecord(const CMaterialBrowserRecord& record)
{
    m_materialRecordMap.erase(record.GetAssetId());
    m_materialRecordMap.insert({ record.GetAssetId(), record });

    QueueDataChangedEvent(record.GetFilterModelIndex());
}

void MaterialBrowserFilterModel::SetSearchFilter(const AzToolsFramework::AssetBrowser::SearchWidget* searchWidget)
{
    /*
     *   The filter matches the following rule:
     *   A. If the entry is a material
     *       1. The material's name matches the search text
     *       2. The sub material's name matches the search text
     *   B. If the entry is a folder:
     *       1. The folder's name matches the search text
     *       2. The folder contains a material matching A
     *       3. The folder contains a folder matching B.1 & B.2
     */

    using namespace AzToolsFramework::AssetBrowser;

    m_searchWidget = searchWidget;

    // Create a search filter where a search text either matches entry/entry parent's name or sub material name
    CompositeFilter* nameFilter = new CompositeFilter(CompositeFilter::LogicOperatorType::OR);
    QSharedPointer<CompositeFilter> searchWidgetFilter = m_searchWidget->GetFilter();
    // The default setting for search widget filter is Down
    // Since now we only need to match for the entry itself in this filter, set back to None
    searchWidgetFilter->SetFilterPropagation(AssetBrowserEntryFilter::PropagateDirection::None);
    nameFilter->AddFilter(FilterConstType(m_subMaterialSearchFilter));
    nameFilter->AddFilter(FilterConstType(searchWidgetFilter));

    EntryTypeFilter* productsFilter = new EntryTypeFilter();
    productsFilter->SetEntryType(AssetBrowserEntry::AssetEntryType::Product);
    InverseFilter* noProductsFilter = new InverseFilter();
    noProductsFilter->SetFilter(FilterConstType(productsFilter));

    // Create a filter where the entry needs to match the previous name filter and it needs to be material itself or it contains material.
    CompositeFilter* isMaterialAndMatchesSearchFilter = new CompositeFilter(CompositeFilter::LogicOperatorType::AND);
    isMaterialAndMatchesSearchFilter->AddFilter(FilterConstType(nameFilter));
    isMaterialAndMatchesSearchFilter->AddFilter(FilterConstType(m_assetTypeFilter));
    isMaterialAndMatchesSearchFilter->AddFilter(FilterConstType(noProductsFilter));
    isMaterialAndMatchesSearchFilter->AddFilter(FilterConstType(m_levelMaterialSearchFilter));
    // Make sure any folder contains the matching result is included
    isMaterialAndMatchesSearchFilter->SetFilterPropagation(AssetBrowserEntryFilter::PropagateDirection::Down);

    // Set the filter for the MaterialBrowserFilterModel
    SetFilter(FilterConstType(isMaterialAndMatchesSearchFilter));
}

void MaterialBrowserFilterModel::ShowOnlyLevelMaterials(bool levelOnly, bool invalidateFilterNow)
{
    m_levelMaterialSearchFilter->ShowOnlyLevelMaterials(levelOnly);
    if (levelOnly)
    {
        m_levelMaterialSearchFilter->CacheLoadedMaterials();
    }

    if (invalidateFilterNow)
    {
        // we need to invalid the filter immediately, for example this is used when we are changing level, otherwise
        // the current filter is used when getting file paths for all of the materials as part of StartRecordUpdateJobs.
        invalidateFilter();
    }
    else
    {
        filterUpdatedSlot();
    }
}

void MaterialBrowserFilterModel::SearchFilterUpdated()
{
    m_subMaterialSearchFilter->SetFilterString(m_searchWidget->textFilter());
    filterUpdatedSlot();
}

void MaterialBrowserFilterModel::QueueDataChangedEvent(const QPersistentModelIndex& filterModelIndex)
{
    // Queue a data changed event to be executed on the main thread
    AZStd::function<void()> emitDataChanged =
        [this, filterModelIndex]()
        {
            if (filterModelIndex.isValid())
            {
                Q_EMIT dataChanged(filterModelIndex, filterModelIndex);
            }
        };

    AZ::TickBus::QueueFunction(emitDataChanged);
}

void MaterialBrowserFilterModel::InitializeRecordUpdateJob()
{
    AZ::JobManagerDesc desc;
    AZ::JobManagerThreadDesc threadDesc;
    for (size_t i = 0; i < AZStd::thread::hardware_concurrency(); ++i)
    {
        desc.m_workerThreads.push_back(threadDesc);
    }

    // Check to ensure these have not already been initialized.
    AZ_Error("Material Browser", !m_jobManager && !m_jobCancelGroup && !m_jobContext, "MaterialBrowserFilterModel::InitializeRecordUpdateJob is being called again after it has already been initialized");
    m_jobManager = aznew AZ::JobManager(desc);
    m_jobCancelGroup = aznew AZ::JobCancelGroup();
    m_jobContext = aznew AZ::JobContext(*m_jobManager, *m_jobCancelGroup);
}

void MaterialBrowserFilterModel::StartRecordUpdateJobs()
{
    // Generate a list of file paths, assetId's, and asset browser model indices
    // This must be done on the main thread, otherwise it can lead to a crash when the tree view UI is being initialized
    AZStd::vector<MaterialBrowserRecordAssetBrowserData> files;
    GetRelativeFilePaths(files);
    
    // Kick off the background process that will iterate over the list of file paths and update the material record map
    m_mainUpdateRecordJob = aznew MaterialBrowserUpdateJobCreator(this, files, m_jobContext);
    m_mainUpdateRecordJob->Start();
}

void MaterialBrowserFilterModel::CancelRecordUpdateJobs()
{
    m_jobContext->GetCancelGroup()->Cancel();
    m_jobContext->GetCancelGroup()->Reset();
}

void MaterialBrowserFilterModel::ClearRecordMap()
{
    CancelRecordUpdateJobs();
    m_materialRecordMap.clear();
}

MaterialBrowserUpdateJobCreator::MaterialBrowserUpdateJobCreator(MaterialBrowserFilterModel* model, AZStd::vector<MaterialBrowserRecordAssetBrowserData>& files, AZ::JobContext* context /*= NULL*/)
    : Job(true, context)
    , m_files(files)
    , m_filterModel(model)
{
}

void MaterialBrowserUpdateJobCreator::Process()
{
    // Split the files to be processed evenly among threads
    int numJobs = GetContext()->GetJobManager().GetNumWorkerThreads();
    int materialsPerJob = (m_files.size() / numJobs) + 1;

    for (auto it = m_files.begin(); it <= m_files.end(); it += materialsPerJob)
    {
        // Create a subset of the list of material files to be processed by another job
        auto start = it;
        auto end = it + materialsPerJob;
        if (end > m_files.end())
        {
            end = m_files.end();
        }
        AZStd::vector<MaterialBrowserRecordAssetBrowserData> subset(start, end);

        if (subset.size() > 0)
        {
            AZ::Job* materialBrowserUpdateJob = aznew MaterialBrowserUpdateJob(m_filterModel, subset, GetContext());
            StartAsChild(materialBrowserUpdateJob);
        }
    }

    WaitForChildren();

    MaterialBrowserWidgetBus::Broadcast(&MaterialBrowserWidgetEvents::MaterialRecordUpdateFinished);
}

MaterialBrowserUpdateJob::MaterialBrowserUpdateJob(MaterialBrowserFilterModel* model, AZStd::vector<MaterialBrowserRecordAssetBrowserData>& files, AZ::JobContext* context /*= NULL*/)
    : Job(true, context)
    , m_filterModel(model)
    , m_files(files)
{
}

void MaterialBrowserUpdateJob::Process()
{
    for (size_t i = 0; i < m_files.size(); ++i)
    {
        // Early out when the job is cancelled
        if (IsCancelled())
        {
            return;
        }
        CMaterialBrowserRecord record;
        record.SetAssetBrowserData(m_files[i]);

        // Get the writable status of the file, but don't update source control status until it is actually needed
        record.m_lastCachedFileAttributes = static_cast<ESccFileAttributes>(CFileUtil::GetAttributes(record.GetFullSourcePath().c_str(), false));

        record.m_material = GetIEditor()->GetMaterialManager()->LoadMaterialWithFullSourcePath(record.GetRelativeFilePath().c_str(), record.GetFullSourcePath().c_str());
        m_filterModel->SetRecord(record);
    }
}
