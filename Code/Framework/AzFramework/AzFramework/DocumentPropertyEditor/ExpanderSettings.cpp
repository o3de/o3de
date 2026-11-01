/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/IO/SystemFile.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Settings/SettingsRegistry.h>
#include <AzCore/Settings/SettingsRegistryMergeUtils.h>
#include <AzCore/Utils/Utils.h>

#include <AzFramework/DocumentPropertyEditor/DocumentAdapter.h>
#include <AzFramework/DocumentPropertyEditor/ExpanderSettings.h>
#include <AzFramework/DocumentPropertyEditor/PropertyEditorNodes.h>

namespace AZ::DocumentPropertyEditor
{
    //! Everything private to this file (a named namespace: a unity build joins this file with others, where anonymous ones merge)
    namespace ExpanderSettingsInternal
    {
        // ------------------------------------------------------------------------------
        // Settings file merges: the expander settings of one property editor share one settings file, and the registry holds its
        // contents from the first merge on (a save writes it back to the file), so the file is merged again only when it changed on
        // disk. The registry keeps the file's stamp from its last merge or save under a runtime key that is never saved; a new
        // registry has none.
        // ------------------------------------------------------------------------------

        constexpr const char* MergedSettingsFilesKey = "/O3DE/Runtime/Internal/DocumentPropertyEditor/MergedSettingsFiles";

        //! The registry key that keeps the stamp of a settings file
        AZStd::string StampKey(const AZ::IO::Path& settingsFilepath)
        {
            const AZStd::string_view fileName = settingsFilepath.Filename().Native();
            return AZStd::string::format("%s/%.*s", MergedSettingsFilesKey, AZ_STRING_ARG(fileName));
        }

        //! The file's change time and size, as text ("0:0" for a file that does not exist)
        AZ::SettingsRegistryInterface::FixedValueString Stamp(const AZ::IO::Path& settingsFilepath)
        {
            AZ::IO::FixedMaxPath fullPath = AZ::Utils::GetProjectPath();
            fullPath /= settingsFilepath;
            return AZ::SettingsRegistryInterface::FixedValueString::format(
                "%llu:%llu",
                static_cast<unsigned long long>(AZ::IO::SystemFile::ModificationTime(fullPath.c_str())),
                static_cast<unsigned long long>(AZ::IO::SystemFile::Length(fullPath.c_str())));
        }
    } // namespace ExpanderSettingsInternal

    ExpanderSettings::ExpanderSettings(
        const AZ::DocumentPropertyEditor::DocumentAdapter* adapter,
        const AZStd::string& settingsRegistryKey,
        const AZStd::string& propertyEditorName)
        : m_shouldSettingsPersist(true)
        , m_adapter(adapter)
    {
        if (!settingsRegistryKey.empty() && !propertyEditorName.empty())
        {
            m_settingsRegistryBasePath = AZStd::string::format("%s/%s", RootSettingsRegistryPath, propertyEditorName.c_str());
            m_fullSettingsRegistryPath = AZStd::string::format("%s/%s", m_settingsRegistryBasePath.c_str(), settingsRegistryKey.c_str());

            m_settingsFilepath.Append(RootSettingsFilepath)
                .Append(AZStd::string::format("%s_settings", propertyEditorName.c_str()))
                .ReplaceExtension(SettingsRegistrar::SettingsRegistryFileExt);

            m_wereSettingsLoaded = LoadExpanderStates();

            if (m_wereSettingsLoaded)
            {
                SetCleanExpanderStateCallback(
                [this](ExpanderSettings::ExpanderStateMap& storedStates)
                {
                    const auto& rootValue = m_adapter->GetContents();
                    auto numErased = AZStd::erase_if(
                        storedStates,
                        [&rootValue](const AZStd::pair<PathType, bool>& statePair)
                        {
                            return !rootValue.FindChild(AZ::Dom::Path(statePair.first.Native()));
                        });
                    return numErased > 0;
                });
            }
        }
    }

    ExpanderSettings::~ExpanderSettings()
    {
        SaveAndCleanExpanderStates();
    }

