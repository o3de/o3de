/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Asset/AssetSerializer.h>
#include <AzToolsFramework/API/EditorAssetSystemAPI.h>
#include <Builder/ScriptCanvasBuilder.h>
#include <Builder/ScriptCanvasBuilderWorker.h>
#include <Builder/ScriptCanvasBuilderWorkerUtility.h>
#include <ScriptCanvas/Assets/ScriptCanvasFileHandling.h>
#include <ScriptCanvas/Components/EditorDeprecationData.h>
#include <ScriptCanvas/Components/EditorGraph.h>
#include <ScriptCanvas/Components/EditorGraphVariableManagerComponent.h>
#include <ScriptCanvas/Grammar/AbstractCodeModel.h>

namespace BuildVariableOverridesCpp
{
    enum Version
    {
        Original = 1,
        EditorAssetRedux,

        // add description above
        Current
    };

    bool VersionConverter
        ( AZ::SerializeContext& serializeContext
        , AZ::SerializeContext::DataElementNode& rootElement)
    {
        if (rootElement.GetVersion() < BuildVariableOverridesCpp::Version::EditorAssetRedux)
        {
            auto sourceIndex = rootElement.FindElement(AZ_CRC_CE("source"));
            if (sourceIndex == -1)
            {
                AZ_Error("ScriptCanvas", false, "BuildVariableOverrides coversion failed: 'source' was missing");
                return false;
            }

            auto& sourceElement = rootElement.GetSubElement(sourceIndex);
            AZ::Data::Asset<ScriptCanvasEditor::Deprecated::ScriptCanvasAsset> asset;
            if (!sourceElement.GetData(asset))
            {
                AZ_Error("ScriptCanvas", false, "BuildVariableOverrides coversion failed: could not retrieve 'source' data");
                return false;
            }

            ScriptCanvasEditor::SourceHandle sourceHandle(nullptr, asset.GetId().m_guid, {});
            if (!rootElement.AddElementWithData(serializeContext, "source", sourceHandle))
            {
                AZ_Error("ScriptCanvas", false, "BuildVariableOverrides coversion failed: could not add updated 'source' data");
                return false;
            }
        }

        return true;
    }
}

namespace ScriptCanvasBuilder
{
    void BuildVariableOverrides::Clear()
    {
        m_source = {};
        m_variables.clear();
        m_overrides.clear();
        m_overridesUnused.clear();
        m_entityIds.clear();
        m_dependencies.clear();
    }

