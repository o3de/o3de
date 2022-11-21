/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "FavoriteDataModel.h"
#include "ComponentSliceFavoritesWindow.h"

#include <AzCore/Asset/AssetManager.h>
#include <AzCore/IO/SystemFile.h>
#include <AzCore/Slice/SliceAsset.h>
#include <AzCore/XML/rapidxml_print.h>
#include <AzCore/Asset/AssetManager.h>
#include <AzCore/Utils/Utils.h>

#include <AzFramework/Asset/AssetSystemComponent.h>
#include <AzFramework/StringFunc/StringFunc.h>

#include <AzQtComponents/DragAndDrop/ViewportDragAndDrop.h>

#include <AzToolsFramework/API/ViewPaneOptions.h>
#include <AzToolsFramework/AssetBrowser/AssetBrowserEntry.h>
#include <AzToolsFramework/AssetBrowser/Entries/AssetBrowserEntryUtils.h>
#include <AzToolsFramework/Entity/EditorEntityContextBus.h>
#include <AzToolsFramework/Entity/SliceEditorEntityOwnershipServiceBus.h>
#include <AzToolsFramework/ToolsComponents/TransformComponent.h>

AZ_PUSH_DISABLE_WARNING(4251 4244, "-Wunknown-warning-option") // qevent.h(197): warning C4244: 'return': conversion from 'qreal' to 'int', possible loss of data 
#include <QDragEnterEvent>
AZ_POP_DISABLE_WARNING
#include <QDragLeaveEvent>
#include <QDragMoveEvent>
#include <QDropEvent>
#include <QMimeData>
#include <QMessageBox>

namespace SliceFavorites
{
    //  XML file format strings
    static const char RootXMLTag[] = "SliceFavorites";
    static const char FavoriteDataXMLTag[] = "FavoriteData";
    static const char NameXMLTag[] = "FavoriteDataName";
    static const char TypeXMLTag[] = "FavoriteDataType";
    static const char SubTypeXMLTag[] = "FavoriteDataSubType";
    static const char AssetIdXMLTag[] = "FavoriteDataAssetId";

    static const char* const ManageSliceFavorites = "Slice Favorites";

    FavoriteData::~FavoriteData()
    {
        Reset();
    }

    bool FavoriteData::IsAssetUnique(AZ::Data::AssetId assetId, const FavoriteData* root)
    {
        if (root->m_assetId == assetId)
        {
            return false;
        }

        for (auto favoriteData : root->m_children)
        {
            if (favoriteData->m_assetId == assetId || !IsAssetUnique(assetId, favoriteData))
            {
                return false;
            }
        }
        return true;
    }

    int FavoriteData::LoadFromXML(AZ::rapidxml::xml_node<char>* node, const FavoriteData* root)
    {
        int numFavoritesLoaded = 0;

        for (AZ::rapidxml::xml_node<char>* childNode = node->first_node(); childNode; childNode = childNode->next_sibling())
        {
            if (!azstricmp(childNode->name(), FavoriteDataXMLTag))
            {
                // We have a child!
                FavoriteData* childData = new FavoriteData();
                int numLoaded = childData->LoadFromXML(childNode, root);
                if (numLoaded)
                {
                    numFavoritesLoaded += numLoaded;
                    m_children.push_back(childData);
                }
            }
            else if (!azstricmp(childNode->name(), NameXMLTag))
            {
                m_name = childNode->value();
            }
            else if (!azstricmp(childNode->name(), TypeXMLTag))
            {
                m_type = static_cast<FavoriteData::FavoriteType>(atoi(childNode->value()));
            }
            else if (!azstricmp(childNode->name(), AssetIdXMLTag))
            {
                m_assetId = AZ::Data::AssetId::CreateString(childNode->value());
                if (IsAssetUnique(m_assetId, root))
                {
                    ++numFavoritesLoaded;
                }
            }
            else if (!azstricmp(childNode->name(), SubTypeXMLTag))
            {
                m_subType = static_cast<FavoriteData::FavoriteSubType>(atoi(childNode->value()));
            }
        }

        return numFavoritesLoaded;
    }

    int FavoriteData::AddToXML(AZ::rapidxml::xml_node<char>* node, AZ::rapidxml::xml_document<char>* xmlDoc) const
    {
        // If we don't have a name, then don't include us as a favorite added to the xml
        int numFavoritesAdded = (m_name.size() > 0 ? 1 : 0);

        char* value = xmlDoc->allocate_string(m_name.toStdString().c_str());
        AZ::rapidxml::xml_node<char>* nameNode = xmlDoc->allocate_node(AZ::rapidxml::node_element, NameXMLTag, value);
        node->append_node(nameNode);
        
        AZStd::string typeString = AZStd::string::format("%d", (int)m_type);
        value = xmlDoc->allocate_string(typeString.c_str());
        AZ::rapidxml::xml_node<char>* typeNode = xmlDoc->allocate_node(AZ::rapidxml::node_element, TypeXMLTag, value);
        node->append_node(typeNode);

        AZStd::string subTypeString = AZStd::string::format("%d", (int)m_subType);
        value = xmlDoc->allocate_string(subTypeString.c_str());
        AZ::rapidxml::xml_node<char>* subTypeNode = xmlDoc->allocate_node(AZ::rapidxml::node_element, SubTypeXMLTag, value);
        node->append_node(subTypeNode);

        AZStd::string assetIdString;
        m_assetId.ToString(assetIdString);
        value = xmlDoc->allocate_string(assetIdString.c_str());
        AZ::rapidxml::xml_node<char>* assetIdNode = xmlDoc->allocate_node(AZ::rapidxml::node_element, AssetIdXMLTag, value);
        node->append_node(assetIdNode);

        // For each child in m_rootItem, recursively write out the favorite data
        for (const FavoriteData* data : m_children)
        {
            AZ::rapidxml::xml_node<char>* favoriteNode = xmlDoc->allocate_node(AZ::rapidxml::node_element, FavoriteDataXMLTag, "");
            numFavoritesAdded += data->AddToXML(favoriteNode, xmlDoc);
            node->append_node(favoriteNode);
        }

        return numFavoritesAdded;
    }

