/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <ImGui/ImGuiSaveFilePath.h>
#include "ScriptableImGui.h"
#include <AzFramework/IO/LocalFileIO.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/Utils.h>

namespace ScriptAutomation
{
    void ImGuiSaveFilePath::Reflect(AZ::ReflectContext* context)
    {
        ImGuiSaveFilePath::Config::Reflect(context);
    }

    void ImGuiSaveFilePath::Config::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<Config>()
                ->Version(0)
                ->Field("autoMode", &Config::m_autoMode)
                ->Field("currentExtension", &Config::m_currentExtension)
                ->Field("filePath", &Config::m_filePath)
                ;
        }
    }

    ImGuiSaveFilePath::ImGuiSaveFilePath(AZStd::string_view configFilePath)
    {
        m_configFilePath = configFilePath;
    }

    void ImGuiSaveFilePath::Activate()
    {
        // The original m_configFilePath passed to the constructor likely starts with "@user@" which needs to be replaced with the real path.
        // The constructor is too early to resolve the path so we do it here on activation.
        char configFileFullPath[AZ_MAX_PATH_LEN] = {0};
        AZ::IO::FileIOBase::GetInstance()->ResolvePath(m_configFilePath.c_str(), configFileFullPath, AZ_MAX_PATH_LEN);
        m_configFilePath = configFileFullPath;

        LoadConfigFile();
    }

    void ImGuiSaveFilePath::Deactivate()
    {
        if (!m_configFilePath.empty())
        {
            // We only report this message in Deactivate(), not inside SaveConfigFile(), to avoid spamming
            // this message when SaveConfigFile() is called in OnTick()
            AZ_TracePrintf("ImGuiAssetBrowser", "Saved settings to '%s'\n", m_configFilePath.c_str());

            SaveConfigFile();
        }
    }

    void ImGuiSaveFilePath::SetDefaultFolder(const AZStd::string& folderPath)
    {
        m_defaultFolder = folderPath;
    }

    void ImGuiSaveFilePath::SetDefaultFileName(const AZStd::string& fileNameNoExt)
    {
        m_defaultFileName = fileNameNoExt;
    }

    void ImGuiSaveFilePath::SetAvailableExtensions(const AZStd::vector<AZStd::string>& extensions)
    {
        AZ_Assert(!extensions.empty(), "At least one extension is required");
        m_availableExtensions = extensions;
    }

    bool ImGuiSaveFilePath::LoadConfigFile()
    {
        AZStd::unique_ptr<Config> configFile(AZ::Utils::LoadObjectFromFile<Config>(m_configFilePath));

        if (configFile)
        {
            m_config = *configFile;

            azstrncpy(m_filePath, AZ_ARRAY_SIZE(m_filePath), m_config.m_filePath.c_str(), m_config.m_filePath.size());

            if (m_config.m_currentExtension >= m_availableExtensions.size() || m_config.m_currentExtension < 0)
            {
                m_config.m_currentExtension = 0;
            }

            return true;
        }
        else
        {
            return false;
        }
    }

    void ImGuiSaveFilePath::SaveConfigFile()
    {
        m_config.m_filePath = m_filePath;

        if (m_configFilePath.empty())
        {
            AZ_Warning("ImGuiSaveFilePath", false, "m_configFilePath is not set. GUI state not saved.");
        }
        else if (!AZ::Utils::SaveObjectToFile(m_configFilePath, AZ::DataStream::ST_XML, &m_config))
        {
            AZ_Error("ImGuiSaveFilePath", false, "Failed to save '%s'", m_configFilePath.c_str());
        }
    }

    AZStd::string ImGuiSaveFilePath::GetNextAutoSaveFilePath()
    {
        auto makeDefaultFilePath = [this](uint32_t fileIndex)
        {
            AZStd::string defaultFilePath;
            AZStd::string defaultFileName = AZStd::string::format("%s_%i.%s", m_defaultFileName.c_str(), fileIndex, m_availableExtensions[m_config.m_currentExtension].c_str());
            AzFramework::StringFunc::Path::Join(m_defaultFolder.c_str(), defaultFileName.c_str(), defaultFilePath, true, false);
            return defaultFilePath;
        };

        AZStd::string defaultFilePath = makeDefaultFilePath(m_autoFileIndex);

        while (AZ::IO::LocalFileIO().Exists(defaultFilePath.c_str()))
        {
            defaultFilePath = makeDefaultFilePath(++m_autoFileIndex);
        }

        return defaultFilePath;
    }

    bool ImGuiSaveFilePath::GetExtension(void* data, int index, const char** out)
    {
        ImGuiSaveFilePath* thisPtr = reinterpret_cast<ImGuiSaveFilePath*>(data);
        if (index < thisPtr->m_availableExtensions.size())
        {
            *out = thisPtr->m_availableExtensions[index].c_str();
            return true;
        }
        else
        {
            return false;
        }
    }

    void ImGuiSaveFilePath::Tick(const WidgetSettings& widgetSettings)
    {
        bool configChanged = false;

        if (ImGui::TreeNodeEx(widgetSettings.m_labels.m_filePath, ImGuiTreeNodeFlags_DefaultOpen))
        {
            if (ImGui::Checkbox("Auto", &m_config.m_autoMode))
            {
                configChanged = true;
            }

            if (m_config.m_autoMode)
            {
                if (m_availableExtensions.size() > 1)
                {
                    if (ImGui::Combo("File Type", &m_config.m_currentExtension, &GetExtension, this, aznumeric_cast<int>(m_availableExtensions.size())))
                    {
                        // Search for a new "first available" filename
                        m_autoFileIndex = 0;

                        configChanged = true;
                    }
                }

                AZStd::string defaultFilePath = GetNextAutoSaveFilePath();
                azstrncpy(m_filePath, AZ_ARRAY_SIZE(m_filePath), defaultFilePath.c_str(), defaultFilePath.size());

                ImGui::Text("%s", defaultFilePath.c_str());
            }
            else
            {
                if (ImGui::InputText("##FilePath", m_filePath, AZ_MAX_PATH_LEN))
                {
                    configChanged = true;
                }
            }

            ImGui::TreePop();
        }

        if (configChanged)
        {
            SaveConfigFile();
        }
    }

    AZStd::string ImGuiSaveFilePath::GetSaveFilePath() const
    {
        return m_filePath;
    }
} // namespace ScriptAutomation
