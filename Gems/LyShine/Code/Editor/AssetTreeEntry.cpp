/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "AssetTreeEntry.h"
#include "EditorCommon.h"

#include <AzCore/std/containers/map.h>
#include <AzCore/Asset/AssetManager.h>
#include <AzCore/Asset/AssetTypeInfoBus.h>

#include <AzFramework/StringFunc/StringFunc.h>

#include <AzToolsFramework/AssetBrowser/AssetBrowserModel.h>
#include <AzToolsFramework/AssetBrowser/AssetBrowserBus.h>
#include <AzToolsFramework/AssetBrowser/AssetBrowserEntry.h>

UISliceLibraryFilter::UISliceLibraryFilter(const AZ::Data::AssetType& assetType, const AZStd::string& pathToSearch)
    : m_assetType(assetType)
    , m_pathToSearch(pathToSearch)
{
    // propagation = Down means the filter will examine all children recursively until it satisfies or no more children are left
    // in our case we start from root entry and examine everything underneath it
    SetFilterPropagation(AzToolsFramework::AssetBrowser::AssetBrowserEntryFilter::PropagateDirection::Down);
}

AzToolsFramework::AssetBrowser::AssetBrowserEntryFilter* UISliceLibraryFilter::Clone() const
{
    auto clone = new UISliceLibraryFilter(m_assetType, m_pathToSearch.c_str());
    clone->m_name = m_name;
    clone->m_tag = m_tag;
    clone->m_direction = m_direction;
    return clone;
}

QString UISliceLibraryFilter::GetNameInternal() const
{
    return "UISliceLibraryFilter";
}

bool UISliceLibraryFilter::MatchInternal(const AzToolsFramework::AssetBrowser::AssetBrowserEntry* entry) const
{
    // entry must be a product
    auto product = azrtti_cast<const AzToolsFramework::AssetBrowser::ProductAssetBrowserEntry*>(entry);
    if (!product)
    {
        return false;
    }
    // entry must be of slice asset type
    if (product->GetAssetType() != m_assetType)
    {
        return false;
    }
    // entry must be located within m_pathToSearch
    AZStd::string relativePath = product->GetRelativePath();
    AzFramework::StringFunc::AssetDatabasePath::Normalize(relativePath);
    if (AzFramework::StringFunc::Find(relativePath, m_pathToSearch.c_str()) == AZStd::string::npos)
    {
        return false;
    }
    return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
AssetTreeEntry::~AssetTreeEntry()
{
    for (auto folderEntry : m_folders)
    {
        delete folderEntry.second;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void AssetTreeEntry::Insert(const AZStd::string& path, const AZStd::string& menuName, const AZ::Data::AssetId& assetId)
{
    if (path.empty())
    {
        // there are no more folders in the pathname - add the leaf file entry
        m_files.insert(AZStd::make_pair(menuName, assetId));
    }
    else
    {
        AZStd::string folderName;
        AZStd::string remainderPath;
        size_t separator = path.find('/');
        if (separator == AZStd::string::npos)
        {
            folderName = path;
        }
        else
        {
            folderName = path.substr(0, separator);
            if (path.length() > separator + 1)
            {
                remainderPath = path.substr(separator + 1);
            }
        }

        AssetTreeEntry* folderEntry = nullptr;
        // check if the folder already exists
        if (m_folders.count(folderName) == 0)
        {
            // does not exist, create it and insert it in tree
            folderEntry = new AssetTreeEntry;
            m_folders.insert(AZStd::make_pair(folderName, folderEntry));
        }
        else
        {
            // already exists
            folderEntry = m_folders[folderName];
        }

        // recurse down the pathname creating folders until we get to the leaf folder
        // to insert the file entry
        folderEntry->Insert(remainderPath, menuName, assetId);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
AssetTreeEntry* AssetTreeEntry::BuildAssetTree(const AZ::Data::AssetType& assetType, const AZStd::string& pathToSearch)
{
    using namespace AzToolsFramework::AssetBrowser;

    // asset browser model is a collection of all assets. We search it from root entry down for all ui slice files.
    AssetBrowserModel* assetBrowserModel;
    AssetBrowserComponentRequestBus::BroadcastResult(assetBrowserModel, &AssetBrowserComponentRequests::GetAssetBrowserModel);
    AZ_Assert(assetBrowserModel, "Failed to get asset browser model");

    const auto rootEntry = assetBrowserModel->GetRootEntry();

    // UISliceLibraryFilter::Filter function returns all assets (recursively) that match the specified filter
    // in this case we are only looking for ui slices.
    AZStd::unordered_set<const AssetBrowserEntry*> entries;
    UISliceLibraryFilter filter(assetType, pathToSearch);
    filter.Filter(entries, rootEntry.get());

    AssetTreeEntry* assetTree = new AssetTreeEntry;
    for (const auto& entry : entries)
    {
        if (auto product = azrtti_cast<const ProductAssetBrowserEntry*>(entry); product)
        {
            AZStd::string name;
            AZStd::string path;
            // split the product relative path into name and path. Note that product's parent (source entry) is used because
            // product name stored in db is in all lower case, but we want to preserve case here
            AzFramework::StringFunc::Path::Split(product->GetParent()->GetRelativePath().c_str(), nullptr, &path, &name);
            // find next character position after default slice path in order to generate hierarchical sub-menus matching the subfolders
            const size_t pos = AzFramework::StringFunc::Find(path, pathToSearch) + pathToSearch.length();
            assetTree->Insert(path.substr(pos), name, product->GetAssetId());
        }
    }
    return assetTree;
}
