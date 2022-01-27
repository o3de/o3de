/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/IO/FileIO.h>
#include <AzCore/IO/GenericStreams.h>
#include <AzCore/Serialization/Json/JsonSerialization.h>
#include <AzCore/Serialization/Json/JsonSerializationResult.h>
#include <AzCore/Serialization/Json/JsonUtils.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/Utils.h>
#include <AzCore/Utils/Utils.h>
#include <AzCore/std/string/string_view.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include <AzToolsFramework/API/EditorAssetSystemAPI.h>
#include <ScriptCanvas/Assets/ScriptCanvasFileHandling.h>
#include <ScriptCanvas/Bus/ScriptCanvasBus.h>
#include <ScriptCanvas/Components/EditorGraph.h>
#include <ScriptCanvas/Core/SerializationListener.h>
#include <ScriptCanvas/Asset/RuntimeAsset.h>
#include <ScriptCanvas/Libraries/Math/MathNodeUtilities.h>

namespace ScriptCanvasFileHandlingCpp
{
    void AppendTabs(AZStd::string& result, size_t depth)
    {
        for (size_t i = 0; i < depth; ++i)
        {
            result += "\t";
        }
    }

    void CollectNodes(const ScriptCanvas::GraphData::NodeContainer& container, ScriptCanvas::SerializationListeners& listeners)
    {
        for (auto& nodeEntity : container)
        {
            if (nodeEntity)
            {
                if (auto listener = azrtti_cast<ScriptCanvas::SerializationListener*>
                    ( AZ::EntityUtils::FindFirstDerivedComponent<ScriptCanvas::Node>(nodeEntity)))
                {
                    listeners.push_back(listener);
                }
            }
        }
    }
}

