/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <EditorDefs.h>
#include "CryEdit.h"
#include "AssetCatalogModel.h"
#include "Objects/ComponentEntityObject.h"

#include <ISourceControl.h>
#include <IEditor.h>
#include <qevent.h>
#include <qmimedata.h>

#include <LmbrCentral/Rendering/LensFlareAsset.h>
#include <LmbrCentral/Rendering/MeshAsset.h>
#include <LmbrCentral/Rendering/MaterialAsset.h>

#include <AzCore/Memory/Memory.h>
#include <AzCore/RTTI/TypeInfo.h>
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Settings/SettingsRegistryMergeUtils.h>
#include <AzCore/Slice/SliceAsset.h>
#include <AzCore/Asset/AssetManager.h>
#include <AzCore/Asset/AssetManagerBus.h>
#include <AzCore/Asset/AssetTypeInfoBus.h>

#include <AzFramework/API/ApplicationAPI.h>

#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzToolsFramework/Entity/EditorEntityContextBus.h>
#include <AzToolsFramework/ToolsComponents/EditorAssetMimeDataContainer.h>
#include <AzToolsFramework/ToolsComponents/ComponentAssetMimeDataContainer.h>
#include <AzToolsFramework/ToolsComponents/ScriptEditorComponent.h>
#include <AzToolsFramework/ToolsComponents/TransformComponent.h>
#include <AzToolsFramework/Commands/EntityStateCommand.h>

#include <QTimer>


///////////////////////////////////////////////////////////////////////////////
// AssetCatalogModelWorkerThread
///////////////////////////////////////////////////////////////////////////////

AssetCatalogModelWorkerThread::AssetCatalogModelWorkerThread(AssetCatalogModel* catalog, QThread* returnThread)
    : m_catalog(catalog)
    , m_returnThread(returnThread)
{
    connect(this, &QThread::started, this, &AssetCatalogModelWorkerThread::startJob);
    connect(m_catalog, &AssetCatalogModel::LoadComplete, this, &AssetCatalogModelWorkerThread::ReturnToThread);
}

void AssetCatalogModelWorkerThread::ReturnToThread()
{
    quit();
}

void AssetCatalogModelWorkerThread::startJob()
{
    disconnect(this, &QThread::started, this, &AssetCatalogModelWorkerThread::startJob);
    m_catalog->StartProcessingAssets();
    QTimer::singleShot(0, m_catalog, &AssetCatalogModel::ProcessAssets);
}

void AssetCatalogModelWorkerThread::run()
{
    exec();

    disconnect(m_catalog, &AssetCatalogModel::LoadComplete, this, &AssetCatalogModelWorkerThread::ReturnToThread);
    m_catalog->moveToThread(m_returnThread);
}


///////////////////////////////////////////////////////////////////////////////
// AssetCatalogEntry
///////////////////////////////////////////////////////////////////////////////
bool AssetCatalogEntry::operator<(const QStandardItem& other) const
{
    //  Set directories as always less than files.
    bool leftIsDir = data(FolderRole).toBool();
    bool rightIsDir = other.data(FolderRole).toBool();

    if (leftIsDir != rightIsDir)
    {
        return leftIsDir;
    }

    QVariant leftName = data(Qt::DisplayRole);
    QVariant rightName = other.data(Qt::DisplayRole);

    return leftName.toString().compare(rightName.toString(), Qt::CaseInsensitive) < 0;
}