    void BuildVariableOverrides::CopyPreviousOverriddenValues(const BuildVariableOverrides& source)
    {
        auto isEqual = [](const ScriptCanvas::GraphVariable& lhs, const ScriptCanvas::GraphVariable& rhs)
        {
            return (lhs.GetVariableId() == rhs.GetVariableId() && lhs.GetDataType() == rhs.GetDataType())
                || (lhs.GetVariableName() == rhs.GetVariableName() && lhs.GetDataType() == rhs.GetDataType());
        };

        auto copyPreviousIfFound = [isEqual](ScriptCanvas::GraphVariable& overriddenValue, const AZStd::vector<ScriptCanvas::GraphVariable>& source)
        {
            auto iter = AZStd::find_if(source.begin(), source.end()
                , [&overriddenValue, isEqual](const auto& candidate) { return isEqual(candidate, overriddenValue); });

            if (iter != source.end())
            {
                overriddenValue.ModDatum().DeepCopyDatum(*iter->GetDatum());
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

        for (auto& overriddenValue : m_overridesUnused)
        {
            if (!copyPreviousIfFound(overriddenValue, source.m_overridesUnused))
            {
                // the variable in question may have been previously used, and is now unused, so copy the previous value over
                copyPreviousIfFound(overriddenValue, source.m_overrides);
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
                ->Version(BuildVariableOverridesCpp::Version::Current, &BuildVariableOverridesCpp::VersionConverter)
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
    AZ::Outcome<void, AZStd::string> BuildVariableOverrides::PopulateFromParsedResults(ScriptCanvas::Grammar::AbstractCodeModelConstPtr abstractCodeModel, const ScriptCanvas::VariableData& variables)
    {
        if (!abstractCodeModel)
        {
            return AZ::Failure(AZStd::string("null abstract code model input"));
        }

        const ScriptCanvas::Grammar::ParsedRuntimeInputs& inputs = abstractCodeModel->GetRuntimeInputs();

        for (auto& variable : inputs.m_variables)
        {
            auto graphVariable = variables.FindVariable(variable.first);
            if (!graphVariable)
            {
                return AZ::Failure(AZStd::string("Missing Variable from graph data that was just parsed"));
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
                return AZ::Failure(AZStd::string("Missing unused Variable from graph data that was just parsed"));
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

        const ScriptCanvas::OrderedDependencies& orderedDependencies = abstractCodeModel->GetOrderedDependencies();
        for (auto& dependency : orderedDependencies.orderedUserGraphAssetIds)
        {
            // Populating the immediate dependencies is enough, recursion is not necessary. Each
            // previous dependency has populated its immediate dependencies and stored them directly,
            // rather than storing their dependencies as asset references.
            auto result = LoadBuilderDataAsset(AZ::Data::AssetId(dependency.m_guid, AZ_CRC_CE("BuilderData")));
            if (!result.IsSuccess())
            {
                return AZ::Failure(AZStd::string::format("Failed to load dependency for builder: %s",
                    result.GetError().c_str()));
            }

            m_dependencies.push_back(result.TakeValue());
        }

        return AZ::Success();
    }

    void BuildVariableOverrides::SetHandlesToDescription()
    {
        m_source = m_source.Describe();

        for (auto& dependency : m_dependencies)
        {
            dependency.SetHandlesToDescription();
        }
    }

    ScriptCanvas::RuntimeDataOverrides ConvertToRuntime(const BuildVariableOverrides& buildOverrides)
    {
        ScriptCanvas::RuntimeDataOverrides runtimeOverrides;
        runtimeOverrides.m_runtimeAsset = AZ::Data::Asset<ScriptCanvas::RuntimeAsset>
            ( AZ::Data::AssetId(buildOverrides.m_source.Id(), AZ_CRC_CE("RuntimeData"))
            , azrtti_typeid<ScriptCanvas::RuntimeAsset>()
            , {});
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

    AZ::Outcome<BuildVariableOverrides, AZStd::string> LoadBuilderData(const ScriptCanvasEditor::SourceHandle& sourceHandle)
    {
        const AZ::Data::AssetId builderAssetId(sourceHandle.Id(), AZ_CRC_CE("BuilderData"));
        return LoadBuilderDataAsset(builderAssetId);
    }

    AZ::Outcome<BuildVariableOverrides, AZStd::string> LoadBuilderDataAsset(const AZ::Data::AssetId& assetId)
    {
        AZ::Data::Asset<BuildVariableOverridesData> sourceAsset
            = AZ::Data::AssetManager::Instance().GetAsset<BuildVariableOverridesData>
                ( assetId
                , AZ::Data::AssetLoadBehavior::PreLoad);

        // use the asset handler directly? Asset manager may not be running, see RunUnitTestGraph loading mechanism
        auto assetStatus = sourceAsset.BlockUntilLoadComplete();
        if (assetStatus != AZ::Data::AssetData::AssetStatus::Ready || !sourceAsset.GetAs<BuildVariableOverrides>())
        {
            return AZ::Failure(AZStd::string::format("Failed to load builder asset: %s", assetId.ToString<AZStd::string>().c_str()));
        }

        // No recursion is necessary, as the AP has populated the assets through all dependencies during processing.
        BuildVariableOverrides& result = *sourceAsset.GetAs<BuildVariableOverrides>();
        return AZ::Success(result);
    }

    /*
    AZ::Outcome<BuildVariableOverrides, AZStd::string> ParseEditorAssetTree(const ScriptCanvasEditor::EditorAssetTree& editorAssetTree)
    {
        auto buildEntity = editorAssetTree.m_asset.Get()->GetEntity();
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
                    ( "ParseEditorAssetTree failed to parse dependent graph from %s: %s"
                    , dependentAsset.m_asset.ToString().c_str()
                    , parseDependentOutcome.GetError().c_str()));
            }

            result.m_dependencies.push_back(parseDependentOutcome.TakeValue());
        }

        return AZ::Success(result);
    }*/


    AZ::Data::AssetHandler::LoadResult BuildVariableOverridesAssetHandler::LoadAssetData
        ( const AZ::Data::Asset<AZ::Data::AssetData>& asset
        , AZStd::shared_ptr<AZ::Data::AssetDataStream> stream
        , const AZ::Data::AssetFilterCB& assetLoadFilterCB)
    {
        auto* builderData = asset.GetAs<BuildVariableOverridesData>();
        AZ_Assert(builderData, "This should be a Script Canvas builder data asset, as this is the only type we process!");

        if (builderData && m_serializeContext)
        {
            stream->Seek(0U, AZ::IO::GenericStream::ST_SEEK_BEGIN);
            bool loadSuccess = AZ::Utils::LoadObjectFromStreamInPlace(*stream, builderData->m_overrides, m_serializeContext
                , AZ::ObjectStream::FilterDescriptor(assetLoadFilterCB));
            return loadSuccess ? AZ::Data::AssetHandler::LoadResult::LoadComplete : AZ::Data::AssetHandler::LoadResult::Error;
        }

        return AZ::Data::AssetHandler::LoadResult::Error;
    }

    bool BuildVariableOverridesAssetHandler::SaveAssetData(const AZ::Data::Asset<AZ::Data::AssetData>& asset, AZ::IO::GenericStream* stream)
    {
        auto builderData = asset.GetAs<BuildVariableOverridesData>();
        AZ_Assert(builderData, "This should be Script Canvas builder data, as this is the only type we process!");

        if (builderData && m_serializeContext)
        {
            AZ::ObjectStream* binaryObjStream = AZ::ObjectStream::Create
                ( stream
                , *m_serializeContext
                , ScriptCanvas::g_saveEditorAssetsAsPlainTextForDebug
                    ? AZ::ObjectStream::ST_XML
                    : AZ::ObjectStream::ST_BINARY);
            bool graphSaved = binaryObjStream->WriteClass(&builderData->m_overrides);
            binaryObjStream->Finalize();
            return graphSaved;
        }

        return false;
    }
}
