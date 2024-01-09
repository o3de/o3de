/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/std/string/string.h>
#include <AzCore/std/containers/map.h>
#include <AzCore/Asset/AssetCommon.h>
#include <AzToolsFramework/AssetBrowser/Search/Filter.h>
#include <QString>

namespace AzToolsFramework
{
    namespace AssetBrowser
    {
        class AssetBrowserEntry;
    }
}

//! UISliceLibraryFilter locates all of the UI slices so that they can be instantiated via "Element from Slice Library" menu function
class UISliceLibraryFilter
    : public AzToolsFramework::AssetBrowser::AssetBrowserEntryFilter
{
public:
    UISliceLibraryFilter(const AZ::Data::AssetType& assetType, const AZStd::string& pathToSearch);
    ~UISliceLibraryFilter() override = default;
    AzToolsFramework::AssetBrowser::AssetBrowserEntryFilter* Clone() const override;

protected:
    QString GetNameInternal() const override;
    bool MatchInternal(const  AzToolsFramework::AssetBrowser::AssetBrowserEntry* entry) const override;

private:
    AZ::Data::AssetType m_assetType;
    AZStd::string m_pathToSearch;
};

////////////////////////////////////////////////////////////////////////////////////////////////////
//! Class to build and represent a hierarchical tree view of files and folders containing
//! assets of a given type under a given path
class AssetTreeEntry
{
public: // types
    using FolderMap = AZStd::map<AZStd::string, AssetTreeEntry*>;
    using FileMap = AZStd::map<AZStd::string, AZ::Data::AssetId>;

public: // methods
    ~AssetTreeEntry();

    static AssetTreeEntry* BuildAssetTree(const AZ::Data::AssetType& assetType, const AZStd::string& pathToSearch);

public: // data

    FileMap m_files;
    FolderMap m_folders;

protected:
    void Insert(const AZStd::string& path, const AZStd::string& menuName, const AZ::Data::AssetId& assetId);
};