///////////////////////////////////////////////////////////////////////////////
// AssetCatalogModel
///////////////////////////////////////////////////////////////////////////////
AssetCatalogModel::AssetCatalogModel(QObject* parent)
    : QStandardItemModel(parent)
    , m_canProcessAssets(true)
{
    AZStd::string allExtensions;
    AZStd::vector<AZ::Data::AssetType> assetTypes;
    //  Discover all types that the Asset system recognizes.
    //  Create a one-to-many map that associates extensions with AssetTypes.
    EBUS_EVENT(AZ::Data::AssetCatalogRequestBus, GetHandledAssetTypes, assetTypes);
    for (auto type : assetTypes)
    {
        AZStd::vector<AZStd::string> extensions;
        allExtensions.clear();
        EBUS_EVENT_ID(type, AZ::AssetTypeInfoBus, GetAssetTypeExtensions, extensions);
        for (int i = 0; i < extensions.size(); i++)
        {
            if (i > 0)
            {
                allExtensions += ";";
            }
            allExtensions += ".";   //  Adding dots to all extensions to be able to separate full extensions from substrings, i.e. "bin" and input"bin"dings.
            allExtensions += extensions[i].c_str();
        }
        if (!allExtensions.empty())
        {
            auto existingEntry = m_extensionToAssetType.find(allExtensions);
            if (existingEntry != m_extensionToAssetType.end())
            {
                existingEntry->second.push_back(type);
            }
            else
            {
                m_extensionToAssetType.insert(AZStd::make_pair(allExtensions, AZStd::vector<AZ::Uuid> {type}));
            }
        }
    }

    //  Special cases for SimpleAssets. If these get full-fledged AssetData types, these cases can be removed.
    QString textureExtensions = LmbrCentral::TextureAsset::GetFileFilter();
    m_extensionToAssetType.insert(AZStd::make_pair(textureExtensions.replace("*", "").replace(" ", "").toStdString().c_str(), AZStd::vector<AZ::Uuid> { AZ::AzTypeInfo<LmbrCentral::TextureAsset>::Uuid() }));
    QString materialExtensions = LmbrCentral::MaterialAsset::GetFileFilter();
    m_extensionToAssetType.insert(AZStd::make_pair(materialExtensions.replace("*", "").replace(" ", "").toStdString().c_str(), AZStd::vector<AZ::Uuid> { AZ::AzTypeInfo<LmbrCentral::MaterialAsset>::Uuid() }));
    QString dccMaterialExtensions = LmbrCentral::DccMaterialAsset::GetFileFilter();
    m_extensionToAssetType.insert(AZStd::make_pair(dccMaterialExtensions.replace("*", "").replace(" ", "").toStdString().c_str(), AZStd::vector<AZ::Uuid> { AZ::AzTypeInfo<LmbrCentral::DccMaterialAsset>::Uuid() }));

    AZ::SerializeContext* serializeContext = nullptr;
    EBUS_EVENT_RESULT(serializeContext, AZ::ComponentApplicationBus, GetSerializeContext);
    AZ_Assert(serializeContext, "Failed to acquire application serialize context.");

    serializeContext->EnumerateDerived<AZ::Component>([this](const AZ::SerializeContext::ClassData* classData, const AZ::Uuid&) -> bool
        {
            if (classData->m_editData)
            {
                AZ::Data::AssetType assetType;
                const AZ::Edit::ElementData* element = classData->m_editData->FindElementData(AZ::Edit::ClassElements::EditorData);
                if (element)
                {
                    const AZ::Edit::Attribute* assetTypeAttribute = element->FindAttribute(AZ::Edit::Attributes::PrimaryAssetType);
                    if (assetTypeAttribute)
                    {
                        auto* assetTypeData = azdynamic_cast<const AZ::Edit::AttributeData<AZ::Uuid>*>(assetTypeAttribute);
                        if (assetTypeData)
                        {
                            assetType = assetTypeData->Get(nullptr);
                            m_assetTypeToComponent[assetType] = classData->m_azRtti->GetTypeId();
                        }
                    }
                    else
                    {
                        assetType = AZ::Data::AssetType::CreateNull();
                    }

                    if (!assetType.IsNull())
                    {
                        const AZ::Edit::Attribute* iconAttribute = element->FindAttribute(AZ_CRC("Icon"));
                        if (iconAttribute)
                        {
                            auto* iconAttributeData = azdynamic_cast<const AZ::Edit::AttributeData<const char*>*>(iconAttribute);
                            if (iconAttributeData)
                            {
                                QIcon icon(iconAttributeData->Get(nullptr));
                                if (!icon.isNull())
                                {
                                    m_assetTypeToIcon[assetType] = icon;
                                }
                            }
                        }
                    }
                }
            }
            return true;
        });
}
AssetCatalogModel::~AssetCatalogModel()
{
    AzFramework::AssetCatalogEventBus::Handler::BusDisconnect();
}