namespace ScriptCanvasEditor
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
        ScriptCanvasFileHandlingCpp::AppendTabs(result, depth);
        result += m_asset.ToString();
        depth += m_dependencies.empty() ? 0 : 1;

        for (const auto& dependency : m_dependencies)
        {
            result += "\n";
            ScriptCanvasFileHandlingCpp::AppendTabs(result, depth);
            result += dependency.ToString(depth);
        }

        return result;
    }

    AZ::Outcome<void, AZStd::string> LoadDataFromJson
        ( ScriptCanvas::ScriptCanvasData& dataTarget
        , AZStd::string_view source
        , AZ::SerializeContext& serializeContext)
    {
        namespace JSRU = AZ::JsonSerializationUtils;
        using namespace ScriptCanvas;

        AZ::JsonDeserializerSettings settings;
        settings.m_serializeContext = &serializeContext;
        settings.m_metadata.Create<SerializationListeners>();

        auto loadResult = JSRU::LoadObjectFromStringByType
            ( &dataTarget
            , azrtti_typeid<ScriptCanvasData>()
            , source
            , &settings);

        if (!loadResult.IsSuccess())
        {
            return loadResult;
        }
                    
        if (auto graphData = dataTarget.ModGraph())
        {
            auto listeners = settings.m_metadata.Find<SerializationListeners>();
            AZ_Assert(listeners, "Failed to find SerializationListeners");

            ScriptCanvasFileHandlingCpp::CollectNodes(graphData->GetGraphData()->m_nodes, *listeners);

            for (auto listener : *listeners)
            {
                listener->OnDeserialize();
            }
        }
        else
        {
            return AZ::Failure(AZStd::string("Failed to find graph data after loading source"));
        }

        return AZ::Success();
    }


    AZ::Outcome<EditorAssetTree, AZStd::string> LoadEditorAssetTree(SourceHandle handle, EditorAssetTree* parent)
    {
        if (!CompleteDescriptionInPlace(handle))
        {
            return AZ::Failure(AZStd::string::format("LoadEditorAssetTree failed to describe graph from %s", handle.ToString().c_str()));
        }

        if (!handle.Get())
        {
            auto loadAssetOutcome = LoadFromFile(handle.Path().c_str());
            if (!loadAssetOutcome.IsSuccess())
            {
                return AZ::Failure(AZStd::string::format("LoadEditorAssetTree failed to load graph from %s: %s"
                    , handle.ToString().c_str(), loadAssetOutcome.GetError().c_str()));
            }

            handle = SourceHandle(loadAssetOutcome.GetValue(), handle.Id(), handle.Path().c_str());
        }

        AZStd::vector<SourceHandle> dependentAssets;
        const auto subgraphInterfaceAssetTypeID = azrtti_typeid<AZ::Data::Asset<ScriptCanvas::SubgraphInterfaceAsset>>();
        
        auto beginElementCB = [&subgraphInterfaceAssetTypeID, &dependentAssets]
            ( void* instance
            , const AZ::SerializeContext::ClassData* classData
            , const AZ::SerializeContext::ClassElement* classElement) -> bool
        {
            if (classElement)
            {
                // if we are a pointer, then we may be pointing to a derived type.
                if (classElement->m_flags & AZ::SerializeContext::ClassElement::FLG_POINTER)
                {
                    // if ptr is a pointer-to-pointer, cast its value to a void* (or const void*) and dereference to get to the actual object pointer.
                    instance = *(void**)(instance);
                }
            }

            if (classData->m_typeId == subgraphInterfaceAssetTypeID)
            {
                auto asset = reinterpret_cast<AZ::Data::Asset<ScriptCanvas::SubgraphInterfaceAsset>*>(instance);
                auto id = asset->GetId();
                dependentAssets.push_back(SourceHandle(nullptr, id.m_guid, {}));
            }

            return true;
        };

        AZ::SerializeContext* serializeContext = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(serializeContext, &AZ::ComponentApplicationRequests::GetSerializeContext);
        AZ_Assert(serializeContext, "LoadEditorAssetTree() ailed to retrieve serialize context!");

        const ScriptCanvasEditor::Graph* graph = handle.Get();
        serializeContext->EnumerateObject(graph, beginElementCB, nullptr, AZ::SerializeContext::ENUM_ACCESS_FOR_READ);

        EditorAssetTree result;

        for (auto& dependentAsset : dependentAssets)
        {
            auto loadDependentOutcome = LoadEditorAssetTree(dependentAsset, &result);
            if (!loadDependentOutcome.IsSuccess())
            {
                return AZ::Failure(AZStd::string::format("LoadEditorAssetTree failed to load graph from %s: %s"
                    , dependentAsset.ToString().c_str(), loadDependentOutcome.GetError().c_str()));
            }

            result.m_dependencies.push_back(loadDependentOutcome.TakeValue());
        }

        if (parent)
        {
            result.SetParent(*parent);
        }

        result.m_asset = AZStd::move(handle);
        return AZ::Success(result);
    }

    AZ::Outcome<ScriptCanvasEditor::SourceHandle, AZStd::string> LoadFromFile(AZStd::string_view path)
    {
        namespace JSRU = AZ::JsonSerializationUtils;

        auto fileStringOutcome = AZ::Utils::ReadFile<AZStd::string>(path);
        if (!fileStringOutcome)
        {
            return AZ::Failure(fileStringOutcome.TakeError());
        }

        const auto& asString = fileStringOutcome.GetValue();
        ScriptCanvas::DataPtr scriptCanvasData = aznew ScriptCanvas::ScriptCanvasData();
        if (!scriptCanvasData)
        {
            return AZ::Failure(AZStd::string("failed to allocate ScriptCanvas::ScriptCanvasData after loading source file"));
        }
        
        AZ::SerializeContext* serializeContext = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(serializeContext, &AZ::ComponentApplicationRequests::GetSerializeContext);
        if (!serializeContext)
        {
            return AZ::Failure(AZStd::string("no serialize context available to properly parse source file"));
        }

        // attempt JSON deserialization...
        auto jsonResult = LoadDataFromJson(*scriptCanvasData, AZStd::string_view{ asString.begin(), asString.size() }, *serializeContext);
        if (!jsonResult.IsSuccess())
        {
            // ...try legacy xml as a failsafe
            AZ::IO::ByteContainerStream byteStream(&asString);
            if (!AZ::Utils::LoadObjectFromStreamInPlace
                ( byteStream
                , *scriptCanvasData
                , serializeContext
                , AZ::ObjectStream::FilterDescriptor(nullptr, AZ::ObjectStream::FILTERFLAG_IGNORE_UNKNOWN_CLASSES)))
            {
                return AZ::Failure(AZStd::string::format("XML and JSON load attempts failed: %s", jsonResult.GetError().c_str()));
            }
        }

        if (auto entity = scriptCanvasData->GetScriptCanvasEntity())
        {
            AZ_Assert(entity->GetState() == AZ::Entity::State::Constructed, "Entity loaded in bad state");
            AZ::u64 entityId =
                aznumeric_caster(ScriptCanvas::MathNodeUtilities::GetRandomIntegral<AZ::s64>(1, std::numeric_limits<AZ::s64>::max()));
            entity->SetId(AZ::EntityId(entityId));

            auto graph = entity->FindComponent<ScriptCanvasEditor::Graph>();
            graph->MarkOwnership(*scriptCanvasData);

            entity->Init();
            entity->Activate();
        }
        else
        {
            return AZ::Failure(AZStd::string("Loaded script canvas file was missing a necessary Entity."));
        }

        return AZ::Success(ScriptCanvasEditor::SourceHandle(scriptCanvasData, path));
    }

    AZ::Outcome<void, AZStd::string> SaveToStream(const SourceHandle& source, AZ::IO::GenericStream& stream)
    {
        namespace JSRU = AZ::JsonSerializationUtils;
        
        if (!source.IsGraphValid())
        {
            return AZ::Failure(AZStd::string("no source graph to save"));
        }

        if (source.Path().empty())
        {
            return AZ::Failure(AZStd::string("no destination path specified"));
        }

        AZ::SerializeContext* serializeContext = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(serializeContext, &AZ::ComponentApplicationRequests::GetSerializeContext);
        if (!serializeContext)
        {
            return AZ::Failure(AZStd::string("no serialize context available to properly save source file"));
        }

        auto graphData = source.Get()->GetOwnership();
        if (!graphData)
        {
            return AZ::Failure(AZStd::string("source is missing save container"));
        }
        
        if (graphData->GetEditorGraph() != source.Get())
        {
            return AZ::Failure(AZStd::string("source save container refers to incorrect graph"));
        }

        auto saveTarget = graphData->ModGraph();
        if (!saveTarget || !saveTarget->GetGraphData())
        {
            return AZ::Failure(AZStd::string("source save container failed to return serializable graph data"));
        }

        AZ::JsonSerializerSettings settings;
        settings.m_metadata.Create<ScriptCanvas::SerializationListeners>();
        auto listeners = settings.m_metadata.Find<ScriptCanvas::SerializationListeners>();
        AZ_Assert(listeners, "Failed to create SerializationListeners");
        ScriptCanvasFileHandlingCpp::CollectNodes(saveTarget->GetGraphData()->m_nodes, *listeners);
        settings.m_keepDefaults = false;
        settings.m_serializeContext = serializeContext;

        for (auto listener : *listeners)
        {   
            listener->OnSerialize();
        }

        auto saveOutcome = JSRU::SaveObjectToStream<ScriptCanvas::ScriptCanvasData>(graphData.get(), stream, nullptr, &settings);
        if (!saveOutcome.IsSuccess())
        {
            return AZ::Failure(AZStd::string::format("JSON serialization failed to save source: %s", saveOutcome.GetError().c_str()));
        }

        return AZ::Success();
    }
}