    void FavoriteData::Reset()
    {
        qDeleteAll(m_children);
    }

    void FavoriteData::appendChild(FavoriteData* child)
    {
        m_children.push_back(child);
    }

    FavoriteData* FavoriteData::child(int row)
    {
        if (row < m_children.size())
        {
            return m_children[row];
        }

        return nullptr;
    }

    int FavoriteData::childCount() const
    {
        return m_children.size();
    }

    int FavoriteData::columnCount() const
    {
        return 1;
    }

    int FavoriteData::row() const
    {
        if (m_parent)
        {
            return m_parent->m_children.indexOf(const_cast<FavoriteData*>(this));
        }

        return 0;
    }

    QVariant FavoriteData::data(int role) const
    {
        switch (role)
        {
            case Qt::DecorationRole:
            {
                if (m_type == DataType_Folder)
                {
                    return QIcon(":/Icons/SliceFavorite_Icon_Folder");
                }
                else if (m_type == DataType_Favorite)
                {
                    if (m_subType == DynamicSlice)
                    {
                        return QIcon(":Icons/SliceFavorite_Icon_DynamicFavorite");
                    }
                    else
                    {
                        return QIcon(":/Icons/SliceFavorite_Icon_Favorite");
                    }
                }
                break;
            }
            case Qt::DisplayRole:
            {
                return m_name;
            }
            case Qt::ToolTipRole:
            {
                return GenerateTooltip();
            }
        }

        return QVariant();
    }

    QString FavoriteData::GenerateTooltip() const
    {
        if (m_type == DataType_Favorite)
        {
            using namespace AzToolsFramework::AssetBrowser;
            ProductAssetBrowserEntry* product = ProductAssetBrowserEntry::GetProductByAssetId(m_assetId);
            if (product)
            {
                return QObject::tr(product->GetRelativePath().c_str());
            }
            else
            {
                return QObject::tr("<slice not found>");
            }
        }

        return QObject::tr("");
    }

    int FavoriteData::GetNumFoldersInHierarchy() const
    {
        return GetNumOfType(DataType_Folder);
    }

    int FavoriteData::GetNumFavoritesInHierarchy() const
    {
        return GetNumOfType(DataType_Favorite);

    }

    int FavoriteData::GetNumOfType(FavoriteType type) const
    {
        int numOfType = 0;

        if (m_type == type)
        {
            numOfType++;
        }

        for (FavoriteData* child : m_children)
        {
            numOfType += child->GetNumOfType(type);
        }

        return numOfType;
    }

    ////////////////////////////////////////////////////////////////////////////
    ////////////////////////////////////////////////////////////////////////////
    FavoriteDataModel::FavoriteDataModel([[maybe_unused]] QObject *parent)
    {
        m_rootItem = AZStd::make_unique<FavoriteData>(FavoriteData::DataType_Folder);
        m_favoritesMenu = AZStd::make_unique<QMenu>(QObject::tr("Slice favorites"));

        AzToolsFramework::EditorEvents::Bus::Handler::BusConnect();
        AzToolsFramework::AssetBrowser::AssetBrowserInteractionNotificationBus::Handler::BusConnect();
        AzFramework::AssetCatalogEventBus::Handler::BusConnect();
        AzQtComponents::DragAndDropEventsBus::Handler::BusConnect(AzQtComponents::DragAndDropContexts::EditorViewport);
        AzToolsFramework::AssetBrowser::AssetBrowserComponentNotificationBus::Handler::BusConnect();
    }

    FavoriteDataModel::~FavoriteDataModel()
    {
        AzToolsFramework::EditorEvents::Bus::Handler::BusDisconnect();
        AzToolsFramework::AssetBrowser::AssetBrowserInteractionNotificationBus::Handler::BusDisconnect();
        AzFramework::AssetCatalogEventBus::Handler::BusDisconnect();
        AzQtComponents::DragAndDropEventsBus::Handler::BusDisconnect();
        AzToolsFramework::AssetBrowser::AssetBrowserComponentNotificationBus::Handler::BusDisconnect();

        AzToolsFramework::UnregisterViewPane(SliceFavorites::ManageSliceFavorites);
    }

    QVariant FavoriteDataModel::data(const QModelIndex &index, int role) const
    {
        if (index.isValid())
        {
            FavoriteData* item = GetFavoriteDataFromModelIndex(index);
            if (item)
            {
                return item->data(role);
            }
        }

        return QVariant();
    }
    
    Qt::ItemFlags FavoriteDataModel::flags(const QModelIndex &index) const
    {
        if (!index.isValid())
        {
            return  Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsDropEnabled | Qt::ItemIsDragEnabled;
        }

        return QAbstractItemModel::flags(index) | Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsDropEnabled | Qt::ItemIsDragEnabled | Qt::ItemIsEditable;;
    }

    QVariant FavoriteDataModel::headerData(int section, Qt::Orientation orientation, int role) const
    {
        if (orientation == Qt::Horizontal && role == Qt::DisplayRole)
        {
            if (!m_rootItem)
            {
                return QVariant();
            }

            return m_rootItem->data(section);
        }

        return QVariant();
    }

    QModelIndex FavoriteDataModel::index(int row, int column, const QModelIndex &parent) const
    {
        FavoriteData* parentItem;

        if (!parent.isValid())
        {
            parentItem = m_rootItem.get();
        }
        else
        {
            parentItem = GetFavoriteDataFromModelIndex(parent);
        }

        if (!parentItem)
        {
            return QModelIndex();
        }

        FavoriteData* childItem = parentItem->child(row);
        if (childItem)
        {
            return createIndex(row, column, childItem);
        }
        else
        {
            return QModelIndex();
        }
    }

    QModelIndex FavoriteDataModel::parent(const QModelIndex &index) const
    {
        if (!index.isValid())
        {
            return QModelIndex();
        }

        FavoriteData* childItem = GetFavoriteDataFromModelIndex(index);
        FavoriteData* parentItem = childItem ? childItem->parentItem() : m_rootItem.get();

        if (parentItem == m_rootItem.get())
        {
            return QModelIndex();
        }

        return createIndex(parentItem->row(), 0, parentItem);
    }