AZ::Data::AssetType AssetCatalogModel::GetAssetType(const QString &filename) const
{

    //  Compare file extensions with the map created from the asset database.
    int dotIndex = filename.lastIndexOf('.');
    if (dotIndex < 0)
    {
        return AZ::Uuid::CreateNull();
    }

    QStringRef extension = filename.midRef(dotIndex);
    for (const auto& pair : m_extensionToAssetType)
    {
        QString qExtensions = pair.first.c_str();
        if (qExtensions.indexOf(extension) < 0 || pair.second.empty())
        {
            continue;
        }
        if (pair.second.size() == 1)
        {
            return pair.second[0];
        }

        //  There are multiple types with this extension. Search for a handler that can handle this data type.
        AZStd::string azFilename = filename.toStdString().c_str();
        EBUS_EVENT(AzFramework::ApplicationRequests::Bus, MakePathAssetRootRelative, azFilename);
        AZ::Data::AssetId assetId;
        EBUS_EVENT_RESULT(assetId, AZ::Data::AssetCatalogRequestBus, GetAssetIdByPath, azFilename.c_str(), AZ::Data::s_invalidAssetType, false);

        for (const AZ::Uuid& type : pair.second)
        {
            const AZ::Data::AssetHandler* handler = AZ::Data::AssetManager::Instance().GetHandler(type);
            if (handler && handler->CanHandleAsset(assetId))
            {
                return type;
            }
        }
    }

    return AZ::Uuid::CreateNull();
}

QStandardItem* AssetCatalogModel::GetPath(QString& path, bool createIfNeeded, QStandardItem* parent)
{
    if (!parent)
    {
        parent = invisibleRootItem();
    }
    QString cleanPath = path.replace("\\", "/");
    while (cleanPath.startsWith("/"))
    {
        cleanPath = cleanPath.mid(1);
    }
    while (cleanPath.endsWith("/"))
    {
        cleanPath.chop(1);
    }

    QString currentFolder;
    QString restOfPath;

    int slashIdx = cleanPath.indexOf('/', 1);
    if (slashIdx < 0)
    {
        currentFolder = cleanPath;
        restOfPath.clear();
    }
    else
    {
        currentFolder = cleanPath.left(slashIdx);
        restOfPath = cleanPath.mid(slashIdx + 1);
    }

    if (currentFolder.isEmpty())
    {
        return parent;
    }

    for (int i = 0; i < parent->rowCount(); i++)
    {
        QString name = parent->child(i)->data(Qt::DisplayRole).toString();
        bool isFolder = parent->child(i)->data(AssetCatalogEntry::FolderRole).toBool();

        if (currentFolder == name && isFolder)
        {
            if (restOfPath.isEmpty())
            {
                return parent->child(i);
            }
            else
            {
                return GetPath(restOfPath, createIfNeeded, parent->child(i));
            }
        }
    }

    if (createIfNeeded)
    {
        QString fullpath = parent->data(AssetCatalogEntry::FilePathRole).toString();
        fullpath += currentFolder + "/";
        AssetCatalogEntry* folder = new AssetCatalogEntry();
        folder->setData(currentFolder, Qt::DisplayRole);
        folder->setData(fullpath, AssetCatalogEntry::FilePathRole);
        folder->setData(true, AssetCatalogEntry::FolderRole);
        folder->setData(true, AssetCatalogEntry::VisibilityRole);

        parent->appendRow(folder);

        if (restOfPath.isEmpty())
        {
            return folder;
        }
        else
        {
            return GetPath(restOfPath, createIfNeeded, folder);
        }
    }
    else
    {
        return nullptr;
    }
}

AssetCatalogEntry* AssetCatalogModel::FindAsset(QString assetPath)
{
    QString path;
    QString asset;

    //  Separate file name and folder name.
    int slashIdx = assetPath.lastIndexOf('/');
    if (slashIdx < 0)
    {
        asset = assetPath;
        path.clear();
    }
    else
    {
        path = assetPath.left(slashIdx);
        asset = assetPath.mid(slashIdx + 1);
    }

    QStandardItem* folder = GetPath(path, false);

    if (folder)
    {
        for (int i = 0; i < folder->rowCount(); i++)
        {
            QString name = folder->child(i)->data(Qt::DisplayRole).toString();
            if (name == asset)
            {
                AssetCatalogEntry* entry = static_cast<AssetCatalogEntry*>(folder->child(i));
                return entry;
            }
        }
    }

    return nullptr;
}

