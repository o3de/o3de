/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzFramework/Asset/AssetCatalogBus.h>
#include <AzToolsFramework/API/EditorAssetSystemAPI.h>
#include <Builder/ScriptCanvasBuilder.h>
#include <Builder/ScriptCanvasBuilderDataSystemBus.h>
#include <ScriptCanvas/Bus/EditorScriptCanvasBus.h>
#include <ScriptCanvas/Components/EditorScriptCanvasComponentSerializer.h>
#include <ScriptCanvas/Components/EditorDeprecationData.h>

namespace ScriptCanvasEditor
{
    /// <summary>
    /// Configuration provides user-facing facilities for selecting a ScriptCanvas source file, monitoring its status, and exposing its
    /// properties for configuration if possible.
    /// </summary>
    class Configuration final
        : public AzFramework::AssetCatalogEventBus::Handler
        , public ScriptCanvasBuilder::DataSystemSourceNotificationsBus::Handler
    {
        friend class AZ::EditorScriptCanvasComponentSerializer;
        friend class Deprecated::EditorScriptCanvasComponentVersionConverter;

    public:
        AZ_TYPE_INFO(Configuration, "{0F4D78A9-EF29-4D6A-AC5B-8F4E19B1A6EE}");

        static void Reflect(AZ::ReflectContext* context);

        Configuration();

        explicit Configuration(const SourceHandle& sourceHandle);
        
        ~Configuration();

        const ScriptCanvasBuilder::BuildVariableOverrides* CompileLatest();

        /// Will signal when the properties have been modified by the user, or when the source file has been changed.
        AZ::EventHandler<const Configuration&> ConnectToPropertiesChanged(AZStd::function<void(const Configuration&)>&& function) const;

        /// Will signal when the selected source file has been successfully compiled.
        AZ::EventHandler<const Configuration&> ConnectToSourceCompiled(AZStd::function<void(const Configuration&)>&& function) const;

        /// Will signal when the selected source file has failed to compile for any reason.
        AZ::EventHandler<const Configuration&> ConnectToSourceFailed(AZStd::function<void(const Configuration&)>&& function) const;

        /// Returns the user editable properties of the selected source. The properties could be empty.
        const ScriptCanvasBuilder::BuildVariableOverrides& GetOverrides() const;

        const SourceHandle& GetSource() const;

        bool HasSource() const;

        /// Provides a manual call to Refresh() with currently selected source file.
        void Refresh();

        /// Sets the selected file to the input sourceHandle, compiles latest, and sends all signals.
        void Refresh(const SourceHandle& sourceHandle);

    private:
        enum class BuildStatusValidation
        {
            Good,
            Bad,
            IncompatibleScript,
        };

        mutable AZ::Event<const Configuration&> m_eventPropertiesChanged;
        mutable AZ::Event<const Configuration&> m_eventSourceCompiled;
        mutable AZ::Event<const Configuration&> m_eventSourceFailed;
        
        SourceHandle m_sourceHandle;
        AZStd::string m_sourceName;
        ScriptCanvasBuilder::BuildVariableOverrides m_propertyOverrides;
        
        void ClearVariables();

        BuildStatusValidation CompileLatestInternal();

        void MergeWithLatestCompilation(const ScriptCanvasBuilder::BuildVariableOverrides& buildData);

        // on RPE source selection changed
        AZ::u32 OnEditorChangeSource();

        AZ::u32 OnEditorChangeProperties();

        void OpenEditor(const AZ::Data::AssetId&, const AZ::Data::AssetType&);

        // if result is good, merge results and update display
        void SourceFileChanged
            ( const ScriptCanvasBuilder::BuilderSourceResult& result
            , AZStd::string_view relativePath
            , AZStd::string_view scanFolder) override;

        // update the display icon for failure, save the values in the graph
        void SourceFileFailed(AZStd::string_view relativePath, AZStd::string_view scanFolder) override;

        // update the display icon for removal, save the values in the graph
        void SourceFileRemoved(AZStd::string_view relativePath, AZStd::string_view scanFolder) override;

        BuildStatusValidation ValidateBuildResult(const ScriptCanvasBuilder::BuilderSourceResult& result) const;

        // #scriptcanvas_component_extension ...
    public:
        bool AcceptsComponentScript() const;

        /// Some Scripts refer the 'self Entity Id', part of the Entity / Component extension of current ScriptCanvas scripting system.
        /// This allows programmers to enable or disable using such a script with this Configuraiton.
        void SetAcceptsComponentScript(bool value);

        AZ::EventHandler<const Configuration&> ConnectToIncompatilbleScript(AZStd::function<void(const Configuration&)>&& function) const;

    private:
        mutable AZ::Event<const Configuration&> m_eventIncompatibleScript;

        bool m_acceptsComponentScript = true;
        // ... #scriptcanvas_component_extension
        };
}
