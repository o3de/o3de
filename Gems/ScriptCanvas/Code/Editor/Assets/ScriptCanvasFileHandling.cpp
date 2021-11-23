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

namespace ScriptCanvasFileHandlingCpp
{
    using namespace ScriptCanvas;

    void CollectNodes(const GraphData::NodeContainer& container, SerializationListeners& listeners)
    {
        for (auto& nodeEntity : container)
        {
            if (nodeEntity)
            {
                if (auto listener = azrtti_cast<SerializationListener*>(AZ::EntityUtils::FindFirstDerivedComponent<Node>(nodeEntity)))
                {
                    listeners.push_back(listener);
                }
            }
        }
    }
}

namespace ScriptCanvasEditor
{
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

    AZ::Outcome<ScriptCanvasEditor::SourceHandle, AZStd::string> LoadFromFile(AZStd::string_view path)
    {
        namespace JSRU = AZ::JsonSerializationUtils;
        using namespace ScriptCanvas;

        auto fileStringOutcome = AZ::Utils::ReadFile<AZStd::string>(path);
        if (!fileStringOutcome)
        {
            return AZ::Failure(fileStringOutcome.TakeError());
        }

        const auto& asString = fileStringOutcome.GetValue();
        DataPtr scriptCanvasData = AZStd::make_shared<ScriptCanvas::ScriptCanvasData>();
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
            entity->Init();
            entity->Activate();

            auto graph = entity->FindComponent<ScriptCanvasEditor::Graph>();
            graph->MarkOwnership(*scriptCanvasData);
        }

        return AZ::Success(ScriptCanvasEditor::SourceHandle(scriptCanvasData, {}, path));
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

        auto saveOutcome = JSRU::SaveObjectToStream<ScriptCanvas::ScriptCanvasData>(graphData, stream, nullptr, &settings);
        if (!saveOutcome.IsSuccess())
        {
            AZStd::string result = saveOutcome.TakeError();
            return AZ::Failure(AZStd::string("JSON serialization failed to save source: %s", result.c_str()));
        }

        return AZ::Success();
    }
}