AssetCatalogEntry* AssetCatalogModel::AddAsset(QString assetPath, AZ::Data::AssetId id)
{
    QString path;
    QString asset;

    //  Separate file name and folder name.
    int slashIdx = assetPath.lastIndexOf('/');
    if (slashIdx < 0)
    {
        asset = assetPath;
        path.clear();
    }
    else
    {
        path = assetPath.left(slashIdx);
        asset = assetPath.mid(slashIdx + 1);
    }

    QRegExp mipMapExtension("\\.dds\\.\\d+a?$");    // Files that end with ".dds.#", with an optional "a"
    if (asset.contains(mipMapExtension))
    {
        //  Mip map files should be ignored by the file browser.
        //  This is a temporary solution until texture streams are refactored.
        return nullptr;
    }

    QStandardItem* folder = GetPath(path, true);

    QString fullPath = folder->data(AssetCatalogEntry::FilePathRole).toString() + asset;
    AZ::Data::AssetType assetType = GetAssetType(fullPath);
    AZ::Uuid            classId = AZ::Uuid::CreateNull();
    auto it = m_assetTypeToComponent.find(assetType);
    if (it != m_assetTypeToComponent.end())
    {
        classId = it->second;
    }

    AssetCatalogEntry* entry = new AssetCatalogEntry();
    entry->setData(asset, Qt::DisplayRole);
    entry->setData(fullPath, AssetCatalogEntry::FilePathRole);
    entry->setData(false, AssetCatalogEntry::FolderRole);
    entry->setData(true, AssetCatalogEntry::VisibilityRole);

    entry->m_assetId = id;
    entry->m_assetType = assetType;
    entry->m_classId = classId;

    if (!assetType.IsNull())
    {
        auto iconIt = m_assetTypeToIcon.find(assetType);
        if (iconIt == m_assetTypeToIcon.end())
        {
            //  The m_assetTypeToIcon map was seeded with icons for known asset types.
            //  If we come across an asset type that is not associated with a component,
            //  we'll get its icon from OS if we can. This will help users recognize files more easily.
            QFileInfo fileInfo(m_rootPath + fullPath);
            QIcon fileIcon = m_iconProvider.icon(fileInfo);
            //  Now, make a deep copy for OS-provided icons. On Windows 10, there seems to be an issue with
            //  icons' memory being reclaimed and crashing the Editor.
            QSize size = fileIcon.actualSize(QSize(16, 16));
            QIcon deepCopy = fileIcon.pixmap(size).copy(0, 0, size.width(), size.height());

            if (!fileIcon.isNull())
            {
                m_assetTypeToIcon[assetType] = deepCopy;
            }
        }
    }

    folder->appendRow(entry);

    return entry;
}

AssetCatalogEntry* AssetCatalogModel::RemoveAsset(QString assetPath)
{
    AssetCatalogEntry* entry = FindAsset(assetPath);
    if (entry)
    {
        QStandardItem* parent = entry->parent();
        if (parent)
        {
            parent->removeRow(entry->row());
            AssetCatalogEntry* folder = static_cast<AssetCatalogEntry*>(parent);
            return folder;
        }
    }

    return nullptr;
}

void AssetCatalogModel::LoadDatabase()
{
    clear();

    AZStd::string assetRootFolder;
    if (auto settingsRegistry = AZ::SettingsRegistry::Get(); settingsRegistry != nullptr)
    {
        settingsRegistry->Get(assetRootFolder, AZ::SettingsRegistryMergeUtils::FilePathKey_CacheRootFolder);
    }
    m_rootPath = assetRootFolder.c_str();

    auto startCB = []() {};

    auto enumerateCB = [this](const AZ::Data::AssetId id, const AZ::Data::AssetInfo& assetInfo)
        {
            DatabaseEntry* entry = new DatabaseEntry(id, assetInfo.m_relativePath.c_str());
            m_fileCache.push_back(entry);
        };

    auto endCB = [this]()
        {
            m_fileCacheCurrentIndex = 0;
            Q_EMIT UpdateProgress(0);
            Q_EMIT SetTotalProgress(static_cast<int>(m_fileCache.size()));
        };

    EBUS_EVENT(AZ::Data::AssetCatalogRequestBus, EnumerateAssets, startCB, enumerateCB, endCB);

    AzFramework::AssetCatalogEventBus::Handler::BusConnect();

    m_canProcessAssets = true;
}

void AssetCatalogModel::ProcessAssets()
{
    if (m_fileCacheCurrentIndex >= m_fileCache.size())
    {
        sort(0);
        m_fileCache.clear();
        Q_EMIT LoadComplete();
    }
    else
    {
        for (int i = 0; m_canProcessAssets && i < ASSET_CATALOG_BATCH_SIZE && m_fileCacheCurrentIndex < m_fileCache.size(); i++, m_fileCacheCurrentIndex++)
        {
            AddAsset(m_fileCache[m_fileCacheCurrentIndex]->m_path, m_fileCache[m_fileCacheCurrentIndex]->m_id);
        }
        Q_EMIT UpdateProgress(m_fileCacheCurrentIndex);

        if (m_canProcessAssets)
        {
            QTimer::singleShot(1, this, &AssetCatalogModel::ProcessAssets);
        }
    }
}