    int FavoriteDataModel::rowCount(const QModelIndex &parent) const
    {
        FavoriteData* parentItem;
        if (parent.column() > 0)
        {
            return 0;
        }

        if (!parent.isValid())
        {
            parentItem = m_rootItem.get();
        }
        else
        {
            parentItem = GetFavoriteDataFromModelIndex(parent);
        }

        if (!parentItem)
        {
            return 0;
        }

        return parentItem->childCount();
    }

    int FavoriteDataModel::columnCount([[maybe_unused]] const QModelIndex &parent) const
    {
        return 1;
    }

    void FavoriteDataModel::LoadFavorites()
    {
        if (!m_rootItem)
        {
            return;
        }

        beginResetModel();

        m_rootItem->m_children.clear();
        m_favoriteMap.clear();

        QSettings settings;
        settings.beginGroup("SliceFavorites");

        QString projectName = GetProjectName();
        if (projectName.length() > 0)
        {
            settings.beginGroup(GetProjectName());
        }

        ReadChildren(settings, m_rootItem->m_children);

        BuildChildToParentMap();

        UpdateFavorites();

        endResetModel();
    }

    void FavoriteDataModel::SaveFavorites()
    {
        if (!m_rootItem)
        {
            return;
        }

        QSettings settings;
        settings.beginGroup("SliceFavorites");

        QString projectName = GetProjectName();
        if (projectName.length() > 0)
        {
            // Clear the group
            settings.beginGroup(projectName);
            settings.remove("");
            settings.endGroup();

            settings.beginGroup(projectName);
        }


        WriteChildren(settings, m_rootItem->m_children);
    }

    void FavoriteDataModel::WriteChildren(QSettings& settings, FavoriteList& currentList)
    {
        settings.beginWriteArray("Children");

        for (size_t index = 0; index < currentList.size(); index++)
        {
            FavoriteData* current = currentList[static_cast<int>(index)];

            if (!current)
            {
                continue;
            }

            settings.setArrayIndex(static_cast<int>(index));
            settings.setValue("name", current->m_name);

            AZStd::string assetIdString;
            current->m_assetId.ToString(assetIdString);
            settings.setValue("assetId", assetIdString.c_str());

            settings.setValue("type", current->m_type);
            settings.setValue("subType", current->m_subType);

            if (current->m_type == FavoriteData::DataType_Folder && current->m_children.size() > 0)
            {
                WriteChildren(settings, current->m_children);
            }
        }

        settings.endArray();
    }

    void FavoriteDataModel::ReadChildren(QSettings& settings, FavoriteList& currentList)
    {
        currentList.clear();

        int size = settings.beginReadArray("Children");

        for (int index = 0; index < size; index++)
        {
            FavoriteData* current = new FavoriteData();

            settings.setArrayIndex(index);

            current->m_name = settings.value("name").toString().toUtf8().data();

            QString assetIdString = settings.value("assetId").toString();
            current->m_assetId = AZ::Data::AssetId::CreateString(assetIdString.toUtf8().data());

            current->m_type = (FavoriteData::FavoriteType)settings.value("type").toInt();
            current->m_subType = (FavoriteData::FavoriteSubType)settings.value("subType").toInt();

            // Check if asset still exists
            AZ::Data::AssetInfo checkAsset;
            AZ::Data::AssetCatalogRequestBus::BroadcastResult(checkAsset, &AZ::Data::AssetCatalogRequests::GetAssetInfoById, current->m_assetId);
            if (current->m_type == FavoriteData::DataType_Favorite)
            {
                if (checkAsset.m_sizeBytes > 0)
                {
                    m_favoriteMap.emplace(current->m_assetId, current);
                    currentList.push_back(current);
                }
            }
            else if (current->m_type == FavoriteData::DataType_Folder)
            {
                ReadChildren(settings, current->m_children);
                currentList.push_back(current);
            }
        }

        settings.endArray();
    }

    void FavoriteDataModel::BuildChildToParentMap()
    {
        UpdateChildren(m_rootItem.get());
    }

    void FavoriteDataModel::UpdateChildren(FavoriteData* parent)
    {
        if (parent)
        {
            for (FavoriteData* data : parent->m_children)
            {
                data->m_parent = parent;
                UpdateChildren(data);
            }
        }
    }

    void FavoriteDataModel::RebuildMenu()
    {
        // Rebuild the menu from the current tree
        m_favoritesMenu->clear();

        m_favoritesMenu->addAction(QIcon(":/Icons/SliceFavorite_Icon_Manage"), "Manage favorites...", m_favoritesMenu.get(), []()
        {
            AzToolsFramework::OpenViewPane(SliceFavorites::ManageSliceFavorites);
        });

        m_favoritesMenu->addSeparator();

        for (FavoriteData* favorite : m_rootItem->m_children)
        {
            // AddFavoriteToMenu will recursively add all favorites/menus to the menu passed in
            AddFavoriteToMenu(favorite, m_favoritesMenu.get());
        }
    }

    void FavoriteDataModel::AddFavoriteToMenu(const FavoriteData* favorite, QMenu* menu)
    {
        if (favorite->m_type == FavoriteData::DataType_Favorite)
        {
            // Only add this option if we have a valid assetId to instantiate
            if (favorite->m_assetId.IsValid())
            {
                const AZ::Data::AssetId& savedAssetId = favorite->m_assetId;
                menu->addAction(favorite->m_name, menu, [&savedAssetId]()
                {
                    AzToolsFramework::EditorRequestBus::Broadcast(&AzToolsFramework::EditorRequestBus::Events::InstantiateSliceFromAssetId, savedAssetId);
                });
            }
        }
        else if (favorite->m_type == FavoriteData::DataType_Folder)
        {
            // If there isn't a separator before this folder and it isn't going to be the first element in the menu, add a separator
            if (menu->actions().size() > 0)
            {
                if (!menu->actions().back()->isSeparator())
                {
                    menu->addSeparator();
                }
            }

            QMenu* newMenu = menu->addMenu(QIcon(":/Icons/SliceFavorite_Icon_Folder"), favorite->m_name);

            menu->addSeparator();

            for (FavoriteData* childFavorite : favorite->m_children)
            {
                AddFavoriteToMenu(childFavorite, newMenu);
            }
        }
    }

