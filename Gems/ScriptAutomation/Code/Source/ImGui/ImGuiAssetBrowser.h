/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include "Utils.h"
#include "ImGuiMessageBox.h"
#include <AzFramework/Asset/AssetCatalogBus.h>

namespace ScriptAutomation
{
    //! Provides a pair of list boxes for browsing and selecting assets.
    //! The first list box is for a collection of all 'available' assets and
    //! the second is a list of 'pinned' assets. The client code provides the
    //! collection of available assets using PopulateAssets(), and can query
    //! the browser for the available, pinned, and selected assets.
    //! 
    //! The state of the UI is stored in a local cache file so the layout
    //! and pinned asset list will be preserved between runs.
    //! 
    //! Note, this has nothing to do with the AzToolsFramework::AssetBrowser;
    //! it's just a very simple way to expose a pick from a list of assets in ImGui.
    class ImGuiAssetBrowser 
        : public AzFramework::AssetCatalogEventBus::Handler
    {
    public:
        using AssetList = AZStd::vector<Utils::AssetEntry>;

        struct WidgetSettings
        {
            struct Labels
            {
                const char* m_root = "Assets";
                const char* m_assetList = "Available";
                const char* m_pinnedAssetList = "Pinned";
                const char* m_pinButton = "Pin";
                const char* m_unpinButton = "Unpin";
            } m_labels;
        };

        using AssetFilterCallback = AZStd::function<bool(const AZ::Data::AssetInfo& assetInfo)>;

        static void Reflect(AZ::ReflectContext* context);

        void Activate();
        void Deactivate();

        //! @param configFilePath - Path to a local json file for maintaining state between runs. Should start with "@user@/"
        explicit ImGuiAssetBrowser(AZStd::string_view configFilePath);

        //! Set a callback function that will be used to filter which assets should be included in the displayed list.
        //! @param shouldInclude - return true for any asset that should be included in the list
        void SetFilter(AssetFilterCallback shouldInclude);
        
        //! Returns whether a config file was loaded. See LoadConfigFile()
        bool IsConfigFileLoaded() const;

        //! Sets the default list of pinned assets, which will be applied if the user clicks "Reset to Defaults".
        //! @param assetPaths - A list of paths to asset files in the asset cache.
        //! @param applyNow - Calls SetPinnedAssets with this list too
        void SetDefaultPinnedAssets(const AZStd::vector<AZStd::string>& assetPaths, bool applyNow = false);

        //! Replaces the list of pinned assets
        //! @param assetPaths - A list of paths to asset files in the asset cache
        void SetPinnedAssets(const AZStd::vector<AZStd::string>& assetPaths);

        //! Resets the pin list to the set of default assets. See SetDefaultPinnedAssets().
        void ResetPinnedAssetsToDefault();

        //! Returns the list of all available assets, shown in the first box
        const AssetList& GetAssets() const;

        //! Returns the list of all pinned assets, which is a subset of GetAssets(), shown in the second box
        const AssetList& GetPinnedAssets() const;

        //! Set which of the available assets is selected
        void SelectAsset(int32_t assetIndex);

        //! Returns the index of the selected asset, or -1 if none is selected
        int32_t GetSelectedAssetIndex() const;

        //! Returns the AssetId of the selected asset. May be null if there is no selection, or there was an error loading the selected asset.
        AZ::Data::AssetId GetSelectedAssetId() const;

        //! Returns an Asset<> reference of the selected asset. May be null if there is no selection, or there was an error loading the selected asset.
        template<typename AssetDataT>
        AZ::Data::Asset<AssetDataT> GetSelectedAsset() const;

        //! Returns the path of the selected asset. May be empty if there is no selection, or there was an error loading the selected asset.
        AZStd::string GetSelectedAssetPath() const;

        //! Returns the index of the previously selected asset, or -1 if none was selected
        int32_t GetPrevSelectedAssetIndex() const;

        //! Returns the AssetId of the previously selected asset. May be null if there was no selection, or there was an error loading the selected asset.
        AZ::Data::AssetId GetPrevSelectedAssetId() const;

        //! Draw the ImGui
        //! @return true if the asset selection changed
        bool Tick(const WidgetSettings& widgetSettings);

        //! Force a UI refresh on the next Tick().
        void SetNeedsRefresh() { m_needsRefresh = true;  }

    private:

        struct ConfigFile final
        {
            AZ_TYPE_INFO(ConfigFile, "{9CD887DD-F572-4BEB-B57B-21A86CA1DD42}");
            AZ_CLASS_ALLOCATOR(ConfigFile, AZ::SystemAllocator, 0);

            static void Reflect(AZ::ReflectContext* context);

            // We only store paths in the pinned-list because they may change if changes
            // are made to the asset builders. This is uncommon for regular users
            // but as this may be a more common case for developing and debugging
            // Atom it is worth handling.
            AZStd::vector<AZStd::string> m_pinnedAssetPaths;

            bool m_expandRoot = true;
            bool m_expandAvailableList = true;
            bool m_expandPinnedList = true;
        };

        //! Attempts to load the saved widget state from the local cache file.
        //! @return true if successfully loaded
        bool LoadConfigFile();

        void PopulateAssets(AZStd::function<bool(const AZ::Data::AssetInfo& assetInfo)> shouldInclude);

        // AzFramework::AssetCatalogEventBus::Handler overrides...
        void OnCatalogAssetAdded(const AZ::Data::AssetId& assetId) override;
        void OnCatalogAssetRemoved(const AZ::Data::AssetId& assetId, const AZ::Data::AssetInfo& assetInfo) override;

        void OnCatalogChanged(const AZ::Data::AssetId& assetId);

        void UpdateConfigFilePins();
        void SaveConfigFile();

        AZStd::string m_configFilePath;
        ConfigFile m_configFile;
        bool m_isConfigFileLoaded = false;

        AssetFilterCallback m_includedAssetFilter;
        bool m_needsRefresh = true;

        AZStd::vector<AZStd::string> m_defaultPinnedAssetPaths;

        ImGuiMessageBox m_confirmClearPinList;

        AssetList m_assets;
        AssetList m_pinnedAssets;
        int32_t m_prevSelectedAssetIndex = -1;
        int32_t m_selectedAssetIndex = -1;
        int32_t m_selectedPinnedAssetIndex = -1;
    };

    template<typename AssetDataT>
    AZ::Data::Asset<AssetDataT> ImGuiAssetBrowser::GetSelectedAsset() const
    {
        if (GetSelectedAssetId().IsValid())
        {
            return AZ::Data::Asset<AssetDataT>{GetSelectedAssetId(), azrtti_typeid<AssetDataT>(), GetSelectedAssetPath()};
        }
        else
        {
            return AZ::Data::Asset<AssetDataT>{};
        }
    }
} // namespace ScriptAutomation
