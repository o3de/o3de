/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#include "precompiled.h"
#include "EntityMimeDataHandler.h"

#include <QMimeData>
#include <QDragEnterEvent>
#include <QDragMoveEvent>
#include <QDropEvent>
#include <QGraphicsView>

#include <Editor/Nodes/NodeUtils.h>
#include <ScriptCanvas/Bus/RequestBus.h>
#include <ScriptCanvas/Bus/NodeIdPair.h>

#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzToolsFramework/ToolsComponents/EditorEntityIdContainer.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzCore/Component/Entity.h>

#include <GraphCanvas/Components/SceneBus.h>
#include <GraphCanvas/Components/GridBus.h>
#include <GraphCanvas/Components/ViewBus.h>
#include <GraphCanvas/Components/VisualBus.h>
#include <Core/GraphBus.h>

#include <ScriptCanvas/Variable/VariableBus.h>
#include <GraphCanvas/Utils/GraphUtils.h>

namespace ScriptCanvasEditor
{
    namespace EntityMimeData
    {
        static QString GetMimeType()
        {
            return AzToolsFramework::EditorEntityIdContainer::GetMimeType();
        }
    }

    void EntityMimeDataHandler::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (!serializeContext)
        {
            return;
        }

        serializeContext->Class<EntityMimeDataHandler, AZ::Component>()
            ->Version(1)
            ;
    }

    EntityMimeDataHandler::EntityMimeDataHandler()
    {}

    void EntityMimeDataHandler::Activate()
    {
        GraphCanvas::SceneMimeDelegateHandlerRequestBus::Handler::BusConnect(GetEntityId());
    }

    void EntityMimeDataHandler::Deactivate()
    {
        GraphCanvas::SceneMimeDelegateHandlerRequestBus::Handler::BusDisconnect();
    }
    
    bool EntityMimeDataHandler::IsInterestedInMimeData(const AZ::EntityId& sceneId, const QMimeData* mimeData)
    {        
        (void)sceneId;

        return mimeData->hasFormat(EntityMimeData::GetMimeType());
    }    

    void EntityMimeDataHandler::HandleMove(const AZ::EntityId&, const QPointF&, const QMimeData*)
    {
    }

    void EntityMimeDataHandler::HandleDrop(const AZ::EntityId& graphCanvasGraphId, const QPointF& dropPoint, const QMimeData* mimeData)
    {
        if (!mimeData->hasFormat(EntityMimeData::GetMimeType()))
        {
            return;
        }

        QByteArray arrayData = mimeData->data(EntityMimeData::GetMimeType());

        AzToolsFramework::EditorEntityIdContainer entityIdListContainer;
        if (!entityIdListContainer.FromBuffer(arrayData.constData(), arrayData.size()) || entityIdListContainer.m_entityIds.empty())
        {
            return;
        }

        bool areEntitiesEditable = true;
        AzToolsFramework::ToolsApplicationRequests::Bus::BroadcastResult(areEntitiesEditable, &AzToolsFramework::ToolsApplicationRequests::AreEntitiesEditable, entityIdListContainer.m_entityIds);
        if (!areEntitiesEditable)
        {
            return;
        }

        ScriptCanvas::ScriptCanvasId scriptCanvasId;
        GeneralRequestBus::BroadcastResult(scriptCanvasId, &GeneralRequests::GetScriptCanvasId, graphCanvasGraphId);

        ScriptCanvas::GraphVariableManagerRequests* variableManagerRequests = ScriptCanvas::GraphVariableManagerRequestBus::FindFirstHandler(scriptCanvasId);

        if (variableManagerRequests == nullptr)
        {
            return;
        }

        AZStd::vector< ScriptCanvas::VariableId > variableIds;
        variableIds.reserve(entityIdListContainer.m_entityIds.size());

        {
            GraphCanvas::ScopedGraphUndoBlocker undoBlocker(graphCanvasGraphId);

            AZ::Vector2 pos(aznumeric_cast<float>(dropPoint.x()), aznumeric_cast<float>(dropPoint.y()));            

            for (const AZ::EntityId& entityId : entityIdListContainer.m_entityIds)
            {
                AZStd::string variableName;

                AZ::Entity* entity = nullptr;
                AZ::ComponentApplicationBus::BroadcastResult(entity, &AZ::ComponentApplicationRequests::FindEntity, entityId);

                if (entity)
                {
                    // Because we add in the entity id to this.
                    // we make the name mostly unique.
                    // If we just use the name, we'll run into some potential rename issues when looking things up.
                    variableName = AZStd::string::format("%s %s", entity->GetName().c_str(), entityId.ToString().c_str());
                    
                    ScriptCanvas::GraphVariable* graphVariable = variableManagerRequests->FindVariable(variableName);

                    int counter = 0;

                    AZStd::string baseName = variableName;
                    baseName.append(" (Copy)");

                    // If the variable datum already exists. That means we already have a reference to that. So we don't need to create it.
                    while (graphVariable != nullptr)
                    {
                        if (graphVariable->GetDataType() == ScriptCanvas::Data::Type::EntityID())
                        {
                            variableIds.emplace_back(graphVariable->GetVariableId());
                            break;
                        }

                        if (counter == 0)
                        {
                            variableName = baseName;
                        }
                        else
                        {
                            variableName = AZStd::string::format("%s (%i)", baseName.c_str(), counter);
                        }

                        ++counter;
                        
                        graphVariable = variableManagerRequests->FindVariable(variableName);
                    }

                    if (graphVariable == nullptr)
                    {
                        ScriptCanvas::Datum datum = ScriptCanvas::Datum(entityId);

                        AZ::Outcome<ScriptCanvas::VariableId, AZStd::string > addVariableOutcome = variableManagerRequests->AddVariable(variableName, datum);

                        if (addVariableOutcome.IsSuccess())
                        {
                            variableIds.emplace_back(addVariableOutcome.GetValue());
                        }
                    }
                }
            }

            if (!variableIds.empty())
            {
                AZ::EntityId gridId;
                GraphCanvas::SceneRequestBus::EventResult(gridId, graphCanvasGraphId, &GraphCanvas::SceneRequests::GetGrid);

                AZ::Vector2 gridStep;
                GraphCanvas::GridRequestBus::EventResult(gridStep, gridId, &GraphCanvas::GridRequests::GetMinorPitch);

                for (const ScriptCanvas::VariableId& variableId : variableIds)
                {
                    NodeIdPair nodePair = Nodes::CreateGetVariableNode(variableId, scriptCanvasId);
                    GraphCanvas::SceneRequestBus::Event(graphCanvasGraphId, &GraphCanvas::SceneRequests::AddNode, nodePair.m_graphCanvasId, pos);
                    pos += gridStep;
                }
                
            }
        }

        if (!variableIds.empty())
        {
            GeneralRequestBus::Broadcast(&GeneralRequests::PostUndoPoint, scriptCanvasId);
        }
    }

    void EntityMimeDataHandler::HandleLeave(const AZ::EntityId&, const QMimeData*)
    {

    }
} // namespace ScriptCanvasEditor