    void FavoriteDataModel::RemoveFavorite(const QModelIndex& indexToRemove)
    {
        if (indexToRemove.isValid())
        {
            FavoriteData* dataToRemove = GetFavoriteDataFromModelIndex(indexToRemove);
            RemoveFavorite(dataToRemove);
            delete dataToRemove;

            UpdateFavorites();
        }
    }


    void FavoriteDataModel::RemoveFavorite(const FavoriteData* toRemove)
    {
        if (!toRemove || !toRemove->m_parent)
        {
            return;
        }

        int row = toRemove->row();

        beginRemoveRows(GetModelIndexForParent(toRemove), row, row);

        toRemove->m_parent->m_children.removeAt(row);

        endRemoveRows();

        RemoveFromFavoriteMap(toRemove);
    }

    void FavoriteDataModel::RemoveFromFavoriteMap(const FavoriteData* toRemove, bool removeChildren)
    {
        if (!toRemove)
        {
            return;
        }

        const auto& foundIt = m_favoriteMap.find(toRemove->m_assetId);
        if (foundIt != m_favoriteMap.end())
        {
            m_favoriteMap.erase(foundIt);
        }

        if (removeChildren)
        {
            for (FavoriteData* child : toRemove->m_children)
            {
                RemoveFromFavoriteMap(child, removeChildren);
            }
        }
    }

    void FavoriteDataModel::UpdateFavorites()
    {
        SaveFavorites();
        RebuildMenu();

        emit DataModelChanged();
    }

    bool FavoriteDataModel::IsFavorite(const AzToolsFramework::AssetBrowser::ProductAssetBrowserEntry* product) const
    {
        if (!product)
        {
            return false;
        }

        auto foundIt = m_favoriteMap.find(product->GetAssetId());
        return (foundIt != m_favoriteMap.end());
    }

    void FavoriteDataModel::AddFavorite(const AzToolsFramework::AssetBrowser::ProductAssetBrowserEntry* product, const QModelIndex parent)
    {
        if (!product)
        {
            return;
        }

        if (IsFavorite(product))
        {
            return;
        }

        FavoriteData* parentData = m_rootItem.get();
        QModelIndex parentIndex = QModelIndex();

        if (parent.isValid())
        {
            parentIndex = parent;
            parentData = GetFavoriteDataFromModelIndex(parent);
        }

        if (!parentData)
        {
            return;
        }

        beginInsertRows(parentIndex, parentData->m_children.size(), parentData->m_children.size());

        // These automatically get added at the end of the root list
        AZStd::string fileName;
        AzFramework::StringFunc::Path::GetFileName(product->GetName().c_str(), fileName);
        FavoriteData::FavoriteSubType subType = (product->GetAssetType() == AZ::AzTypeInfo<AZ::DynamicSliceAsset>::Uuid()) ? FavoriteData::FavoriteSubType::DynamicSlice : FavoriteData::FavoriteSubType::Slice;
        FavoriteData* newFavorite = new FavoriteData(QObject::tr(fileName.c_str()), product->GetAssetId(), FavoriteData::FavoriteType::DataType_Favorite, subType);
        newFavorite->m_parent = parentData;
        parentData->m_children.push_back(newFavorite);

        m_favoriteMap.emplace(product->GetAssetId(), newFavorite);

        UpdateFavorites();

        endInsertRows();
    }

    void FavoriteDataModel::ProcessRemovedAssets()
    {
        if (m_removedAssets.empty())
        {
            return;
        }

        for (AZ::Data::AssetId assetId : m_removedAssets)
        {
            RemoveFavorite(assetId);
        }

        m_removedAssets.clear();

        UpdateFavorites();
    }

    void FavoriteDataModel::RemoveFavorite(const AzToolsFramework::AssetBrowser::ProductAssetBrowserEntry* product)
    {
        if (!product)
        {
            return;
        }

        RemoveFavorite(product->GetAssetId());
    }

    QString FavoriteDataModel::GetProjectName()
    {
        AZ::SettingsRegistryInterface::FixedValueString projectName = AZ::Utils::GetProjectName();
        if (!projectName.empty())
        {
            return QString::fromUtf8(projectName.c_str(), aznumeric_cast<int>(projectName.size()));
        }

        return "unknown";
    }

    size_t FavoriteDataModel::GetNumFavorites()
    {
        return m_favoriteMap.size();
    }

    int FavoriteDataModel::GetNumFavoritesAndFolders()
    {
        if (!m_rootItem)
        {
            return 0;
        }

        return m_rootItem->GetNumFavoritesInHierarchy() + m_rootItem->GetNumFoldersInHierarchy();
    }

    void FavoriteDataModel::EnumerateFavorites(const AZStd::function<void(const AZ::Data::AssetId& assetId)>& callback)
    {
        for (auto& favorite : m_favoriteMap)
        {
            callback(favorite.first);
        }
    }

    void FavoriteDataModel::PopulateEditorGlobalContextMenu_SliceSection(QMenu* menu, const AZ::Vector2& /*point*/, int /*flags*/)
    {
        if (!menu)
        {
            return;
        }

        if (menu->children().size() > 0)
        {
            menu->addSeparator();
        }
        menu->addMenu(GetFavoritesMenu());
    }

    void FavoriteDataModel::AddContextMenuActions(QWidget* /*caller*/, QMenu* menu, const AZStd::vector<AzToolsFramework::AssetBrowser::AssetBrowserEntry*>& entries)
    {
        if (!menu)
        {
            return;
        }

        using namespace AzToolsFramework::AssetBrowser;

        if (entries.size() == 0)
        {
            return;
        }

        const ProductAssetBrowserEntry* product = GetSliceProductFromBrowserEntry(entries.front());
        if (product)
        {
            menu->addSeparator();

            if (IsFavorite(product))
            {
                menu->addAction(QString("Remove as slice favorite"), menu, [this, product]()
                {
                    RemoveFavorite(product);
                });
            }
            else
            {
                menu->addAction(QString("Add as slice favorite"), menu, [this, product]()
                {
                    AddFavorite(product);
                });
            }
        }
    }