void AssetCatalogModel::StartProcessingAssets()
{
    m_canProcessAssets = true;
}

void AssetCatalogModel::StopProcessingAssets()
{
    m_canProcessAssets = false;
}


void AssetCatalogModel::OnCatalogAssetAdded(const AZ::Data::AssetId& assetId)
{
    AZ::Data::AssetInfo assetInfo;
    EBUS_EVENT_RESULT(assetInfo, AZ::Data::AssetCatalogRequestBus, GetAssetInfoById, assetId);

    // note that this will get called twice, once with the real assetId and once with legacy assetId.
    // we only want to add the real asset to the list, in which the assetId passed in is equal to the final assetId returned
    // otherwise, you look up assetId (and its a legacy assetId) and the actual asset will be different.
    if ((assetInfo.m_assetId.IsValid()) && (assetInfo.m_assetId == assetId))
    {
        AssetCatalogEntry* asset = AddAsset(assetInfo.m_relativePath.c_str(), assetInfo.m_assetId);
        if (asset)
        {
            Q_EMIT itemChanged(asset);
        }
    }
}

void AssetCatalogModel::OnCatalogAssetRemoved(const AZ::Data::AssetId& /*assetId*/, const AZ::Data::AssetInfo& assetInfo)
{
    AssetCatalogEntry* asset = RemoveAsset(assetInfo.m_relativePath.c_str());
    if (asset)
    {
        Q_EMIT itemChanged(asset);
    }
}

QVariant AssetCatalogModel::data(const QModelIndex& index, int role) const
{
    QStandardItem* item = itemFromIndex(index);
    if (item && role == Qt::DecorationRole)
    {
        AssetCatalogEntry* entry = static_cast<AssetCatalogEntry*>(item);
        auto it = m_assetTypeToIcon.find(entry->m_assetType);
        if (it != m_assetTypeToIcon.end())
        {
            return it->second;
        }

        bool isFolder = item->data(AssetCatalogEntry::FolderRole).toBool();
        return isFolder ? m_iconProvider.icon(QFileIconProvider::Folder) : m_iconProvider.icon(QFileIconProvider::File);
    }

    return QStandardItemModel::data(index, role);
}

QMimeData* AssetCatalogModel::mimeData(const QModelIndexList& indexes) const
{
    AssetCatalogEntry* item = static_cast<AssetCatalogEntry*>(itemFromIndex(indexes[0]));
    bool isFolder = item ? item->data(AssetCatalogEntry::FolderRole).toBool() : true;

    if (isFolder)
    {
        return new QMimeData();
    }

    QString fullPath = item->data(AssetCatalogEntry::FilePathRole).toString();
    QMimeData* mimeData = new QMimeData;

    if (!item->m_assetType.IsNull() && item->m_assetId.IsValid())
    {
        //  This mime data is used to drag into PropertyAssetCtrl fields.
        AzToolsFramework::EditorAssetMimeDataContainer mimeDataContainer;
        mimeDataContainer.AddEditorAsset(item->m_assetId, item->m_assetType);
        mimeDataContainer.AddToMimeData(mimeData);

        // This mime data is used for spawning of entities with components and the adding of components through assets.
        AzToolsFramework::ComponentAssetMimeDataContainer componentContainer;
        componentContainer.AddComponentAsset(item->m_classId, item->m_assetId);
        componentContainer.AddToMimeData(mimeData);
    }
    //  Also, add the filename, for untyped fields.
    QList<QUrl> urls;
    urls << QUrl::fromLocalFile(fullPath);
    mimeData->setUrls(urls);

    return mimeData;
}

QVariant AssetCatalogModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role == Qt::DisplayRole && section == 0 && orientation == Qt::Horizontal)
    {
        return tr("Assets");
    }

    return QAbstractItemModel::headerData(section, orientation, role);
}

void AssetCatalogModel::SearchCriteriaChanged(QStringList& criteriaList, AzToolsFramework::FilterOperatorType filterOperator)
{
    BuildFilter(criteriaList, filterOperator);
    InvalidateFilter();
}

