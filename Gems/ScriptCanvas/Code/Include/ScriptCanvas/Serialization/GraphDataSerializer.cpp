/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Serialization/Json/JsonSerialization.h>
#include <ScriptCanvas/Core/SerializationListener.h>
#include <ScriptCanvas/Core/GraphData.h>
#include <ScriptCanvas/Serialization/GraphDataSerializer.h>

using namespace ScriptCanvas;

namespace GraphDataSerializerCpp
{
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

namespace AZ
{
    AZ_CLASS_ALLOCATOR_IMPL(GraphDataSerializer, SystemAllocator, 0);

    JsonSerializationResult::Result GraphDataSerializer::Load
        ( void* outputValue
        , [[maybe_unused]] const Uuid& outputValueTypeId
        , const rapidjson::Value& inputValue
        , JsonDeserializerContext& context)
    {
        namespace JSR = JsonSerializationResult;

        AZ_Assert(outputValueTypeId == azrtti_typeid<GraphData>()
            , "RuntimeVariableSerializer Load against output typeID that was not GraphData");
        AZ_Assert(outputValue, "RuntimeVariableSerializer Load against null output");
        context.GetMetadata().Add(SerializationListeners());
        JSR::ResultCode result(JSR::Tasks::ReadField);
        result.Combine(ContinueLoading(outputValue, outputValueTypeId, inputValue, context, ContinuationFlags::NoTypeSerializer));
        auto listeners = context.GetMetadata().Find<SerializationListeners>();
        AZ_Assert(listeners, "Failed to create SerializationListeners");
        GraphDataSerializerCpp::CollectNodes(reinterpret_cast<GraphData*>(outputValue)->m_nodes, *listeners);

        for (auto listener : *listeners)
        {
            listener->OnDeserialize();
        }

        reinterpret_cast<GraphData*>(outputValue)->OnDeserialized();

        return context.Report(result, result.GetProcessing() != JSR::Processing::Halted
            ? "GraphDataSerializer Load finished loading GraphData"
            : "GraphDataSerializer Load failed to load GraphData");
    }

    JsonSerializationResult::Result GraphDataSerializer::Store
        ( rapidjson::Value& outputValue
        , const void* inputValue
        , const void* defaultValue
        , [[maybe_unused]] const Uuid& valueTypeId
        , JsonSerializerContext& context)
    {
        namespace JSR = JsonSerializationResult;

        AZ_Assert(valueTypeId == azrtti_typeid<GraphData>()
            , "RuntimeVariableSerializer Store against output typeID that was not GraphData");
        AZ_Assert(inputValue, "RuntimeVariableSerializer Store against null output");
        context.GetMetadata().Add(SerializationListeners());
        auto listeners = context.GetMetadata().Find<SerializationListeners>();
        GraphDataSerializerCpp::CollectNodes(reinterpret_cast<const GraphData*>(inputValue)->m_nodes, *listeners);

        for (auto listener : *listeners)
        {
            listener->OnSerializeBegin();
        }

        JSR::ResultCode result(JSR::Tasks::WriteValue);
        result.Combine(ContinueStoring(outputValue, inputValue, defaultValue, valueTypeId, context, ContinuationFlags::NoTypeSerializer));

        for (auto listener : *listeners)
        {
            listener->OnSerializeEnd();
        }

        return context.Report(result, result.GetProcessing() != JSR::Processing::Halted
            ? "GraphDataSerializer::Store finished storing GraphData"
            : "GraphDataSerializer::Store failed to store GraphData");
    }
}