    const AzToolsFramework::AssetBrowser::ProductAssetBrowserEntry* FavoriteDataModel::GetSliceProductFromBrowserEntry(AzToolsFramework::AssetBrowser::AssetBrowserEntry* entry) const
    {
        if (!entry)
        {
            return nullptr;
        }

        using namespace AzToolsFramework::AssetBrowser;

        const ProductAssetBrowserEntry* product = nullptr;

        if (entry->GetEntryType() == AssetBrowserEntry::AssetEntryType::Source)
        {
            // See if our first entry has a product of the appropriate type
            AZStd::vector<const ProductAssetBrowserEntry*> productChildren;
            entry->GetChildren(productChildren);

            auto entryIt = AZStd::find_if
            (
                productChildren.begin(),
                productChildren.end(),
                [this](const ProductAssetBrowserEntry* entry) -> bool
            {
                return IsSliceAssetType(entry->GetAssetType());
            }
            );

            if (entryIt != productChildren.end())
            {
                product = *entryIt;
            }
        }
        else if (entry->GetEntryType() == AssetBrowserEntry::AssetEntryType::Product)
        {
            const ProductAssetBrowserEntry* productCast = azrtti_cast<const ProductAssetBrowserEntry*>(entry);

            if (productCast && IsSliceAssetType(productCast->GetAssetType()))
            {
                // Entry is the right type, proceed
                product = productCast;
            }
        }

        return product;
    }

    bool FavoriteDataModel::IsSliceAssetType(const AZ::Data::AssetType& type) const
    {
        return ((type == AZ::AzTypeInfo<AZ::SliceAsset>::Uuid()) || (type == AZ::AzTypeInfo<AZ::DynamicSliceAsset>::Uuid()));
    }

    bool FavoriteDataModel::moveRows(const QModelIndex& sourceParent, int sourceRow, int count, const QModelIndex& destinationParent, int destinationChild)
    {
        FavoriteData* sourceData = GetFavoriteDataFromModelIndex(sourceParent);
        FavoriteData* destinationData = GetFavoriteDataFromModelIndex(destinationParent);

        if (!sourceData || !destinationData)
        {
            return false;
        }

        if (destinationData->m_type == FavoriteData::DataType_Favorite)
        {
            destinationData = destinationData->m_parent;
        }

        for (int childIndex = sourceRow + count - 1; childIndex >= sourceRow; childIndex--)
        {
            FavoriteData* moving = sourceData->m_children[childIndex];
            moving->m_parent = destinationData;
            sourceData->m_children.removeAt(childIndex);

            destinationData->m_children.insert(destinationChild, moving);
        }

        UpdateFavorites();

        return true;
    }

    QMimeData* FavoriteDataModel::mimeData(const QModelIndexList& indexes) const
    {
        using namespace AzToolsFramework::AssetBrowser;

        QMimeData* mimeData = new QMimeData;

        QVector<QString> mimeVector;
        AZStd::vector<const AssetBrowserEntry*> entriesFound;

        for (const auto& index : indexes)
        {
            if (index.isValid())
            {
                FavoriteData* item = GetFavoriteDataFromModelIndex(index);
                if (item)
                {
                    quintptr address = (quintptr)item;
                    mimeVector.push_back(QString::number(address));

                    // Only add the product as mime data if there is only one
                    if (item->m_type == FavoriteData::DataType_Favorite && indexes.size() == 1)
                    {
                        ProductAssetBrowserEntry* product = ProductAssetBrowserEntry::GetProductByAssetId(item->m_assetId);

                        if (product)
                        {
                            entriesFound.push_back(product);
                        }
                    }
                }
            }
        }

        // add any entries found to mimeData.  If no entries are found, this won't do anything.
        Utils::ToMimeData(mimeData, entriesFound);

        // add the custom mimedata for favorites
        if (!mimeVector.empty())
        {
            QByteArray buffer;
            QDataStream dataStream(&buffer, QIODevice::WriteOnly);

            dataStream << mimeVector;

            mimeData->setData(FavoriteData::GetMimeType(), buffer);
        }
        return mimeData;
    }

    QStringList FavoriteDataModel::mimeTypes() const
    {
        QStringList list = QAbstractItemModel::mimeTypes();
        list.append(FavoriteData::GetMimeType());
        return list;
    }

