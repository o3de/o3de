/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "ImGuiAssetBrowser.h"
#include <AzCore/IO/SystemFile.h>
#include <AzCore/std/sort.h>
#include <AzCore/Serialization/Utils.h>
#include "ScriptableImGui.h"

namespace ScriptAutomation
{
    void ImGuiAssetBrowser::Reflect(AZ::ReflectContext* context)
    {
        ImGuiAssetBrowser::ConfigFile::Reflect(context);
    }

    void ImGuiAssetBrowser::ConfigFile::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<ConfigFile>()
                ->Version(0)
                ->Field("PinnedAssetPaths", &ConfigFile::m_pinnedAssetPaths)
                ->Field("ExpandRoot", &ConfigFile::m_expandRoot)
                ->Field("ExpandAvailableList", &ConfigFile::m_expandAvailableList)
                ->Field("ExpandPinnedList", &ConfigFile::m_expandPinnedList)
                ;
        }
    }

    ImGuiAssetBrowser::ImGuiAssetBrowser(AZStd::string_view configFilePath)
    {
        m_configFilePath = configFilePath;
    }

    void ImGuiAssetBrowser::Activate()
    {
        // The original m_configFilePath passed to the constructor likely starts with "@user@" which needs to be replaced with the real path.
        // The constructor is too early to resolve the path so we do it here on activation.
        char configFileFullPath[AZ_MAX_PATH_LEN] = {0};
        AZ::IO::FileIOBase::GetInstance()->ResolvePath(m_configFilePath.c_str(), configFileFullPath, AZ_MAX_PATH_LEN);
        m_configFilePath = configFileFullPath;

        AzFramework::AssetCatalogEventBus::Handler::BusConnect();
    }

    void ImGuiAssetBrowser::Deactivate()
    {
        if (!m_configFilePath.empty())
        {
            // We only report this message in Deactivate(), not inside SaveConfigFile(), to avoid spamming
            // this message when SaveConfigFile() is called in OnTick()
            AZ_TracePrintf("ImGuiAssetBrowser", "Saved settings to '%s'\n", m_configFilePath.c_str());

            SaveConfigFile();
        }

        AzFramework::AssetCatalogEventBus::Handler::BusDisconnect();
    }

    void ImGuiAssetBrowser::OnCatalogAssetAdded(const AZ::Data::AssetId& assetId)
    {
        OnCatalogChanged(assetId);
    }
        
    void ImGuiAssetBrowser::OnCatalogAssetRemoved(const AZ::Data::AssetId& assetId, const AZ::Data::AssetInfo&)
    {
        OnCatalogChanged(assetId);
    }

    void ImGuiAssetBrowser::OnCatalogChanged(const AZ::Data::AssetId& assetId)
    {
        AZ::Data::AssetInfo assetInfo;
        AZ::Data::AssetCatalogRequestBus::BroadcastResult(assetInfo, &AZ::Data::AssetCatalogRequestBus::Events::GetAssetInfoById, assetId);

        if (m_includedAssetFilter && m_includedAssetFilter(assetInfo))
        {
            m_needsRefresh = true;
        }
    }

    void ImGuiAssetBrowser::SetFilter(AssetFilterCallback shouldInclude)
    {
        m_includedAssetFilter = shouldInclude;
    }

    void ImGuiAssetBrowser::PopulateAssets(AZStd::function<bool(const AZ::Data::AssetInfo& assetInfo)> shouldInclude)
    {
        m_assets.clear();
        m_pinnedAssets.clear();
        m_configFile.m_pinnedAssetPaths.clear();
        m_prevSelectedAssetIndex = -1;
        m_selectedAssetIndex = -1;
        m_selectedPinnedAssetIndex = -1;

        auto startCB = []() {};

        auto enumerateCB = [this,shouldInclude](const AZ::Data::AssetId id, const AZ::Data::AssetInfo& assetInfo)
        {
            if (shouldInclude(assetInfo))
            {
                Utils::AssetEntry entry;
                entry.m_path = assetInfo.m_relativePath;
                entry.m_assetId = id;
                entry.m_name = assetInfo.m_relativePath;

                m_assets.push_back(entry);
            }
        };

        auto endCB = []() {};

        AZ::Data::AssetCatalogRequestBus::Broadcast(&AZ::Data::AssetCatalogRequestBus::Events::EnumerateAssets, startCB, enumerateCB, endCB);

        // Sort the assets that we've found alphabetically

        AZStd::sort(m_assets.begin(), m_assets.end(), [](const Utils::AssetEntry& lhs, const Utils::AssetEntry& rhs) {
            return lhs.m_path < rhs.m_path;
        });
    }

    const ImGuiAssetBrowser::AssetList& ImGuiAssetBrowser::GetAssets() const
    {
        return m_assets;
    }

    const ImGuiAssetBrowser::AssetList& ImGuiAssetBrowser::GetPinnedAssets() const
    {
        return m_pinnedAssets;
    }

    void ImGuiAssetBrowser::SelectAsset(int32_t assetIndex)
    {
        m_prevSelectedAssetIndex = m_selectedAssetIndex;
        m_selectedAssetIndex = assetIndex;
        m_selectedPinnedAssetIndex = -1;
    }

    int32_t ImGuiAssetBrowser::GetSelectedAssetIndex() const
    {
        return m_selectedAssetIndex;
    }

    AZ::Data::AssetId ImGuiAssetBrowser::GetSelectedAssetId() const
    {
        AZ::Data::AssetId id;

        if (m_selectedAssetIndex >= 0)
        {
            id = m_assets[m_selectedAssetIndex].m_assetId;
        }

        return id;
    }

    AZStd::string ImGuiAssetBrowser::GetSelectedAssetPath() const
    {
        AZStd::string path;

        if (m_selectedAssetIndex >= 0)
        {
            path = m_assets[m_selectedAssetIndex].m_path;
        }

        return path;
    }

    int32_t ImGuiAssetBrowser::GetPrevSelectedAssetIndex() const
    {
        return m_prevSelectedAssetIndex;
    }

    AZ::Data::AssetId ImGuiAssetBrowser::GetPrevSelectedAssetId() const
    {
        AZ::Data::AssetId id;

        if (m_prevSelectedAssetIndex >= 0)
        {
            id = m_assets[m_prevSelectedAssetIndex].m_assetId;
        }

        return id;
    }

    void ImGuiAssetBrowser::SetDefaultPinnedAssets(const AZStd::vector<AZStd::string>& assetPaths, bool applyNow)
    {
        m_defaultPinnedAssetPaths = assetPaths;

        if (applyNow)
        {
            ResetPinnedAssetsToDefault();
        }
    }

    void ImGuiAssetBrowser::ResetPinnedAssetsToDefault()
    {
        SetPinnedAssets(m_defaultPinnedAssetPaths);
    }

    void ImGuiAssetBrowser::SetPinnedAssets(const AZStd::vector<AZStd::string>& assetPaths)
    {
        m_configFile.m_pinnedAssetPaths = assetPaths;

        // Look up asset ids from asset paths

        m_pinnedAssets.clear();
        m_pinnedAssets.reserve(m_configFile.m_pinnedAssetPaths.size());

        for (const AZStd::string& assetPath : m_configFile.m_pinnedAssetPaths)
        {
            AZ::Data::AssetId assetId;
            AZ::Data::AssetCatalogRequestBus::BroadcastResult(
                assetId, &AZ::Data::AssetCatalogRequestBus::Events::GetAssetIdByPath,
                assetPath.c_str(), AZ::Data::AssetType(), false);

            AZ_Warning("ImGuiAssetBrowser", assetId.IsValid(), "Failed to get asset id for '%s'", assetPath.c_str());

            Utils::AssetEntry entry;
            entry.m_path = assetPath;
            entry.m_assetId = assetId;
            entry.m_name = assetId.IsValid() ? assetPath : "<Missing> " + assetPath;

            m_pinnedAssets.push_back(entry);
        }
    }

    bool ImGuiAssetBrowser::LoadConfigFile()
    {
        AZStd::unique_ptr<ConfigFile> configFile(AZ::Utils::LoadObjectFromFile<ConfigFile>(m_configFilePath));

        if (configFile)
        {
            m_isConfigFileLoaded = true;
            m_configFile = *configFile;
            SetPinnedAssets(configFile->m_pinnedAssetPaths);

            return true;
        }
        else
        {
            return false;
        }
    }

    bool ImGuiAssetBrowser::IsConfigFileLoaded() const
    {
        return m_isConfigFileLoaded;
    }

    void ImGuiAssetBrowser::UpdateConfigFilePins()
    {
        m_configFile.m_pinnedAssetPaths.clear();

        m_configFile.m_pinnedAssetPaths.reserve(m_pinnedAssets.size());

        for (const Utils::AssetEntry& entry : m_pinnedAssets)
        {
            m_configFile.m_pinnedAssetPaths.push_back(entry.m_path);
        }
    }

    void ImGuiAssetBrowser::SaveConfigFile()
    {
        if (m_configFilePath.empty())
        {
            AZ_Warning("ImGuiAssetBrowser", false, "m_configFilePath is not set. GUI state not saved.");
        }
        else if (!AZ::Utils::SaveObjectToFile(m_configFilePath, AZ::DataStream::ST_XML, &m_configFile))
        {
            AZ_Error("ImGuiAssetBrowser", false, "Failed to save '%s'", m_configFilePath.c_str());
        }
    }

    bool ImGuiAssetBrowser::Tick(const WidgetSettings& widgetSettings)
    {
        ScriptableImGui::ScopedNameContext nameContext(widgetSettings.m_labels.m_root);

        if (m_needsRefresh)
        {
            // Save currently pinned assets so they can be restored if the config file fails to load.
            auto savedPinnedAssets = m_configFile.m_pinnedAssetPaths;

            PopulateAssets(m_includedAssetFilter);

            if (!LoadConfigFile())
            {
                AZ_Warning("ImGuiAssetBrowser", false, "Failed to load config '%s'.", m_configFilePath.data());
                SetPinnedAssets(savedPinnedAssets);
            }

            m_needsRefresh = false;
        }

        bool selectionChanged = false;

        bool pinListChanged = false;

        bool rootExpansionChanged = false;
        bool availableListExpansionChanged = false;
        bool pinnedListExpansionChanged = false;

        bool isRootNodeOpen = false;
        bool isAvailableListOpen = false;
        bool isPinnedListOpen = false;

        auto treeNodeFlag = [](bool shouldExpand)
        {
            return shouldExpand ? ImGuiTreeNodeFlags_DefaultOpen : 0;
        };

        ImGui::PushID(this);

        isRootNodeOpen = ImGui::TreeNodeEx(widgetSettings.m_labels.m_root, treeNodeFlag(m_configFile.m_expandRoot));
        if (isRootNodeOpen)
        {
            isAvailableListOpen = ImGui::TreeNodeEx(widgetSettings.m_labels.m_assetList, treeNodeFlag(m_configFile.m_expandAvailableList));
            if (isAvailableListOpen)
            {
                availableListExpansionChanged = !m_configFile.m_expandAvailableList;

                // [GFX TODO] When these list boxes are controlled from a script it would be nice to support auto-scrolling
                // to the selected position; that would require using ListBoxHeader/ListBoxFooter and Selectable instead of ListBox.

                ImGui::PushItemWidth(-1.0f);
                if (ScriptableImGui::ListBox("##Available", &m_selectedAssetIndex, &Utils::AssetEntryNameGetter, m_assets.data(), static_cast<int32_t>(m_assets.size()), 16))
                {
                    selectionChanged = true;
                }
                ImGui::PopItemWidth();

                ImGui::Spacing();

                if (ScriptableImGui::Button(widgetSettings.m_labels.m_pinButton))
                {
                    if (m_selectedAssetIndex >= 0)
                    {
                        const Utils::AssetEntry& selectedAsset = m_assets[m_selectedAssetIndex];

                        bool alreadyExists = false;
                        for (const Utils::AssetEntry& entry : m_pinnedAssets)
                        {
                            if (entry.m_assetId == selectedAsset.m_assetId)
                            {
                                alreadyExists = true;
                                break;
                            }
                        }

                        if (!alreadyExists)
                        {
                            m_pinnedAssets.push_back(selectedAsset);
                            pinListChanged = true;
                        }
                    }
                }

                ImGui::TreePop();
            }

            ImGui::Spacing();

            isPinnedListOpen = ImGui::TreeNodeEx(widgetSettings.m_labels.m_pinnedAssetList, treeNodeFlag(m_configFile.m_expandPinnedList));
            if (isPinnedListOpen)
            {
                pinnedListExpansionChanged = !m_configFile.m_expandPinnedList;

                ImGui::PushItemWidth(-1.0f);
                if (ScriptableImGui::ListBox("##Pinned", &m_selectedPinnedAssetIndex, &Utils::AssetEntryNameGetter, m_pinnedAssets.data(), static_cast<int32_t>(m_pinnedAssets.size()), 6))
                {
                    selectionChanged = true;

                    // Since GetSelectedAssetIndex() returns m_selectedAssetIndex, we have to keep that updated
                    // based on changes to m_selectedPinnedAssetIndex as well.
                    m_prevSelectedAssetIndex = m_selectedAssetIndex;
                    m_selectedAssetIndex = -1;
                    if (m_selectedPinnedAssetIndex >= 0)
                    {
                        AZ::Data::AssetId selectedAssetId = m_pinnedAssets[m_selectedPinnedAssetIndex].m_assetId;
                        for (int32_t i = 0; i < m_assets.size(); ++i)
                        {
                            if (m_assets[i].m_assetId == selectedAssetId)
                            {
                                m_selectedAssetIndex = i;
                                break;
                            }
                        }
                    }
                }
                ImGui::PopItemWidth();

                ImGui::Spacing();

                if (ScriptableImGui::Button(widgetSettings.m_labels.m_unpinButton))
                {
                    if (m_selectedPinnedAssetIndex >= 0 && m_selectedPinnedAssetIndex < m_pinnedAssets.size())
                    {
                        m_pinnedAssets.erase(m_pinnedAssets.begin() + m_selectedPinnedAssetIndex);
                        pinListChanged = true;

                        if (m_pinnedAssets.size() == 0)
                        {
                            // If there are no more pinned assets, explicitely set the m_selectedPinnedAssetIndex to -1. This seems like this should be imGui's responsibility, but it doesn't work that way.
                            m_selectedPinnedAssetIndex = -1;
                        }
                    }
                }

                if (ScriptableImGui::Button(m_defaultPinnedAssetPaths.empty() ? "Clear" : "Reset to Default"))
                {

                    m_confirmClearPinList.OpenPopupConfirmation(
                        m_defaultPinnedAssetPaths.empty() ? "Confirm Clear" : "Confirm Reset to Default",
                        AZStd::string::format("Reset %s to default?", widgetSettings.m_labels.m_pinnedAssetList),
                        [this,&pinListChanged]() {
                            ResetPinnedAssetsToDefault();
                            pinListChanged = true;
                        });
                }

                ImGui::TreePop();
            }

            ImGui::TreePop();

            // These only get set inside "if(isRootNodeOpen)" because otherwise we don't have correct values for isAvailableListOpen and isPinnedListOpen 
            availableListExpansionChanged = isAvailableListOpen != m_configFile.m_expandAvailableList;
            pinnedListExpansionChanged = isPinnedListOpen != m_configFile.m_expandPinnedList;
        }

        m_confirmClearPinList.TickPopup();

        rootExpansionChanged = isRootNodeOpen != m_configFile.m_expandRoot;

        ImGui::PopID();

        if (pinListChanged ||
            rootExpansionChanged ||
            availableListExpansionChanged ||
            pinnedListExpansionChanged)
        {
            if (rootExpansionChanged)
            {
                m_configFile.m_expandRoot = isRootNodeOpen;
            }

            if (availableListExpansionChanged)
            {
                m_configFile.m_expandAvailableList = isAvailableListOpen;
            }

            if (pinnedListExpansionChanged)
            {
                m_configFile.m_expandPinnedList = isPinnedListOpen;
            }

            UpdateConfigFilePins();

            SaveConfigFile();
        }

        return selectionChanged;
    }
} // namespace ScriptAutomation