void AssetCatalogModel::BuildFilter(QStringList& criteriaList, AzToolsFramework::FilterOperatorType filterOperator)
{
    ClearFilterRegExp();

    if (criteriaList.size() > 0)
    {
        QString filter, tag, text;
        for (int i = 0; i < criteriaList.size(); i++)
        {
            AzToolsFramework::SearchCriteriaButton::SplitTagAndText(criteriaList[i], tag, text);
            if (tag.isEmpty())
            {
                tag = "null";
            }

            filter = m_filtersRegExp[tag.toStdString().c_str()].pattern();

            if (filterOperator == AzToolsFramework::FilterOperatorType::Or)
            {
                if (filter.isEmpty())
                {
                    filter = text;
                }
                else
                {
                    filter += "|" + text;
                }
            }
            else if (filterOperator == AzToolsFramework::FilterOperatorType::And)
            {
                filter += "(?=.*" + text + ")"; //  Using Lookaheads to produce an "and" effect.
            }

            SetFilterRegExp(tag.toStdString().c_str(), QRegExp(filter, Qt::CaseInsensitive));
        }
    }
}

void AssetCatalogModel::SetFilterRegExp(const AZStd::string& filterType, const QRegExp& regExp)
{
    m_filtersRegExp[filterType] = regExp;
}

void AssetCatalogModel::ClearFilterRegExp(const AZStd::string& filterType)
{
    if (filterType.empty())
    {
        for (auto& it : m_filtersRegExp)
        {
            it.second = QRegExp();
        }
    }
    else
    {
        m_filtersRegExp[filterType] = QRegExp();
    }
}

void AssetCatalogModel::InvalidateFilter()
{
    ApplyFilter(invisibleRootItem());
}

void AssetCatalogModel::ApplyFilter(QStandardItem* parent)
{
    //  Set the visibility as a breadth-first search of the tree.
    //  This will allow us to also set our parents visible if we are visible
    //  without a later search overriding us.
    for (int i = 0; i < parent->rowCount(); i++)
    {
        QStandardItem* child = parent->child(i);
        if (m_filtersRegExp["name"].isEmpty())
        {
            child->setData(true, AssetCatalogEntry::VisibilityRole);
        }
        else
        {
            QString assetname = child->data(Qt::DisplayRole).toString();
            bool matchesFilter = assetname.contains(m_filtersRegExp["name"]);
            child->setData(matchesFilter, AssetCatalogEntry::VisibilityRole);

            if (matchesFilter)
            {
                //  Set all parents to visible.
                QStandardItem* visiblityParent = parent;
                bool isVisible = visiblityParent->data(AssetCatalogEntry::VisibilityRole).toBool();
                while (!isVisible)    //  Checking isVisible gives us a short circuit for already visible folders.
                {
                    visiblityParent->setData(true, AssetCatalogEntry::VisibilityRole);
                    visiblityParent = visiblityParent->parent();

                    isVisible = visiblityParent ? visiblityParent->data(AssetCatalogEntry::VisibilityRole).toBool() : true;
                }
            }
        }
    }

    //  Recurse through the children that are folders
    for (int i = 0; i < parent->rowCount(); i++)
    {
        QStandardItem* child = parent->child(i);
        bool isFolder = child->data(AssetCatalogEntry::FolderRole).toBool();
        if (isFolder)
        {
            ApplyFilter(child);
        }
    }
}

QString AssetCatalogModel::FileName(const QModelIndex& index) const
{
    QStandardItem* item = itemFromIndex(index);
    if (item)
    {
        return item->data(Qt::DisplayRole).toString();
    }

    return QString();
}

QString AssetCatalogModel::FilePath(const QModelIndex& index) const
{
    //  filePath contains the name of the file.
    QStandardItem* item = itemFromIndex(index);
    if (item)
    {
        QString fullPath = RootPath();
        fullPath += item->data(AssetCatalogEntry::FilePathRole).toString();
        return fullPath;
    }

    return QString();
}

AssetCatalogEntry* AssetCatalogModel::AssetData(const QModelIndex& index) const
{
    return static_cast<AssetCatalogEntry*>(itemFromIndex(index));
}

///////////////////////////////////////////////////////////////////////////////
// End of context menu handling
///////////////////////////////////////////////////////////////////////////////

#include <UI/moc_AssetCatalogModel.cpp>

