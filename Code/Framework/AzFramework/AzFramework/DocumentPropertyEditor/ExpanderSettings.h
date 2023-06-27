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
#include <AzCore/std/containers/unordered_map.h>
#include <AzFramework/DocumentPropertyEditor/DocumentAdapter.h>
#include <AzFramework/DocumentPropertyEditor/SettingsRegistrar.h>

namespace AZ
{
    class ReflectContext;
}

namespace AZ::DocumentPropertyEditor
{
    class DocumentPropertyEditor;

    //! This serializable class stores and loads the DocumentPropertyEditor settings such as tree node expansion state.
    class ExpanderSettings
    {
    public:
        AZ_RTTI(ExpanderSettings, "{7DECB0A1-A1AB-41B2-B31F-E52D3C3014A6}");

        using PathType = AZ::IO::Path;
        using ExpanderStateMap = AZStd::unordered_map<PathType, bool>;
        using CleanExpanderStateCallback = AZStd::function<bool(ExpanderStateMap&)>;

        ExpanderSettings(
            const AZ::DocumentPropertyEditor::DocumentAdapter* adapter = nullptr,
            //! if settingsRegistryKey or propertyEditorName is empty, the expander state will only be stored in-memory (temporary) 
            const AZStd::string& settingsRegistryKey = AZStd::string(),
            const AZStd::string& propertyEditorName = AZStd::string());

        virtual ~ExpanderSettings();

        static void Reflect(AZ::ReflectContext* context);

        void SetExpanderStateForRow(const AZ::Dom::Path& rowPath, bool isExpanded);
        bool GetExpanderStateForRow(const AZ::Dom::Path& rowPath);
        bool HasSavedExpanderStateForRow(const AZ::Dom::Path& rowPath) const;
        void RemoveExpanderStateForRow(const AZ::Dom::Path& rowPath);

        bool WereSettingsLoaded() const { return m_wereSettingsLoaded; };
        bool ShouldEraseStateWhenRowRemoved() const
        {
            return m_eraseStateWhenRowRemoved;
        }

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
        bool m_eraseStateWhenRowRemoved = false;

        SettingsRegistrar m_settingsRegistrar;

        AZ::IO::Path m_settingsFilepath;

        AZStd::string m_fullSettingsRegistryPath;
        AZStd::string m_settingsRegistryBasePath;
        const AZ::DocumentPropertyEditor::DocumentAdapter* m_adapter;
    };

    class LabeledRowDPEExpanderSettings : public ExpanderSettings
    {
    public:
        LabeledRowDPEExpanderSettings(
            const AZ::DocumentPropertyEditor::DocumentAdapter* adapter = nullptr,
            //! if settingsRegistryKey or propertyEditorName is empty, the expander state will only be stored in-memory (temporary)
            const AZStd::string& settingsRegistryKey = AZStd::string(),
            const AZStd::string& propertyEditorName = AZStd::string());

    protected:
        PathType GetStringPathForDomPath(const AZ::Dom::Path& rowPath) const override;
    };

} // namespace AZ::DocumentPropertyEditor
