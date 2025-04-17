/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/IO/GenericStreams.h>
#include <AzCore/Serialization/Json/JsonSerialization.h>
#include <AzCore/Serialization/Json/JsonSerializationResult.h>
#include <AzCore/Serialization/Json/JsonUtils.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/Utils.h>
#include <AzCore/Utils/Utils.h>
#include <AzCore/std/string/string_view.h>
#include <ScriptCanvas/Core/Graph.h>

#include <ScriptCanvas/Core/GraphSerialization.h>

namespace GraphSerializationCpp
{
    void AppendTabs(AZStd::string& result, size_t depth)
    {
        AZStd::fill_n(AZStd::back_inserter(result), depth, '\t');
    }

    void CollectNodes(const ScriptCanvas::GraphData::NodeContainer& container, ScriptCanvas::SerializationListeners& listeners)
    {
        for (auto& nodeEntity : container)
        {
            if (nodeEntity)
            {
                if (auto listener = azrtti_cast<ScriptCanvas::SerializationListener*>
                    (AZ::EntityUtils::FindFirstDerivedComponent<ScriptCanvas::Node>(nodeEntity)))
                {
                    listeners.push_back(listener);
                }
            }
        }
    }

    // Create new EntityIds for all EntityIds found in the SC Entity/Component objects
    // and map all old Ids to the new ones. This way, no Entity activation/deactivation, or
    // bus communication via EntityId will be handled by multiple or incorrect objects on
    // possible multiple instantiations of graphs.
    //
    // EntityIds contained in variable (those set to self or the graph unique id, will be ignored)
    void MakeGraphComponentEntityIdsUnique(
        AZ::Entity& entity, AZ::SerializeContext& serializeContext, AZStd::unordered_map<AZ::EntityId, AZ::EntityId>& oldIdToNewIdOut)
    {
        oldIdToNewIdOut.clear();
        oldIdToNewIdOut[AZ::EntityId()] = AZ::EntityId();
        oldIdToNewIdOut[ScriptCanvas::GraphOwnerId] = ScriptCanvas::GraphOwnerId;
        oldIdToNewIdOut[ScriptCanvas::UniqueId] = ScriptCanvas::UniqueId;

        AZ::IdUtils::Remapper<AZ::EntityId>::GenerateNewIdsAndFixRefs(&entity, oldIdToNewIdOut, &serializeContext);
    }
}

namespace ScriptCanvas
{
    AZStd::string SourceTree::ToString(size_t depth) const
    {
        AZStd::string result;
        GraphSerializationCpp::AppendTabs(result, depth);
        result += m_source.ToString();
        depth += m_dependencies.empty() ? 0 : 1;

        for (const auto& dependency : m_dependencies)
        {
            result += "\n";
            GraphSerializationCpp::AppendTabs(result, depth);
            result += dependency.ToString(depth);
        }

        return result;
    }

