/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

// AZ
#include <AzFramework/StringFunc/StringFunc.h>

// Graph Model
#include <GraphModel/Model/Graph.h>
#include <GraphModel/Model/Slot.h>
#include <GraphModel/Model/Module/ModuleNode.h>
#include <GraphModel/Model/Module/ModuleGraphManager.h>
#include <GraphModel/Model/Module/InputOutputNodes.h>
#include <GraphModel/Model/GraphContext.h>

namespace GraphModel
{
 
    void ModuleNode::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (serializeContext)
        {
            serializeContext->Class<ModuleNode, Node>()
                ->Version(0)
                ->Field("m_moduleGraphFileId", &ModuleNode::m_moduleGraphFileId)
                ->Field("m_nodeTitle", &ModuleNode::m_nodeTitle)
                ;
        }
    }
    
    ModuleNode::ModuleNode(GraphPtr ownerGraph, AZ::Uuid moduleGraphFileId, AZStd::string_view moduleGraphFileName)
        : Node(ownerGraph)
        , m_moduleGraphFileId(moduleGraphFileId)
    {
        // The module file name (without extension) is the node title
        if (!AzFramework::StringFunc::Path::GetFileName(moduleGraphFileName.data(), m_nodeTitle))
        {
            AZ_Error(ownerGraph->GetSystemName(), false, "Could not get node name from file string [%s]", moduleGraphFileName.data());
        }

        LoadModuleGraph(ownerGraph->GetContext()->GetModuleGraphManager());

        RegisterSlots();

        CreateSlotData();
    }

    void ModuleNode::PostLoadSetup(GraphPtr ownerGraph, NodeId id)
    {
        LoadModuleGraph(ownerGraph->GetContext()->GetModuleGraphManager());

        Node::PostLoadSetup(ownerGraph, id);
    }

    const char* ModuleNode::GetTitle() const
    {
        return m_nodeTitle.c_str();
    }

    void ModuleNode::LoadModuleGraph(ModuleGraphManagerPtr moduleGraphManager)
    {
        auto result = moduleGraphManager->GetModuleGraph(m_moduleGraphFileId);
        if (result.IsSuccess())
        {
            m_moduleGraph = result.GetValue();
        }
        else
        {
            AZ_Warning(GetGraph()->GetSystemName(), false, "%s (Module Node [%s])", result.GetError().data(), m_nodeTitle.data());
        }
    }

    void ModuleNode::RegisterSlots()
    {
        if (m_moduleGraph)
        {
            for (auto iter : m_moduleGraph->GetNodes())
            {
                ConstNodePtr node = iter.second;
                if (AZStd::shared_ptr<const GraphInputNode> inputNode = azrtti_cast<const GraphInputNode*>(node))
                {
                    RegisterSlot(GraphModel::SlotDefinition::CreateInputData(
                        inputNode->GetName(), 
                        inputNode->GetDisplayName(), 
                        inputNode->GetNodeDataType(),
                        inputNode->GetDefaultValue(), 
                        inputNode->GetDescription()));
                }
                else if (AZStd::shared_ptr<const GraphOutputNode> outputNode = azrtti_cast<const GraphOutputNode*>(node))
                {
                    RegisterSlot(GraphModel::SlotDefinition::CreateOutputData(
                        outputNode->GetName(), 
                        outputNode->GetDisplayName(), 
                        outputNode->GetNodeDataType(), 
                        outputNode->GetDescription()));
                }
            }
        }
    }
}
