/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Component/ComponentApplicationBus.h>

#include <GraphCanvas/Components/Connections/ConnectionBus.h>
#include <GraphCanvas/Components/Nodes/NodeTitleBus.h>
#include <GraphCanvas/Components/SceneBus.h>

#include <Editor/GraphCanvas/Components/NodeDescriptors/VariableNodeDescriptorComponent.h>
#include <Editor/Include/ScriptCanvas/Bus/RequestBus.h>
#include <Editor/Translation/TranslationHelper.h>
#include <Editor/View/Widgets/PropertyGridBus.h>

#include <ScriptCanvas/Bus/EditorScriptCanvasBus.h>
#include <ScriptCanvas/Bus/RequestBus.h>

namespace ScriptCanvasEditor
{
    ////////////////////////////////////
    // VariableNodeDescriptorComponent
    ////////////////////////////////////

    void VariableNodeDescriptorComponent::Reflect(AZ::ReflectContext* reflectContext)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflectContext);

        if (serializeContext)
        {
            serializeContext->Class<VariableNodeDescriptorComponent, NodeDescriptorComponent>()
                ->Version(1)
                ;
        }
    }

    VariableNodeDescriptorComponent::VariableNodeDescriptorComponent(NodeDescriptorType descriptorType)
        : NodeDescriptorComponent(descriptorType)
    {
    }

    void VariableNodeDescriptorComponent::Activate()
    {
        NodeDescriptorComponent::Activate();

        GraphCanvas::SceneMemberNotificationBus::Handler::BusConnect(GetEntityId());
        VariableNodeDescriptorRequestBus::Handler::BusConnect(GetEntityId());
    }

    void VariableNodeDescriptorComponent::Deactivate()
    {
        ScriptCanvas::VariableNodeNotificationBus::Handler::BusDisconnect();
        VariableNodeDescriptorRequestBus::Handler::BusDisconnect();
        GraphCanvas::SceneMemberNotificationBus::Handler::BusDisconnect();

        NodeDescriptorComponent::Deactivate();
    }

    void VariableNodeDescriptorComponent::OnVariableRenamed(AZStd::string_view variableName)
    {
        UpdateTitle(variableName);
    }

    void VariableNodeDescriptorComponent::OnVariableRemoved()
    {
        AZ_Error("ScriptCanvas", false, "Removing a variable from node that is still in use. Deleting node");
        AZStd::unordered_set< AZ::EntityId > deleteIds = { GetEntityId() };

        AZ::EntityId graphId;
        GraphCanvas::SceneMemberRequestBus::EventResult(graphId, GetEntityId(), &GraphCanvas::SceneMemberRequests::GetScene);

        // After this call the component is no longer valid.
        GraphCanvas::SceneRequestBus::Event(graphId, &GraphCanvas::SceneRequests::Delete, deleteIds);
    }

    void VariableNodeDescriptorComponent::OnVariableIdChanged(const ScriptCanvas::VariableId& /*oldVariableId*/, const ScriptCanvas::VariableId& newVariableId)
    {
        ScriptCanvas::ScriptCanvasId scriptCanvasId;        

        AZ::EntityId scriptCanvasNodeId = FindScriptCanvasNodeId();
        ScriptCanvas::NodeRequestBus::EventResult(scriptCanvasId, scriptCanvasNodeId, &ScriptCanvas::NodeRequests::GetOwningScriptCanvasId);

        ScriptCanvas::VariableNotificationBus::Handler::BusDisconnect();

        ScriptCanvas::GraphScopedVariableId newScopedVariableId = ScriptCanvas::GraphScopedVariableId(scriptCanvasId, newVariableId);

        ScriptCanvas::VariableNotificationBus::Handler::BusConnect(newScopedVariableId);

        ScriptCanvas::Data::Type scriptCanvasType;
        ScriptCanvas::VariableRequestBus::EventResult(scriptCanvasType, newScopedVariableId, &ScriptCanvas::VariableRequests::GetType);

        const AZStd::string typeName = TranslationHelper::GetSafeTypeName(scriptCanvasType);
        GraphCanvas::NodeTitleRequestBus::Event(GetEntityId(), &GraphCanvas::NodeTitleRequests::SetSubTitle, typeName);

        AZ::Uuid dataType = ScriptCanvas::Data::ToAZType(scriptCanvasType);
        GraphCanvas::NodeTitleRequestBus::Event(GetEntityId(), &GraphCanvas::NodeTitleRequests::SetDataPaletteOverride, dataType);

        AZStd::string_view variableName;
        ScriptCanvas::VariableRequestBus::EventResult(variableName, newScopedVariableId, &ScriptCanvas::VariableRequests::GetName);
        UpdateTitle(variableName);

        PropertyGridRequestBus::Broadcast(&PropertyGridRequests::RebuildPropertyGrid);
    }

    void VariableNodeDescriptorComponent::OnAddedToGraphCanvasGraph([[maybe_unused]] const AZ::EntityId& sceneId, const AZ::EntityId& scriptCanvasNodeId)
    {
        OnVariableIdChanged({}, GetVariableId());

        ScriptCanvas::VariableNodeNotificationBus::Handler::BusConnect(scriptCanvasNodeId);
    }

    void VariableNodeDescriptorComponent::OnSceneMemberAboutToSerialize(GraphCanvas::GraphSerialization& graphSerialization)
    {
        auto& userDataMapRef = graphSerialization.GetUserDataMapRef();
        
        auto mapIter = userDataMapRef.find(ScriptCanvas::CopiedVariableData::k_variableKey);

        ScriptCanvas::GraphVariableMapping* variableConfigurations = nullptr;

        if (mapIter == userDataMapRef.end())
        {
            ScriptCanvas::CopiedVariableData variableData;
            auto insertResult = userDataMapRef.emplace(ScriptCanvas::CopiedVariableData::k_variableKey, variableData);

            ScriptCanvas::CopiedVariableData* copiedVariableData = AZStd::any_cast<ScriptCanvas::CopiedVariableData>(&insertResult.first->second);
            variableConfigurations = (&copiedVariableData->m_variableMapping);
        }
        else
        {
            ScriptCanvas::CopiedVariableData* copiedVariableData = AZStd::any_cast<ScriptCanvas::CopiedVariableData>(&mapIter->second);
            variableConfigurations = (&copiedVariableData->m_variableMapping);
        }

        ScriptCanvas::VariableId variableId = GetVariableId();

        if (variableConfigurations->find(variableId) == variableConfigurations->end())
        {
            ScriptCanvas::ScriptCanvasId scriptCanvasId;
            GeneralRequestBus::BroadcastResult(scriptCanvasId, &GeneralRequests::GetActiveScriptCanvasId);

            ScriptCanvas::GraphVariable* configuration = nullptr;
            ScriptCanvas::GraphVariableManagerRequestBus::EventResult(configuration, scriptCanvasId, &ScriptCanvas::GraphVariableManagerRequests::FindVariableById, variableId);

            if (configuration)
            {
                variableConfigurations->emplace(GetVariableId(), (*configuration));
            }
        }
    }

    void VariableNodeDescriptorComponent::OnSceneMemberDeserialized(const AZ::EntityId& graphCanvasGraphId, const GraphCanvas::GraphSerialization& graphSerialization)
    {
        ScriptCanvas::ScriptCanvasId scriptCanvasId;
        GeneralRequestBus::BroadcastResult(scriptCanvasId, &GeneralRequests::GetScriptCanvasId, graphCanvasGraphId);

        ScriptCanvas::GraphVariable* graphVariable = nullptr;
        ScriptCanvas::GraphVariableManagerRequestBus::EventResult(graphVariable, scriptCanvasId, &ScriptCanvas::GraphVariableManagerRequests::FindVariableById, GetVariableId());

        // If the variable is null. We need to create it from our copied data.
        if (graphVariable == nullptr)
        {
            const auto& userDataMapRef = graphSerialization.GetUserDataMapRef();

            auto mapIter = userDataMapRef.find(ScriptCanvas::CopiedVariableData::k_variableKey);

            if (mapIter != userDataMapRef.end())
            {
                const ScriptCanvas::CopiedVariableData* copiedVariableData = AZStd::any_cast<ScriptCanvas::CopiedVariableData>(&mapIter->second);
                const ScriptCanvas::GraphVariableMapping* mapping = (&copiedVariableData->m_variableMapping);

                ScriptCanvas::VariableId originalVariable = GetVariableId();
                auto variableIter = mapping->find(originalVariable);

                if (variableIter != mapping->end())
                {
                    ScriptCanvas::ScriptCanvasId scriptCanvasId2;
                    GeneralRequestBus::BroadcastResult(scriptCanvasId2, &GeneralRequests::GetScriptCanvasId, graphCanvasGraphId);
                    
                    const ScriptCanvas::GraphVariable& variableConfiguration = variableIter->second;

                    AZ::Outcome<ScriptCanvas::VariableId, AZStd::string> remapVariableOutcome = AZ::Failure(AZStd::string());
                    ScriptCanvas::GraphVariableManagerRequestBus::EventResult(remapVariableOutcome, scriptCanvasId2, &ScriptCanvas::GraphVariableManagerRequests::RemapVariable, variableConfiguration);

                    if (remapVariableOutcome)
                    {
                        SetVariableId(remapVariableOutcome.GetValue());
                    }
                }
            }
        }
    }

    ScriptCanvas::VariableId VariableNodeDescriptorComponent::GetVariableId() const
    {
        ScriptCanvas::VariableId variableId;

        AZStd::any* userData = nullptr;
        GraphCanvas::NodeRequestBus::EventResult(userData, GetEntityId(), &GraphCanvas::NodeRequests::GetUserData);

        AZ::EntityId* scNodeId = AZStd::any_cast<AZ::EntityId>(userData);
        if (scNodeId)
        {
            ScriptCanvas::VariableNodeRequestBus::EventResult(variableId, *scNodeId, &ScriptCanvas::VariableNodeRequests::GetId);
        }

        return variableId;
    }

    void VariableNodeDescriptorComponent::UpdateTitle([[maybe_unused]] AZStd::string_view variableName)
    {
        AZ_Error("ScriptCanvas", false, "Should be pure virtual function, but Pure Virtual functions on components are disallowed.");
    }

    void VariableNodeDescriptorComponent::SetVariableId(const ScriptCanvas::VariableId& variableId)
    {
        AZStd::any* userData = nullptr;
        GraphCanvas::NodeRequestBus::EventResult(userData, GetEntityId(), &GraphCanvas::NodeRequests::GetUserData);

        AZ::EntityId* scNodeId = AZStd::any_cast<AZ::EntityId>(userData);
        if (scNodeId)
        {
            ScriptCanvas::VariableNodeRequestBus::Event(*scNodeId, &ScriptCanvas::VariableNodeRequests::SetId, variableId);
        }
    }
}
