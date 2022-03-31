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
    /*! Configuration
    The user facing Editor Component for interacting with ScriptCanvas.
    Per graph instance variables values are stored here and injected into the runtime ScriptCanvas component in BuildGameEntity.
    */
    class Configuration final
        : public AzFramework::AssetCatalogEventBus::Handler
        , public ScriptCanvasBuilder::DataSystemNotificationsBus::Handler
    {
        friend class AZ::EditorScriptCanvasComponentSerializer;
        friend class Deprecated::EditorScriptCanvasComponentVersionConverter;

    public:
        AZ_TYPE_INFO(Configuration, "{0F4D78A9-EF29-4D6A-AC5B-8F4E19B1A6EE}");

        static void Reflect(AZ::ReflectContext* context);

        Configuration();

        Configuration(const SourceHandle& sourceHandle);
        
        ~Configuration();

        const ScriptCanvasBuilder::BuildVariableOverrides* CompileLatest();

        const SourceHandle& GetSource() const;

        bool HasSource() const;

        void Refresh();

        void Refresh(const SourceHandle& sourceHandle);

    protected:
        void ClearVariables();

        void MergeWithLatestCompilation(const ScriptCanvasBuilder::BuildVariableOverrides& buildData);

        void OpenEditor(const AZ::Data::AssetId&, const AZ::Data::AssetType&);

        // on RPE source selection changed
        AZ::u32 OnEditorChangeNotify();

        // if result is good, merge results and update display
        void SourceFileChanged
            ( const ScriptCanvasBuilder::BuildResult& result
            , AZStd::string_view relativePath
            , AZStd::string_view scanFolder) override;

        // update the display icon for failure, save the values in the graph
        void SourceFileFailed(AZStd::string_view relativePath, AZStd::string_view scanFolder) override;

        // update the display icon for removal, save the values in the graph
        void SourceFileRemoved(AZStd::string_view relativePath, AZStd::string_view scanFolder) override;

    private:
        AZStd::string m_sourceName;
        ScriptCanvasBuilder::BuildVariableOverrides m_propertyOverrides;
        SourceHandle m_sourceHandle;
    };
}