    void ExpanderSettings::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (serializeContext)
        {
            serializeContext->Class<ExpanderSettings>()->Version(0)->Field(
                "ExpandedElements", &ExpanderSettings::m_expandedElementStates);
        }
    }

    ExpanderSettings::PathType ExpanderSettings::GetStringPathForDomPath(const AZ::Dom::Path& rowPath) const
    {
        return PathType(rowPath.ToString(), AZ::IO::PosixPathSeparator);
    }

    void ExpanderSettings::SaveExpanderStates()
    {
        m_settingsRegistrar.StoreObjectSettings(m_fullSettingsRegistryPath, this);

        // Configuration that the dumper util will use when collecting our data from the SettingsRegistry
        AZ::SettingsRegistryMergeUtils::DumperSettings dumperSettings;
        dumperSettings.m_prettifyOutput = true;
        dumperSettings.m_jsonPointerPrefix = m_settingsRegistryBasePath;

        auto outcome = m_settingsRegistrar.SaveSettingsToFile(m_settingsFilepath, dumperSettings, m_settingsRegistryBasePath);
        if (!outcome.IsSuccess())
        {
            AZ_Warning("DocumentPropertyEditorSettings", false, outcome.GetError().c_str());
        }
        else if (AZ::SettingsRegistryInterface* registry = AZ::SettingsRegistry::Get())
        {
            // The file now holds what the registry holds: it is not merged back (Settings file merges, above)
            registry->Set(ExpanderSettingsInternal::StampKey(m_settingsFilepath), ExpanderSettingsInternal::Stamp(m_settingsFilepath));
        }
    }

    bool ExpanderSettings::LoadExpanderStates()
    {
        using namespace ExpanderSettingsInternal;

        // Merge the settings file, unless the registry already holds it as it is on disk (Settings file merges, above)
        AZ::SettingsRegistryInterface* registry = AZ::SettingsRegistry::Get();
        const AZStd::string stampKey = StampKey(m_settingsFilepath);
        const AZ::SettingsRegistryInterface::FixedValueString stamp = Stamp(m_settingsFilepath);
        AZ::SettingsRegistryInterface::FixedValueString mergedStamp;
        if (!registry || !registry->Get(mergedStamp, stampKey) || mergedStamp != stamp)
        {
            auto loadOutcome = m_settingsRegistrar.LoadSettingsFromFile(m_settingsFilepath);
            if (!loadOutcome.IsSuccess())
            {
                AZ_Warning("DocumentPropertyEditorSettings", false, loadOutcome.GetError().c_str());
                return false;
            }
            if (registry)
            {
                registry->Set(stampKey, stamp);
            }
        }

        bool loaded = false;
        auto getOutcome = m_settingsRegistrar.GetObjectSettings(this, m_fullSettingsRegistryPath);
        if (getOutcome.IsSuccess())
        {
            loaded = getOutcome.GetValue();
        }
        else
        {
            AZ_Warning("DocumentPropertyEditorSettings", false, getOutcome.GetError().c_str());
        }

        return loaded;
    }

    void ExpanderSettings::SaveAndCleanExpanderStates()
    {
        // Attempt to remove old settings from registry if the local state was successfully cleaned
        // This way we save and dump to file the most accurate expander settings
        if (m_cleanExpanderStateCallback && m_cleanExpanderStateCallback(m_expandedElementStates))
        {
            if (!m_settingsRegistrar.RemoveSettingFromRegistry(m_fullSettingsRegistryPath))
            {
                AZ_Warning("DocumentPropertyEditorSettings", false,
                    "Failed to clean registry state %s before saving", m_fullSettingsRegistryPath.c_str());
            }
        }

        if (m_shouldSettingsPersist && !m_expandedElementStates.empty())
        {
            SaveExpanderStates();
        }
    }

    void ExpanderSettings::SetExpanderStateForRow(const AZ::Dom::Path& rowPath, bool isExpanded)
    {
        m_expandedElementStates[GetStringPathForDomPath(rowPath)] = isExpanded;
    }

    bool ExpanderSettings::GetExpanderStateForRow(const AZ::Dom::Path& rowPath)
    {
        auto path = GetStringPathForDomPath(rowPath);
        if (m_expandedElementStates.contains(path))
        {
            return m_expandedElementStates[path];
        }
        return false;
    }

    bool ExpanderSettings::HasSavedExpanderStateForRow(const AZ::Dom::Path& rowPath) const
    {
        return m_expandedElementStates.contains(GetStringPathForDomPath(rowPath));
    }

    void ExpanderSettings::RemoveExpanderStateForRow(const AZ::Dom::Path& rowPath)
    {
        m_expandedElementStates.erase(GetStringPathForDomPath(rowPath));
    }

    void ExpanderSettings::SetCleanExpanderStateCallback(CleanExpanderStateCallback function)
    {
        m_cleanExpanderStateCallback = function;
    }

    LabeledRowDPEExpanderSettings::LabeledRowDPEExpanderSettings(
        const AZ::DocumentPropertyEditor::DocumentAdapter* adapter,
        const AZStd::string& settingsRegistryKey,
        const AZStd::string& propertyEditorName)
        : ExpanderSettings(adapter, settingsRegistryKey, propertyEditorName)
    {
        // Note: labeled row adapters, like the one used in the inspector, can have so many differently named/pathed
        // elements visible depending on internal state, that it is undesirable to clean them out when switching entities
        m_cleanExpanderStateCallback = nullptr;
    }

    ExpanderSettings::PathType LabeledRowDPEExpanderSettings::GetStringPathForDomPath(const AZ::Dom::Path& rowPath) const
    {
        AZ_Assert(m_adapter, "must have a valid owning editor to resolve paths!");
        PathType newPath(AZ::IO::PosixPathSeparator);

        const auto& fullContents = m_adapter->GetContents();
        AZ::Dom::Path subPath;
        for (auto pathComponent : rowPath)
        {
            subPath.Push(pathComponent);
            auto rowValue = fullContents[subPath];

            AZStd::string_view subString{"<nolabel>"};
            for (auto arrayIter = rowValue.ArrayBegin(), endIter = rowValue.ArrayEnd(); arrayIter != endIter; ++arrayIter)
            {
                auto& currChild = *arrayIter;
                auto valueMember = currChild.FindMember("Value");
                if (valueMember != currChild.MemberEnd() && valueMember->second.IsString())
                {
                    subString = valueMember->second.GetString();
                    break;
                }
            }
            newPath.Append(subString);
        }
        return newPath;
    }

} // namespace AZ::DocumentPropertyEditor
