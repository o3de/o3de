/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Asset/AssetCommon.h>
#include <ScriptCanvas/Asset/RuntimeAsset.h>
#include <ScriptCanvas/Grammar/PrimitivesDeclarations.h>
#include <ScriptCanvas/Variable/VariableCore.h>
#include <AzCore/Asset/AssetCommon.h>


namespace ScriptCanvasEditor
{
    class EditorAssetTree;
}

namespace ScriptCanvasBuilder
{
    // make asset for this... build it in the AP...save it out...
    // load from asset, and DO NOT build it out during edit and prefab build time
    // instead, listen for changes to the asset...and...populate from parsed results there
    class BuildVariableOverrides
    {
    public:
        AZ_TYPE_INFO(BuildVariableOverrides, "{8336D44C-8EDC-4C28-AEB4-3420D5FD5AE2}");
        AZ_CLASS_ALLOCATOR(BuildVariableOverrides, AZ::SystemAllocator, 0);

        static void Reflect(AZ::ReflectContext* reflectContext);

        void Clear();

        // use this to preserve old values that may have been overridden on the instance, and are still valid in the parsed graph
        void CopyPreviousOverriddenValues(const BuildVariableOverrides& source);

        bool IsEmpty() const;

        // use this to initialize the new data, and make sure they have a editor graph variable for proper editor display
        void PopulateFromParsedResults(ScriptCanvas::Grammar::AbstractCodeModelConstPtr abstractCodeModel, const ScriptCanvas::VariableData& variables);

        void SetHandlesToDescription();

        // #functions2 provide an identifier for the node/variable in the source that caused the dependency. the root will not have one.
        ScriptCanvasEditor::SourceHandle m_source;
        // make the above...what...an asset...reference...? someway to recurse the structure
        // but it won't need to be a source reference anymore...
                
        // all of the variables here are overrides
        AZStd::vector<ScriptCanvas::GraphVariable> m_variables;
        // the values here may or may not be overrides
        AZStd::vector<AZStd::pair<ScriptCanvas::VariableId, AZ::EntityId>> m_entityIds;
        // these two variable lists are all that gets exposed to the edit context
        AZStd::vector<ScriptCanvas::GraphVariable> m_overrides;
        AZStd::vector<ScriptCanvas::GraphVariable> m_overridesUnused;
        AZStd::vector<BuildVariableOverrides> m_dependencies;
    };

    // copy the variables overridden during editor / prefab build time back to runtime data
    ScriptCanvas::RuntimeDataOverrides ConvertToRuntime(const BuildVariableOverrides& overrides);

    AZ::Outcome<BuildVariableOverrides, AZStd::string> ParseEditorAssetTree(const ScriptCanvasEditor::EditorAssetTree& editorAssetTree);

    class BuildVariableOverridesData;

    class BuildVariableOverridesAssetDescription
        : public ScriptCanvas::AssetDescription
    {
    public:

        AZ_TYPE_INFO(BuildVariableOverridesAssetDescription, "{80E1B917-E460-4167-8D1C-BBC40CCBE6C2}");

        BuildVariableOverridesAssetDescription()
            : ScriptCanvas::AssetDescription(
                azrtti_typeid<BuildVariableOverridesData>(),
                "Script Canvas Build",
                "Script Canvas Builder Data",
                "@projectroot@/scriptcanvas",
                ".scriptcanvas_builder",
                "Script Canvas Runtime",
                "Untitled-%i",
                "Script Canvas Builder Files (*.scriptcanvas_builder)",
                "Script Canvas Builder",
                "Script Canvas Builder",
                "Icons/ScriptCanvas/Viewport/ScriptCanvas.png",
                AZ::Color(1.0f, 0.0f, 0.0f, 1.0f),
                false
            )
        {}
    };

    class BuildVariableOverridesData
        : public AZ::Data::AssetData
    {
    public:
        AZ_RTTI(BuildVariableOverridesData, "{2264B9CC-20D4-4EAF-96AB-EE60369061B4}", AZ::Data::AssetData);
        AZ_CLASS_ALLOCATOR(BuildVariableOverridesData, AZ::SystemAllocator, 0);

        static const char* GetFileExtension() { return "scriptcanvas_builder"; }
        static const char* GetFileFilter() { return "*.scriptcanvas_builder"; }

        BuildVariableOverrides m_overrides;
    };

    class BuildVariableOverridesAssetHandler
        : public AzFramework::GenericAssetHandler<BuildVariableOverridesData>
    {
    public:
        AZ_CLASS_ALLOCATOR(BuildVariableOverridesAssetHandler, AZ::SystemAllocator, 0);
        AZ_RTTI(BuildVariableOverridesAssetHandler, "{3653D924-B0B6-450A-B96C-7907BC9EE279}", AZ::Data::AssetHandler);

        BuildVariableOverridesAssetHandler()
            : AzFramework::GenericAssetHandler<BuildVariableOverridesData>
                ( "Script Canvas Build Asset"
                , "Script Canvas Build"
                , "scriptcanvas_builder")
        {}
    };
}
