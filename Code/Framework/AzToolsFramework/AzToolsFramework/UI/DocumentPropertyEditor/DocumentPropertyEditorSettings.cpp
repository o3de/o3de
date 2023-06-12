/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "DocumentPropertyEditor.h"

#include <AzCore/Settings/SettingsRegistryMergeUtils.h>

namespace AzToolsFramework
{
    bool DocumentPropertyEditorSettings::ExpanderStateMapComparator::operator()(const AZ::Dom::Path& lhsPath, const AZ::Dom::Path& rhsPath)
    {
        auto ComparePathEntryEqual = [](const AZ::Dom::PathEntry& lhsEntry, const AZ::Dom::PathEntry& rhsEntry)
        {
            return lhsEntry == rhsEntry;
        };

        // Find the PathEntry where the two Path objects differ, if any
        const auto& lhsPathEntries = lhsPath.GetEntries();
        const auto& rhsPathEntries = rhsPath.GetEntries();
        auto [lhsPathIter, rhsPathIter] = AZStd::mismatch(
            lhsPathEntries.begin(), lhsPathEntries.end(), rhsPathEntries.begin(), rhsPathEntries.end(), ComparePathEntryEqual);

        if (lhsPathIter != lhsPathEntries.end() && rhsPathIter != rhsPathEntries.end())
        {
            AZ_Assert(lhsPathIter->IsIndex() && rhsPathIter->IsIndex(),
                "The DocumentPropertyEditorSettings expander state map only supports index path entries.");

            // If the paths differ then compare the differing PathEntries
            return lhsPathIter->GetIndex() < rhsPathIter->GetIndex();
        }
        else
        {
            // If one Path is a prefix of the other, or the paths are identical, then the shorter
            // Path is the smaller Path
            return rhsPathEntries.size() > lhsPathEntries.size();
        }
    }

    DocumentPropertyEditorSettings::DocumentPropertyEditorSettings(
        const AZStd::string& settingsRegistryKey,
        const AZStd::string& propertyEditorName)
    {
        m_settingsRegistryBasePath = AZStd::string::format("%s/%s", RootSettingsRegistryPath, propertyEditorName.c_str());
        m_fullSettingsRegistryPath = AZStd::string::format("%s/%s", m_settingsRegistryBasePath.c_str(), settingsRegistryKey.c_str());

        m_settingsFilepath.Append(RootSettingsFilepath)
            .Append(AZStd::string::format("%s_settings", propertyEditorName.c_str()))
            .ReplaceExtension(SettingsRegistrar::SettingsRegistryFileExt);

        m_wereSettingsLoaded = LoadExpanderStates();
        m_shouldSettingsPersist = true;
    }

    DocumentPropertyEditorSettings::~DocumentPropertyEditorSettings()
    {
        SaveAndCleanExpanderStates();
    }

    void DocumentPropertyEditorSettings::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (serializeContext)
        {
            serializeContext->Class<DocumentPropertyEditorSettings>()->Version(0)
                ->Field("ExpandedElements", &DocumentPropertyEditorSettings::m_expandedElementStates);
        }
    }

    void DocumentPropertyEditorSettings::SaveExpanderStates()
    {
        m_settingsRegistrar.StoreObjectSettings(m_fullSettingsRegistryPath, this);

        // Configuration that the dumper util will use when collecting our data from the SettingsRegistry
        AZ::SettingsRegistryMergeUtils::DumperSettings dumperSettings;
        dumperSettings.m_prettifyOutput = true;
        dumperSettings.m_includeFilter = [pathFilter = m_settingsRegistryBasePath](AZStd::string_view path)
        {
            return AZ::SettingsRegistryMergeUtils::IsPathAncestorDescendantOrEqual(pathFilter, path);
        };
        dumperSettings.m_jsonPointerPrefix = m_settingsRegistryBasePath;

        auto outcome = m_settingsRegistrar.SaveSettingsToFile(m_settingsFilepath, dumperSettings, m_settingsRegistryBasePath);
        if (!outcome.IsSuccess())
        {
            AZ_Warning("DocumentPropertyEditorSettings", false, outcome.GetError().c_str());
        }
    }

    bool DocumentPropertyEditorSettings::LoadExpanderStates()
    {
        bool loaded = false;
        auto loadOutcome = m_settingsRegistrar.LoadSettingsFromFile(m_settingsFilepath);
        if (loadOutcome.IsSuccess())
        {
            auto getOutcome = m_settingsRegistrar.GetObjectSettings(this, m_fullSettingsRegistryPath);
            if (getOutcome.IsSuccess())
            {
                loaded = getOutcome.GetValue();   
            }
            else
            {
                AZ_Warning("DocumentPropertyEditorSettings", false, getOutcome.GetError().c_str());
            }
        }
        else
        {
            AZ_Warning("DocumentPropertyEditorSettings", false, loadOutcome.GetError().c_str());
        }

        return loaded;
    }

    void DocumentPropertyEditorSettings::SaveAndCleanExpanderStates()
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

    void DocumentPropertyEditorSettings::SetExpanderStateForRow(const AZ::Dom::Path& rowPath, bool isExpanded)
    {
        m_expandedElementStates[rowPath] = isExpanded;
    }

    bool DocumentPropertyEditorSettings::GetExpanderStateForRow(const AZ::Dom::Path& rowPath)
    {
        if (m_expandedElementStates.contains(rowPath))
        {
            return m_expandedElementStates[rowPath];
        }
        return false;
    }

    bool DocumentPropertyEditorSettings::HasSavedExpanderStateForRow(const AZ::Dom::Path& rowPath) const
    {
        return m_expandedElementStates.contains(rowPath);
    }

    void DocumentPropertyEditorSettings::RemoveExpanderStateForRow(const AZ::Dom::Path& rowPath)
    {
        m_expandedElementStates.erase(rowPath);
    }

    bool DocumentPropertyEditorSettings::WereSettingsLoaded() const
    {
        return m_wereSettingsLoaded;
    }

    void DocumentPropertyEditorSettings::SetCleanExpanderStateCallback(CleanExpanderStateCallback function)
    {
        m_cleanExpanderStateCallback = function;
    }
} // namespace AzToolsFramework
