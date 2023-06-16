/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/RTTI/TypeInfoSimple.h>
#include <AzCore/RTTI/RTTIMacros.h>
#include <AzToolsFramework/UI/DocumentPropertyEditor/SettingsRegistrar.h>

namespace AZ
{
    class ReflectContext;
}

namespace AzToolsFramework
{
    //! This serializable class stores and loads the DocumentPropertyEditor settings such as tree node expansion state.
    class DocumentPropertyEditorSettings
    {
    public:
        AZ_RTTI(DocumentPropertyEditorSettings, "{7DECB0A1-A1AB-41B2-B31F-E52D3C3014A6}");

        using PathType = AZ::IO::BasicPath<AZStd::string>;
        using ExpanderStateMap = AZStd::unordered_map<PathType, bool>;
        using CleanExpanderStateCallback = AZStd::function<bool(ExpanderStateMap&)>;

        //! Use this constructor when temporary in-memory only storage is desired
        DocumentPropertyEditorSettings() = default;
        //! Use this constructor when storing settings in a persistent registry file on-disk
        DocumentPropertyEditorSettings(const AZStd::string& settingsRegistryKey, const AZStd::string& propertyEditorName);

        virtual ~DocumentPropertyEditorSettings();

        static void Reflect(AZ::ReflectContext* context);

        void SetExpanderStateForRow(const AZ::Dom::Path& rowPath, bool isExpanded);
        bool GetExpanderStateForRow(const AZ::Dom::Path& rowPath);
        bool HasSavedExpanderStateForRow(const AZ::Dom::Path& rowPath) const;
        void RemoveExpanderStateForRow(const AZ::Dom::Path& rowPath);

        bool WereSettingsLoaded() const { return m_wereSettingsLoaded; };

        void SetCleanExpanderStateCallback(CleanExpanderStateCallback function);

        //! Root filepath for DocumentPropertyEditor settings files
        static constexpr const char* RootSettingsFilepath = "user/Registry/DocumentPropertyEditor";

        //! Root SettingsRegistry path where DPE settings are stored
        static constexpr const char* RootSettingsRegistryPath = "/O3DE/DocumentPropertyEditor";

        //! Serialized map of expanded element states
        ExpanderStateMap m_expandedElementStates;

    protected:
        virtual PathType GetStringPathForDomPath(const AZ::Dom::Path& rowPath) const;
        void SaveExpanderStates();
        bool LoadExpanderStates();

        void SaveAndCleanExpanderStates();

        //! Optional callback to clean locally stored state before saving
        CleanExpanderStateCallback m_cleanExpanderStateCallback;

        bool m_wereSettingsLoaded = false;
        bool m_shouldSettingsPersist = false;

        SettingsRegistrar m_settingsRegistrar;

        AZ::IO::Path m_settingsFilepath;

        AZStd::string m_fullSettingsRegistryPath;
        AZStd::string m_settingsRegistryBasePath;
    };

    class LabeledRowDPEExpanderSettings : public DocumentPropertyEditorSettings
    {
    protected:
        PathType GetStringPathForDomPath(const AZ::Dom::Path& rowPath) const override;
    };

} // namespace AzToolsFramework