    bool FavoriteDataModel::dropMimeData(const QMimeData* data, Qt::DropAction action, int row, [[maybe_unused]] int column, const QModelIndex& parent)
    {
        if (!data)
        {
            return true;
        }

        if (action == Qt::IgnoreAction)
        {
            return true;
        }

        FavoriteData* parentData = GetFavoriteDataFromModelIndex(parent);
        if (!parentData)
        {
            parentData = m_rootItem.get();
        }
        else
        {
            // Don't allow drops onto entries that aren't folders
            if (parentData->m_type != FavoriteData::FavoriteType::DataType_Folder)
            {
                return true;
            }
        }

        if (!parentData)
        {
            return true;
        }

        bool droppedOnFolder = false;

        // If we aren't given a specific row, add it to the end of the parent by default
        if (row == -1)
        {
            row = parentData->m_children.size();
            droppedOnFolder = true;
        }

        bool favoritesUpdated = false;

        if (data->hasFormat(FavoriteData::GetMimeType()))
        {
            QList<FavoriteData*> mimeList;
            GetSelectedIndicesFromMimeData(mimeList, data->data(FavoriteData::GetMimeType()));

            int rowOffset = 0;

            // Preliminary check to avoid dropping entries on themselves
            for (FavoriteData* movedChild : mimeList)
            {
                if (movedChild == parentData)
                {
                    return true;
                }
            }

            for (FavoriteData* movedChild : mimeList)
            {
                // Can't move it if it doesn't have a parent
                if (!movedChild->m_parent)
                {
                    continue;
                }

                if (droppedOnFolder && movedChild->m_parent == parentData)
                {
                    continue;
                }

                int oldIndex = movedChild->m_parent->m_children.indexOf(movedChild);

                QModelIndex oldParentModelIndex = GetModelIndexForParent(movedChild);

                int newRow = row + rowOffset;

                // Don't do anything if we're attempting to put it in the same place or right below itself
                if (oldParentModelIndex == parent && (oldIndex == newRow || newRow == oldIndex + 1))
                {
                    continue;
                }

                beginMoveRows(oldParentModelIndex, oldIndex, oldIndex, parent, newRow);

                if (oldParentModelIndex == parent && oldIndex > newRow)
                {
                    // We have to remove first, then add to make sure we handle the case of moving up on the same parent
                    movedChild->m_parent->m_children.removeAt(oldIndex);
                    parentData->m_children.insert(newRow, movedChild);
                }
                else
                {
                    // Otherwise, add first, then remove should work correctly in all other cases
                    parentData->m_children.insert(newRow, movedChild);
                    movedChild->m_parent->m_children.removeAt(oldIndex);
                }

                movedChild->m_parent = parentData;

                endMoveRows();

                emit ExpandIndex(parent, true);

                favoritesUpdated = true;

                rowOffset++;
            }
        }
        else if (data->hasFormat(AzToolsFramework::AssetBrowser::AssetBrowserEntry::GetMimeType()))
        {
            AzToolsFramework::AssetBrowser::AssetBrowserEntry::ForEachEntryInMimeData<AzToolsFramework::AssetBrowser::ProductAssetBrowserEntry>(data, [this, &favoritesUpdated, &parent](const AzToolsFramework::AssetBrowser::ProductAssetBrowserEntry* product)
            {
                if (IsSliceAssetType(product->GetAssetType()) && !IsFavorite(product))
                {
                    AddFavorite(product, parent);
                    favoritesUpdated = true;
                }
            });
        }

        if (favoritesUpdated)
        {
            UpdateFavorites();
        }

        return false;
    }

    void FavoriteDataModel::GetSelectedIndicesFromMimeData(QList<FavoriteData*>& results, const QByteArray& buffer) const
    {
        QVector<QString> mimeVector;
        QDataStream dataStream(buffer);

        dataStream >> mimeVector;

        for (QString movedChildAddress : mimeVector)
        {
            FavoriteData* movedChild = (FavoriteData*)movedChildAddress.toULongLong();
            results.push_back(movedChild);
        }
    }

    bool FavoriteDataModel::canDropMimeData(const QMimeData* data, [[maybe_unused]] Qt::DropAction action, [[maybe_unused]] int row, [[maybe_unused]] int column, const QModelIndex& parent) const
    {
        if (!data)
        {
            return false;
        }

        FavoriteData* parentData = GetFavoriteDataFromModelIndex(parent);
        if (!parentData)
        {
            parentData = m_rootItem.get();
        }

        if (!parentData)
        {
            return false;
        }

        // We can only drop onto a folder or empty space (m_rootItem)
        if (parentData->m_type != FavoriteData::DataType_Folder)
        {
            return false;
        }

        bool hasFavoriteMimeData = data->hasFormat(FavoriteData::GetMimeType());
        bool hasAssetBrowserMimeData = data->hasFormat(AzToolsFramework::AssetBrowser::AssetBrowserEntry::GetMimeType());

        if (hasFavoriteMimeData)
        {
            QList<FavoriteData*> mimeList;
            GetSelectedIndicesFromMimeData(mimeList, data->data(FavoriteData::GetMimeType()));

            // We cannot drop something onto itself
            if (mimeList.contains(parentData))
            {
                return false;
            }

            // We cannot drop a parent onto or into its own child hierarchy
            QList<FavoriteData*> dropHierarchy;
            FavoriteData* currentParent = parentData;
            while (currentParent)
            {
                dropHierarchy.push_back(currentParent);
                currentParent = currentParent->m_parent;
            }

            for (FavoriteData* mime : mimeList)
            {
                if (dropHierarchy.contains(mime))
                {
                    return false;
                }
            }
        }
        else if (hasAssetBrowserMimeData)
        {
            bool hasNonSliceInMimeData = false;
            bool draggingExistingFavorite = false;

            using namespace AzToolsFramework::AssetBrowser;

            // Make sure it is only slice entries and that there aren't existing favorites in the drag
            AssetBrowserEntry::ForEachEntryInMimeData<ProductAssetBrowserEntry>(data, [&hasNonSliceInMimeData, &draggingExistingFavorite, this](const ProductAssetBrowserEntry* product)
            {
                if (!IsSliceAssetType(product->GetAssetType()))
                {
                    hasNonSliceInMimeData = true;
                }

                if (IsFavorite(product))
                {
                    draggingExistingFavorite = true;
                }
            });

            if (hasNonSliceInMimeData | draggingExistingFavorite)
            {
                return false;
            }
        }
        else
        {
            return false;
        }

        return true;
    }

    QModelIndex FavoriteDataModel::AddNewFolder(const QModelIndex& parent)
    {
        FavoriteData* parentData = GetFavoriteDataFromModelIndex(parent);
        if (parentData)
        {
            // We always add new folders to the top
            beginInsertRows(parent, 0, 0);

            FavoriteData* newFavorite = new FavoriteData("New Folder", FavoriteData::DataType_Folder);
            newFavorite->m_parent = parentData;
            parentData->m_children.push_front(newFavorite);

            UpdateFavorites();

            endInsertRows();

            if (parentData != m_rootItem.get())
            {
                // ask for the parent to be expanded to show the new folder
                emit ExpandIndex(parent, true);
            }

            return GetModelIndexForFavorite(newFavorite);
        }

        return QModelIndex();
    }

    QModelIndex FavoriteDataModel::GetModelIndexForParent(const FavoriteData* child) const
    {
        if (!child->m_parent || child->m_parent == m_rootItem.get())
        {
            return QModelIndex();
        }

        return createIndex(child->m_parent->row(), 0, child->m_parent);
    }