    DeserializeResult Deserialize
        ( AZStd::string_view source
        , MakeInternalGraphEntitiesUnique makeUniqueEntities
        , LoadReferencedAssets loadReferencedAssets)
    {
        namespace JSRU = AZ::JsonSerializationUtils;
        using namespace GraphSerializationCpp;

        DeserializeResult result;
        result.m_graphDataPtr = aznew ScriptCanvasData();

        AZ::SerializeContext* serializeContext = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(serializeContext, &AZ::ComponentApplicationRequests::GetSerializeContext);
        if (!serializeContext)
        {
            result.m_isSuccessful = false;
            result.m_errors = "No serialize was context available to properly save source file.";
            return result;
        }

        AZ::JsonDeserializerSettings settings;
        settings.m_serializeContext = serializeContext;
        settings.m_metadata.Create<SerializationListeners>();
        settings.m_clearContainers = true;

        auto loadResult = JSRU::LoadObjectFromStringByType
            ( &(*result.m_graphDataPtr)
            , azrtti_typeid<ScriptCanvasData>()
            , source
            , result.m_jsonResults
            , &settings);

        if (!loadResult.IsSuccess())
        {
            // ...try legacy xml as a failsafe
            result.m_fromObjectStreamXML = true;

            AZ::IO::MemoryStream stream(source.data(), source.length());
            if (!AZ::Utils::LoadObjectFromStreamInPlace
                ( stream
                , *result.m_graphDataPtr
                , serializeContext
                , AZ::ObjectStream::FilterDescriptor(nullptr, AZ::ObjectStream::FILTERFLAG_IGNORE_UNKNOWN_CLASSES)))
            {
                result.m_isSuccessful = false;
                result.m_errors = "JSON (and failsafe XML) deserialize attempt failed.\n";
                result.m_errors += "The error might be caused by deprecated Spawnable Nodes in your target scriptcanvas file.\n";
                result.m_errors += "If so, please run UpdateSpawnableNodes python script for the scriptcanvas file to see if the error is resolved.\n";
                result.m_errors += "(Run 'python {Your o3de engine folder}\\Gems\\ScriptCanvas\\SourceModificationScripts\\UpdateSpawnableNodes.py {Your target scriptcanvas file}')";

                return result;
            }
        }

        auto graph = result.m_graphDataPtr->ModGraph();
        if (!graph)
        {
            result.m_isSuccessful = false;
            result.m_errors = "Failed to find graph data after loading source";
            return result;
        }

        auto listeners = settings.m_metadata.Find<SerializationListeners>();
        AZ_Assert(listeners, "Failed to find SerializationListeners");
        CollectNodes(graph->GetGraphData()->m_nodes, *listeners);
        for (auto listener : *listeners)
        {
            listener->OnDeserialize();
        }

        // can be deprecated ECS management...
        auto entity = result.m_graphDataPtr->GetScriptCanvasEntity();
        if (!entity)
        {
            result.m_isSuccessful = false;
            result.m_errors = "Loaded script canvas file was missing a necessary Entity.";
            return result;
        }

        if (entity->GetState() != AZ::Entity::State::Constructed)
        {
            result.m_isSuccessful = false;
            result.m_errors = "Entity loaded in bad state";
            return result;
        }

        if (makeUniqueEntities == MakeInternalGraphEntitiesUnique::Yes)
        {
            MakeGraphComponentEntityIdsUnique(*entity, *serializeContext, result.m_originalIdsToNewIds);
        }

        graph->MarkOwnership(*result.m_graphDataPtr);

        if (loadReferencedAssets == LoadReferencedAssets::Yes)
        {
            entity->Init();
            entity->Activate();
        }
        // ...can be deprecated ECS management

        result.m_isSuccessful = true;
        return result;
    }

    SerializationResult Serialize(const ScriptCanvasData& source, AZ::IO::GenericStream& stream)
    {
        namespace JSRU = AZ::JsonSerializationUtils;
        using namespace GraphSerializationCpp;

        SerializationResult result;

        AZ::SerializeContext* serializeContext = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(serializeContext, &AZ::ComponentApplicationRequests::GetSerializeContext);
        if (!serializeContext)
        {
            result.m_isSuccessful = false;
            result.m_errors = "no serialize context available to properly save source file";
            return result;
        }

        auto saveTarget = source.GetGraph();
        if (!saveTarget || !saveTarget->GetGraphDataConst())
        {
            result.m_isSuccessful = false;
            result.m_errors = "source save container failed to return serializable graph data";
            return result;
        }

        AZ::JsonSerializerSettings settings;
        settings.m_metadata.Create<ScriptCanvas::SerializationListeners>();
        auto listeners = settings.m_metadata.Find<ScriptCanvas::SerializationListeners>();
        AZ_Assert(listeners, "Failed to create SerializationListeners");
        CollectNodes(saveTarget->GetGraphDataConst()->m_nodes, *listeners);
        settings.m_keepDefaults = false;
        settings.m_serializeContext = serializeContext;

        for (auto listener : *listeners)
        {
            listener->OnSerialize();
        }

        auto saveOutcome = JSRU::SaveObjectToStream<ScriptCanvas::ScriptCanvasData>(&source, stream, nullptr, &settings);
        if (!saveOutcome.IsSuccess())
        {
            result.m_isSuccessful = false;
            result.m_errors = AZStd::string::format("JSON serialization failed to save source: %s", saveOutcome.GetError().c_str());
            return result;
        }

        result.m_isSuccessful = true;
        return result;
    }
}
