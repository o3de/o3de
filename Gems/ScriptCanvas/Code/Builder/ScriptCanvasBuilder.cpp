/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzToolsFramework/API/EditorAssetSystemAPI.h>
#include <Builder/ScriptCanvasBuilder.h>
#include <Builder/ScriptCanvasBuilderWorker.h>
#include <ScriptCanvas/Assets/ScriptCanvasAsset.h>
#include <ScriptCanvas/Components/EditorGraphVariableManagerComponent.h>
#include <ScriptCanvas/Grammar/AbstractCodeModel.h>
#include <AzCore/Asset/AssetSerializer.h>

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
    void BuildVariableOverrides::Clear()
    {
        m_source.Reset();
        m_variables.clear();
        m_overrides.clear();
        m_overridesUnused.clear();
        m_entityIds.clear();
        m_dependencies.clear();
    }

    void BuildVariableOverrides::CopyPreviousOverriddenValues(const BuildVariableOverrides& source)
    {
        auto copyPreviousIfFound = [](ScriptCanvas::GraphVariable& overriddenValue, const AZStd::vector<ScriptCanvas::GraphVariable>& source)
        {
            if (auto iter = AZStd::find_if(source.begin(), source.end(), [&overriddenValue](const auto& candidate) { return candidate.GetVariableId() == overriddenValue.GetVariableId(); });
            iter != source.end())
            {
                overriddenValue.DeepCopy(*iter);
                overriddenValue.SetScriptInputControlVisibility(AZ::Edit::PropertyVisibility::Hide);
                overriddenValue.SetAllowSignalOnChange(false);
                return true;
            }
            else
            {
                return false;
            }
        };

        for (auto& overriddenValue : m_overrides)
        {
            if (!copyPreviousIfFound(overriddenValue, source.m_overrides))
            {
                // the variable in question may have been previously unused, and is now used, so copy the previous value over
                copyPreviousIfFound(overriddenValue, source.m_overridesUnused);
            }
        }

        //////////////////////////////////////////////////////////////////////////
        // #functions2 provide an identifier for the node/variable in the source that caused the dependency. the root will not have one.
        // the above will provide the data to handle the cases where only certain dependency nodes were removed
        // until then we do a sanity check, if any part of the dependencies were altered, assume no overrides are valid.
        if (m_dependencies.size() != source.m_dependencies.size())
        {
            return;
        }
        else
        {
            for (size_t index = 0; index != m_dependencies.size(); ++index)
            {
                if (m_dependencies[index].m_source != source.m_dependencies[index].m_source)
                {
                    return;
                }
            }
        }
        //////////////////////////////////////////////////////////////////////////

        for (size_t index = 0; index != m_dependencies.size(); ++index)
        {
            m_dependencies[index].CopyPreviousOverriddenValues(source.m_dependencies[index]);
        }
    }

    bool BuildVariableOverrides::IsEmpty() const
    {
        return m_variables.empty() && m_entityIds.empty() && m_dependencies.empty();
    }

    void BuildVariableOverrides::Reflect(AZ::ReflectContext* reflectContext)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(reflectContext))
        {
            serializeContext->Class<BuildVariableOverrides>()
                ->Version(1)
                ->Field("source", &BuildVariableOverrides::m_source)
                ->Field("variables", &BuildVariableOverrides::m_variables)
                ->Field("entityId", &BuildVariableOverrides::m_entityIds)
                ->Field("overrides", &BuildVariableOverrides::m_overrides)
                ->Field("overridesUnused", &BuildVariableOverrides::m_overridesUnused)
                ->Field("dependencies", &BuildVariableOverrides::m_dependencies)
                ;

            if (auto editContext = serializeContext->GetEditContext())
            {
                editContext->Class<BuildVariableOverrides>("Variables", "Variables exposed by the attached Script Canvas Graph")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &BuildVariableOverrides::m_overrides, "Variables", "Array of Variables within Script Canvas Graph")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->Attribute(AZ::Edit::Attributes::ContainerCanBeModified, false)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &BuildVariableOverrides::m_overridesUnused, "Unused Variables", "Unused variables within Script Canvas Graph, when used they keep the values set here")
                        ->Attribute(AZ::Edit::Attributes::ContainerCanBeModified, false)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &BuildVariableOverrides::m_dependencies, "Dependencies", "Variables in Dependencies of the Script Canvas Graph")
                        ->Attribute(AZ::Edit::Attributes::ContainerCanBeModified, false)
                    ;
            }
        }
    }

    // use this to initialize the new data, and make sure they have a editor graph variable for proper editor display
    void BuildVariableOverrides::PopulateFromParsedResults(ScriptCanvas::Grammar::AbstractCodeModelConstPtr abstractCodeModel, const ScriptCanvas::VariableData& variables)
    {
        if (!abstractCodeModel)
        {
            AZ_Error("ScriptCanvasBuider", false, "null abstract code model");
            return;
        }

        const ScriptCanvas::Grammar::ParsedRuntimeInputs& inputs = abstractCodeModel->GetRuntimeInputs();

        for (auto& variable : inputs.m_variables)
        {
            auto graphVariable = variables.FindVariable(variable.first);
            if (!graphVariable)
            {
                AZ_Error("ScriptCanvasBuilder", false, "Missing Variable from graph data that was just parsed");
                continue;
            }

            m_variables.push_back(*graphVariable);
            auto& buildVariable = m_variables.back();
            buildVariable.DeepCopy(*graphVariable); // in case of BCO, a new one needs to be created

            // copy to override list for editor display
            m_overrides.push_back(*graphVariable);
            auto& overrideValue = m_overrides.back();
            overrideValue.DeepCopy(*graphVariable);
            overrideValue.SetScriptInputControlVisibility(AZ::Edit::PropertyVisibility::Hide);
            overrideValue.SetAllowSignalOnChange(false);
        }

        for (auto& entityId : inputs.m_entityIds)
        {
            m_entityIds.push_back(entityId);

            if (!ScriptCanvas::Grammar::IsParserGeneratedId(entityId.first))
            {
                if (auto graphEntityId = variables.FindVariable(entityId.first); graphEntityId && graphEntityId->IsComponentProperty())
                {
                    // copy to override list for editor display
                    m_overrides.push_back(*graphEntityId);
                    auto& overrideValue = m_overrides.back();
                    overrideValue.SetScriptInputControlVisibility(AZ::Edit::PropertyVisibility::Hide);
                    overrideValue.SetAllowSignalOnChange(false);
                }
            }
        }

        for (auto& variable : abstractCodeModel->GetVariablesUnused())
        {
            auto graphVariable = variables.FindVariable(variable->m_sourceVariableId);
            if (!graphVariable)
            {
                AZ_Error("ScriptCanvasBuilder", false, "Missing Variable from graph data that was just parsed");
                continue;
            }

            if (graphVariable->IsComponentProperty())
            {
                // copy to override unused list for editor display
                m_overridesUnused.push_back(*graphVariable);
                auto& overrideValue = m_overridesUnused.back();
                overrideValue.DeepCopy(*graphVariable);
                overrideValue.SetScriptInputControlVisibility(AZ::Edit::PropertyVisibility::Hide);
                overrideValue.SetAllowSignalOnChange(false);
            }
        }
    }

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

    ScriptCanvas::RuntimeDataOverrides ConvertToRuntime(const BuildVariableOverrides& buildOverrides)
    {
        ScriptCanvas::RuntimeDataOverrides runtimeOverrides;

        runtimeOverrides.m_runtimeAsset = AZ::Data::Asset<ScriptCanvas::RuntimeAsset>
            (AZ::Data::AssetId(buildOverrides.m_source.GetId().m_guid, AZ_CRC("RuntimeData", 0x163310ae)), azrtti_typeid<ScriptCanvas::RuntimeAsset>(), {});
        runtimeOverrides.m_runtimeAsset.SetAutoLoadBehavior(AZ::Data::AssetLoadBehavior::PreLoad);
        runtimeOverrides.m_variableIndices.resize(buildOverrides.m_variables.size());

        for (size_t index = 0; index != buildOverrides.m_variables.size(); ++index)
        {
            auto& variable = buildOverrides.m_variables[index];
            auto iter = AZStd::find_if
                ( buildOverrides.m_overrides.begin()
                , buildOverrides.m_overrides.end()
                , [&variable](auto& candidate) { return candidate.GetVariableId() == variable.GetVariableId(); });

            if (iter != buildOverrides.m_overrides.end())
            {
                if (iter->GetDatum())
                {
                    runtimeOverrides.m_variables.push_back(ScriptCanvas::RuntimeVariable(iter->GetDatum()->ToAny()));
                    runtimeOverrides.m_variableIndices[index] = true;
                }
                else
                {
                    AZ_Warning("ScriptCanvasBuilder", false, "build overrides missing variable override, Script may not function properly");
                    runtimeOverrides.m_variableIndices[index] = false;
                }
            }
            else
            {
                runtimeOverrides.m_variableIndices[index] = false;
            }
        }

        for (auto& entity : buildOverrides.m_entityIds)
        {
            auto& variableId = entity.first;
            auto iter = AZStd::find_if(buildOverrides.m_overrides.begin(), buildOverrides.m_overrides.end(), [&variableId](auto& candidate) { return candidate.GetVariableId() == variableId; });
            if (iter != buildOverrides.m_overrides.end())
            {
                // the entity was overridden on the instance
                if (iter->GetDatum() && iter->GetDatum()->GetAs<AZ::EntityId>())
                {
                    runtimeOverrides.m_entityIds.push_back(*iter->GetDatum()->GetAs<AZ::EntityId>());
                }
                else
                {
                    AZ_Warning("ScriptCanvasBuilder", false, "build overrides missing EntityId, Script may not function properly");
                    runtimeOverrides.m_entityIds.push_back(AZ::EntityId{});
                }
            }
            else
            {
                // the entity is overridden, as part of the required process of to instantiation
                runtimeOverrides.m_entityIds.push_back(entity.second);
            }
        }

        for (auto& buildDependency : buildOverrides.m_dependencies)
        {
            runtimeOverrides.m_dependencies.push_back(ConvertToRuntime(buildDependency));
        }

        return runtimeOverrides;
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
            return AZ::Failure(AZStd::string::format("LoadEditorAssetTree failed to get engine relative path from %s-%.*s.", editorAssetId.ToString<AZStd::string>().c_str(), aznumeric_cast<int>(assetHint.size()), assetHint.data()));
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

        auto loadAssetOutcome = ScriptCanvasBuilder::LoadEditorAsset(assetInfo.m_relativePath, editorAssetId, filterCB);
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

        auto variableComponent = AZ::EntityUtils::FindFirstDerivedComponent<ScriptCanvas::GraphVariableManagerComponent>(buildEntity);
        if (!variableComponent)
        {
            return AZ::Failure(AZStd::string("No GraphVariableManagerComponent in source Entity"));
        }

        const ScriptCanvas::VariableData* variableData = variableComponent->GetVariableDataConst(); // get this from the entity
        if (!variableData)
        {
            return AZ::Failure(AZStd::string("No variableData in source GraphVariableManagerComponent"));
        }

        auto parseOutcome = ScriptCanvasBuilder::ParseGraph(*buildEntity, "");
        if (!parseOutcome.IsSuccess() || !parseOutcome.GetValue())
        {
            return AZ::Failure(AZStd::string("graph failed to parse"));
        }

        BuildVariableOverrides result;
        result.m_source = editorAssetTree.m_asset;
        result.PopulateFromParsedResults(parseOutcome.GetValue(), *variableData);

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