    bool FavoriteDataModel::setData(const QModelIndex& index, const QVariant& value, int role)
    {
        if (index.isValid())
        {
            FavoriteData* item = GetFavoriteDataFromModelIndex(index);

            if (item)
            {
                if (role == Qt::EditRole)
                {
                    if (value.toString().size() > 0)
                    {
                        item->m_name = value.toString();
                        UpdateFavorites();
                    }
                }
            }
        }

        return QAbstractItemModel::setData(index, value, role);
    }

    QModelIndex FavoriteDataModel::GetModelIndexForFavorite(const FavoriteData* favorite) const
    {
        if (!favorite)
        {
            return QModelIndex();
        }

        return createIndex(favorite->row(), 0, const_cast<FavoriteData*>(favorite));
    }

    bool FavoriteDataModel::IsDescendentOf(QModelIndex index, QModelIndex potentialAncestor)
    {
        if (!index.isValid() || !potentialAncestor.isValid())
        {
            return false;
        }

        if (index == potentialAncestor)
        {
            return false;
        }

        QModelIndex parent = index.parent();
        while (parent.isValid())
        {
            if (parent == potentialAncestor)
            {
                return true;
            }
            parent = parent.parent();
        }

        return false;
    }

    FavoriteData* FavoriteDataModel::GetFavoriteDataFromModelIndex(const QModelIndex& modelIndex) const
    {
        if (modelIndex.isValid())
        {
            return static_cast<FavoriteData*>(modelIndex.internalPointer());
        }

        // Invalid index = root item
        return m_rootItem.get();
    }

    void FavoriteDataModel::NotifyRegisterViews()
    {
        using namespace AzToolsFramework;

        ViewPaneOptions sliceFavoritesOptions;
        sliceFavoritesOptions.canHaveMultipleInstances = false;
        sliceFavoritesOptions.preferedDockingArea = Qt::RightDockWidgetArea;
        RegisterViewPane<ComponentSliceFavoritesWindow>(
            SliceFavorites::ManageSliceFavorites,
            "Other",
            sliceFavoritesOptions);
    }

    void FavoriteDataModel::CountFoldersAndFavoritesFromIndices(const QModelIndexList& indices, int& numFolders, int& numFavorites)
    {
        numFolders = 0;
        numFavorites = 0;

        for (const QModelIndex& index : indices)
        {
            FavoriteData* indexData = GetFavoriteDataFromModelIndex(index);
            numFolders += indexData->GetNumFoldersInHierarchy();
            numFavorites += indexData->GetNumFavoritesInHierarchy();
        }
    }

    void FavoriteDataModel::ClearAll()
    {
        beginResetModel();

        // Make a copy of the list here because RemoveFavorite removes the entries from m_rootItem->m_children
        QList<FavoriteData*> rootChildren = m_rootItem->m_children;
        for (FavoriteData* child : rootChildren)
        {
            RemoveFavorite(child);
        }

        m_favoriteMap.clear();
        m_rootItem->Reset();

        UpdateFavorites();

        endResetModel();
    }

    bool FavoriteDataModel::HasFavoritesOrFolders() const
    {
        if (!m_rootItem)
        {
            return 0;
        }

        return (m_rootItem->m_children.size() > 0);
    }

    int FavoriteDataModel::ImportFavorites(const QString& importFileName)
    {
        beginResetModel();

        if (!AZ::IO::SystemFile::Exists(importFileName.toUtf8().data()))
        {
            emit DisplayWarning(tr("Invalid Slice Favorites File"), tr("File doesn't exist."));
            return 0;
        }

        uint64_t fileSize = AZ::IO::SystemFile::Length(importFileName.toUtf8().data());
        if (fileSize == 0)
        {
            emit DisplayWarning(tr("Invalid Slice Favorites File"), tr("The selected file is empty."));
            return 0;
        }

        std::vector<char> buffer(fileSize + 1);
        buffer[fileSize] = 0;
        if (!AZ::IO::SystemFile::Read(importFileName.toUtf8().data(), buffer.data(), fileSize))
        {
            emit DisplayWarning(tr("Invalid Slice Favorites File"), tr("Error reading the file, it may be corrupt."));
            return 0;
        }

        int numFavoritesImported = 0;

        AZ::rapidxml::xml_document<char>* xmlDoc = azcreate(AZ::rapidxml::xml_document<char>, (), AZ::SystemAllocator);
        if (xmlDoc->parse<0>(buffer.data()))
        {
            AZ::rapidxml::xml_node<char>* xmlRootNode = xmlDoc->first_node();
            if (!xmlRootNode || azstrnicmp(xmlRootNode->name(), RootXMLTag, strlen(RootXMLTag) + 1))
            {
                emit DisplayWarning(tr("Invalid Slice Favorites File"), tr("The XML isn't recognized as a valid SliceFavorites File, please try a different file to import."));
                return 0;
            }

            for (AZ::rapidxml::xml_node<char>* childNode = xmlRootNode->first_node(); childNode; childNode = childNode->next_sibling())
            {
                if (!azstricmp(childNode->name(), FavoriteDataXMLTag))
                {
                    FavoriteData* newFavorite = new FavoriteData();
                    int numFavorites = newFavorite->LoadFromXML(childNode, m_rootItem.get());
                    if (numFavorites)
                    {
                        numFavoritesImported += numFavorites;
                        m_rootItem->m_children.push_back(newFavorite);

                        if (newFavorite->m_type == FavoriteData::DataType_Favorite)
                        {
                            AZ::Data::AssetInfo checkAsset;
                            AZ::Data::AssetCatalogRequestBus::BroadcastResult(checkAsset, &AZ::Data::AssetCatalogRequests::GetAssetInfoById, newFavorite->m_assetId);
                            if (checkAsset.m_sizeBytes > 0) /* Check if the slice asset still exists on disk. */
                            {
                                m_favoriteMap.emplace(newFavorite->m_assetId, newFavorite);
                            }
                        }
                    }
                }
            }

            azdestroy(xmlDoc, AZ::SystemAllocator, AZ::rapidxml::xml_document<char>);
        }

        BuildChildToParentMap();

        UpdateFavorites();

        endResetModel();

        return numFavoritesImported;
    }

