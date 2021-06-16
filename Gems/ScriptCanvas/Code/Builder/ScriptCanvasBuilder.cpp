/*
* All or portions of this file Copyright(c) Amazon.com, Inc.or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
*or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
*WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#include <AzToolsFramework/API/EditorAssetSystemAPI.h>
#include <Builder/ScriptCanvasBuilder.h>
#include <Builder/ScriptCanvasBuilderWorker.h>
#include <ScriptCanvas/Assets/ScriptCanvasAsset.h>
#include <ScriptCanvas/Grammar/AbstractCodeModel.h>

namespace ScriptCanvasBuilderCpp
{
    void AppendTabs(AZStd::string& result, size_t depth)
    {
        for (size_t i = 0; i < depth; ++i)
        {
            result += "\t";
        }
    }
}

namespace ScriptCanvasBuilder
{
    EditorAssetTree* EditorAssetTree::ModRoot()
    {
        if (!m_parent)
        {
            return this;
        }

        return m_parent->ModRoot();
    }

    void EditorAssetTree::SetParent(EditorAssetTree& parent)
    {
        m_parent = &parent;
    }

    AZStd::string EditorAssetTree::ToString(size_t depth) const
    {
        AZStd::string result;
        ScriptCanvasBuilderCpp::AppendTabs(result, depth);
        result += m_asset.GetId().ToString<AZStd::string>();
        result += m_asset.GetHint();
        depth += m_dependencies.empty() ? 0 : 1;

        for (const auto& dependency : m_dependencies)
        {
            result += "\n";
            ScriptCanvasBuilderCpp::AppendTabs(result, depth);
            result += dependency.ToString(depth);
        }

        return result;
    }

    AZ::Outcome<EditorAssetTree, AZStd::string> LoadEditorAssetTree(AZ::Data::AssetId editorAssetId, AZStd::string_view assetHint, EditorAssetTree* parent)
    {
        EditorAssetTree result;
        AZ::Data::AssetInfo assetInfo;
        AZStd::string watchFolder;
        bool resultFound = false;

        if (!AzToolsFramework::AssetSystemRequestBus::FindFirstHandler())
        {
            return AZ::Failure(AZStd::string("LoadEditorAssetTree found no handler for AzToolsFramework::AssetSystemRequestBus."));
        }

        AzToolsFramework::AssetSystemRequestBus::BroadcastResult
            ( resultFound
            , &AzToolsFramework::AssetSystem::AssetSystemRequest::GetSourceInfoBySourceUUID
            , editorAssetId.m_guid
            , assetInfo
            , watchFolder);

        if (!resultFound)
        {
            return AZ::Failure(AZStd::string::format("LoadEditorAssetTree failed to get engine relative path from %s-%s.", editorAssetId.ToString<AZStd::string>().c_str(), assetHint.data()));
        }

        AZStd::vector<AZ::Data::AssetId> dependentAssets;
        
        auto filterCB = [&dependentAssets](const AZ::Data::AssetFilterInfo& filterInfo)->bool
        {
            if (filterInfo.m_assetType == azrtti_typeid<ScriptCanvas::SubgraphInterfaceAsset>())
            {
                dependentAssets.push_back(AZ::Data::AssetId(filterInfo.m_assetId.m_guid, 0));
            }
            else if (filterInfo.m_assetType == azrtti_typeid<ScriptCanvasEditor::ScriptCanvasAsset>())
            {
                dependentAssets.push_back(filterInfo.m_assetId);
            }

            return true;
        };

        auto loadAssetOutcome = ScriptCanvasBuilder::LoadEditorAsset(assetInfo.m_relativePath, filterCB);
        if (!loadAssetOutcome.IsSuccess())
        {
            return AZ::Failure(AZStd::string::format("LoadEditorAssetTree failed to load graph from %s-%s: %s", editorAssetId.ToString<AZStd::string>().c_str(), assetHint.data(), loadAssetOutcome.GetError().c_str()));
        }

        for (auto& dependentAsset : dependentAssets)
        {
            auto loadDependentOutcome = LoadEditorAssetTree(dependentAsset, "", &result);
            if (!loadDependentOutcome.IsSuccess())
            {
                return AZ::Failure(AZStd::string::format("LoadEditorAssetTree failed to load dependent graph from %s-%s: %s", editorAssetId.ToString<AZStd::string>().c_str(), assetHint.data(), loadDependentOutcome.GetError().c_str()));
            }

            result.m_dependencies.push_back(loadDependentOutcome.TakeValue());
        }

        if (parent)
        {
            result.SetParent(*parent);
        }

        result.m_asset = loadAssetOutcome.TakeValue();

        return AZ::Success(result);
    }

    AZ::Outcome<BuildVariableOverrides, AZStd::string> ParseEditorAssetTree(const EditorAssetTree& editorAssetTree)
    {
        auto buildEntity = editorAssetTree.m_asset->GetScriptCanvasEntity();
        if (!buildEntity)
        {
            return AZ::Failure(AZStd::string("No entity from source asset"));
        }

        auto parseOutcome = ScriptCanvasBuilder::ParseGraph(*buildEntity, "");
        if (!parseOutcome.IsSuccess())
        {
            return AZ::Failure(AZStd::string("graph failed to parse"));
        }

        BuildVariableOverrides result;
        result.m_source = editorAssetTree.m_asset;

        ScriptCanvas::Grammar::AbstractCodeModelConstPtr model = parseOutcome.GetValue();

        ScriptCanvas::VariableData* variableData = nullptr; // get this from the entity
        result.PopulateFromParsedResults(model->GetRuntimeInputs(), *variableData);
        // set all these in build game data
        // originalVarNameValuePair->m_graphVariable.SetScriptInputControlVisibility(AZ::Edit::PropertyVisibility::Hide);
        // originalVarNameValuePair->m_graphVariable.SetAllowSignalOnChange(false);

        // recurse...
        for (auto& dependentAsset : editorAssetTree.m_dependencies)
        {
            // #functions2 provide an identifier for the node/variable in the source that caused the dependency. the root will not have one.
            auto parseDependentOutcome = ParseEditorAssetTree(dependentAsset);
            if (!parseDependentOutcome.IsSuccess())
            {
                return AZ::Failure(AZStd::string::format
                    ( "ParseEditorAssetTree failed to parse dependent graph from %s-%s: %s"
                    , dependentAsset.m_asset.GetId().ToString<AZStd::string>().c_str()
                    , dependentAsset.m_asset.GetHint().c_str()
                    , parseDependentOutcome.GetError().c_str()));
            }

            result.m_dependencies.push_back(parseDependentOutcome.TakeValue());
        }
        return AZ::Success(result);
    }

}