    int FavoriteDataModel::ExportFavorites(const QString& exportFileName) const
    {
        AZ::rapidxml::xml_document<char>* xmlDoc = azcreate(AZ::rapidxml::xml_document<char>, (), AZ::SystemAllocator);
        AZ::rapidxml::xml_node<char>* xmlRootNode = xmlDoc->allocate_node(AZ::rapidxml::node_element, RootXMLTag);
       
        int numFavoritesExported = m_rootItem->AddToXML(xmlRootNode, xmlDoc);

        std::string xmlDocString;
        AZ::rapidxml::print(std::back_inserter(xmlDocString), *xmlRootNode, 0);

        AZ::IO::SystemFile outFile;
        outFile.Open(exportFileName.toUtf8().data(), AZ::IO::SystemFile::SF_OPEN_CREATE | AZ::IO::SystemFile::SF_OPEN_WRITE_ONLY);
        outFile.Write(xmlDocString.c_str(), xmlDocString.length());
        outFile.Close();

        azdestroy(xmlDoc, AZ::SystemAllocator, AZ::rapidxml::xml_document<char>);

        return numFavoritesExported;
    }

    void FavoriteDataModel::OnCatalogAssetRemoved(const AZ::Data::AssetId& assetId, const AZ::Data::AssetInfo& /*assetInfo*/)
    {
        if (assetId.IsValid())
        {
            // Add the asset to the removed list so that the removal is processed in the main thread.
            m_removedAssets.push_back(assetId);
            
            QMetaObject::invokeMethod(this, "ProcessRemovedAssets", Qt::QueuedConnection);
        }
    }

    void FavoriteDataModel::OnAssetBrowserComponentReady()
    {
        LoadFavorites();
    }

    void FavoriteDataModel::RemoveFavorite(const AZ::Data::AssetId& assetId)
    {
        FavoriteData* data = GetFavoriteDataFromAssetId(assetId);
        if (data)
        {
            RemoveFavorite(data);
            delete data;

            UpdateFavorites();
        }
    }

    FavoriteData* FavoriteDataModel::GetFavoriteDataFromAssetId(const AZ::Data::AssetId& assetId) const
    {
        const auto& foundIt = m_favoriteMap.find(assetId);
        if (foundIt != m_favoriteMap.end())
        {
            return foundIt->second;
        }

        return nullptr;
    }

    void FavoriteDataModel::DragEnter(QDragEnterEvent* event, AzQtComponents::DragAndDropContextBase& context)
    {
        if (CanAcceptDragAndDropEvent(event, context))
        {
            event->setDropAction(Qt::CopyAction);
            event->setAccepted(true);
        }
    }

    void FavoriteDataModel::DragMove(QDragMoveEvent* event, AzQtComponents::DragAndDropContextBase& context)
    {
        if (CanAcceptDragAndDropEvent(event, context))
        {
            event->setDropAction(Qt::CopyAction);
            event->setAccepted(true);
        }
    }

    void FavoriteDataModel::DragLeave(QDragLeaveEvent* /*event*/)
    {
        // opportunities to show ghosted entities or previews here.
    }

    void FavoriteDataModel::Drop(QDropEvent* event, AzQtComponents::DragAndDropContextBase& context)
    {
        using namespace AzQtComponents;

        // ALWAYS CHECK - you are not the only one connected to this bus, and someone else may have already
        // handled the event or accepted the drop - it might not contain types relevant to you.
        // you still get informed about the drop event in case you did some stuff in your gui and need to clean it up.
        if (!CanAcceptDragAndDropEvent(event, context))
        {
            return;
        }
        // The call for CanAcceptDragAndDropEvent checks for all possible early out criteria (invalid context type,
        // event already accepted by another listener, there is no mimedata, etc)

        ViewportDragContext* viewportDragContext = azrtti_cast<ViewportDragContext*>(&context);

        event->setDropAction(Qt::CopyAction);
        event->setAccepted(true);

        QList<FavoriteData*> mimeList;
        GetSelectedIndicesFromMimeData(mimeList, event->mimeData()->data(FavoriteData::GetMimeType()));

        const AZ::Transform worldTransform = AZ::Transform::CreateTranslation(viewportDragContext->m_hitLocation);

        // make a scoped undo that covers the ENTIRE operation.
        AzToolsFramework::ScopedUndoBatch undo("Instantiate slices from slice favorites");
        
        for (FavoriteData* favorite : mimeList)
        {
            // Handle instantiation of slices.
            if (favorite->m_type == FavoriteData::FavoriteType::DataType_Favorite && favorite->m_subType != FavoriteData::FavoriteSubType::DynamicSlice)
            {
                // Instantiate the slice at the specified location.
                AZ::Data::Asset<AZ::SliceAsset> asset = AZ::Data::AssetManager::Instance().FindOrCreateAsset<AZ::SliceAsset>(favorite->m_assetId, AZ::Data::AssetLoadBehavior::Default);
                if (asset)
                {
                    AzFramework::SliceInstantiationTicket spawnTicket;

                    AzToolsFramework::SliceEditorEntityOwnershipServiceRequestBus::BroadcastResult(spawnTicket,
                        &AzToolsFramework::SliceEditorEntityOwnershipServiceRequests::InstantiateEditorSlice, asset, worldTransform);
                }
            }
        }
    }

    bool FavoriteDataModel::CanAcceptDragAndDropEvent(QDropEvent* event, AzQtComponents::DragAndDropContextBase& context) const
    {
        using namespace AzQtComponents;
        using namespace AzToolsFramework;

        // if a listener with a higher priority already claimed this event, do not touch it.
        ViewportDragContext* viewportDragContext = azrtti_cast<ViewportDragContext*>(&context);
        if ((!event) || (!event->mimeData()) || (event->isAccepted()) || (!viewportDragContext))
        {
            return false;
        }

        return event->mimeData()->hasFormat(FavoriteData::GetMimeType());
    }
}

#include <Source/moc_FavoriteDataModel.cpp>
