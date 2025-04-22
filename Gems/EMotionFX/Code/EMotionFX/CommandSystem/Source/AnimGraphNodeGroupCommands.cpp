/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/std/optional.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include "AnimGraphNodeGroupCommands.h"
#include "AnimGraphConnectionCommands.h"
#include "CommandManager.h"
#include <EMotionFX/Source/AnimGraph.h>
#include <MCore/Source/Random.h>
#include <MCore/Source/AzCoreConversions.h>
#include <MCore/Source/StringConversions.h>
#include <EMotionFX/Source/AnimGraphNode.h>
#include <EMotionFX/Source/AnimGraphInstance.h>
#include <EMotionFX/Source/AnimGraphManager.h>
#include <EMotionFX/Source/ActorManager.h>


namespace CommandSystem
{
    AZ_CLASS_ALLOCATOR_IMPL(CommandAnimGraphAdjustNodeGroup, EMotionFX::CommandAllocator)

    //--------------------------------------------------------------------------------
    // CommandAnimGraphAdjustNodeGroup
    //--------------------------------------------------------------------------------
    CommandAnimGraphAdjustNodeGroup::CommandAnimGraphAdjustNodeGroup(
        MCore::Command* orgCommand,
        AZ::u32 animGraphId,
        AZStd::string name,
        AZStd::optional<bool> visible,
        AZStd::optional<AZStd::string> newName,
        AZStd::optional<AZStd::vector<AZStd::string>> nodeNames,
        AZStd::optional<NodeAction> nodeAction,
        AZStd::optional<AZ::u32> color,
        AZStd::optional<bool> updateUI
    )
        : MCore::Command(s_commandName, orgCommand)
        , ParameterMixinAnimGraphId(animGraphId)
        , m_name(AZStd::move(name))
        , m_isVisible(visible)
        , m_newName(AZStd::move(newName))
        , m_nodeNames(AZStd::move(nodeNames))
        , m_nodeAction(nodeAction)
        , m_color(color)
        , m_updateUI(updateUI)
    {
    }

    AZStd::vector<AZStd::string> CommandAnimGraphAdjustNodeGroup::GenerateNodeNameVector(EMotionFX::AnimGraph* animGraph, const AZStd::vector<EMotionFX::AnimGraphNodeId>& nodeIDs)
    {
        AZStd::vector<AZStd::string> result;
        for (const auto& nodeID : nodeIDs)
        {
            const EMotionFX::AnimGraphNode* animGraphNode = animGraph->RecursiveFindNodeById(nodeID);
            if (!animGraphNode)
            {
                continue;
            }
            result.emplace_back(animGraphNode->GetName());
        }
        return result;
    }


    AZStd::vector<EMotionFX::AnimGraphNodeId> CommandAnimGraphAdjustNodeGroup::CollectNodeIdsFromGroup(EMotionFX::AnimGraphNodeGroup* nodeGroup)
    {
        AZStd::vector<EMotionFX::AnimGraphNodeId> result;

        const size_t numNodes = nodeGroup->GetNumNodes();
        result.reserve(numNodes);
        for (size_t i = 0; i < numNodes; ++i)
        {
            result.push_back(nodeGroup->GetNode(i));
        }

        return result;
    }


    bool CommandAnimGraphAdjustNodeGroup::Execute(const MCore::CommandLine&, AZStd::string& outResult)
    {
        EMotionFX::AnimGraph* animGraph = EMotionFX::GetAnimGraphManager().FindAnimGraphByID(m_animGraphId);
        if (!animGraph)
        {
            return false;
        }

        // find the node group index
        const size_t groupIndex = animGraph->FindNodeGroupIndexByName(m_name.c_str());
        if (groupIndex == InvalidIndex)
        {
            outResult = AZStd::string::format("Node group \"%s\" can not be found.", m_name.c_str());
            return false;
        }

        EMotionFX::AnimGraphNodeGroup* nodeGroup = animGraph->GetNodeGroup(groupIndex);

        if (m_isVisible.has_value())
        {
            m_oldIsVisible = nodeGroup->GetIsVisible();
            nodeGroup->SetIsVisible(*m_isVisible);
        }

        if (m_color.has_value())
        {
            m_oldColor = nodeGroup->GetColor();
            nodeGroup->SetColor(*m_color);
        }

        if (m_newName.has_value())
        {
            nodeGroup->SetName(m_newName->c_str());
        }

        // check if parametes nodeNames is set
        if (m_nodeNames.has_value())
        {
            // keep the old nodes IDs
            m_oldNodeIds = CollectNodeIdsFromGroup(nodeGroup);

            // remove the selected nodes from the given node group
            if (*m_nodeAction == NodeAction::Remove)
            {
                for (const AZStd::string& nodeName : *m_nodeNames)
                {
                    EMotionFX::AnimGraphNode* animGraphNode = animGraph->RecursiveFindNodeByName(nodeName.c_str());
                    if (!animGraphNode)
                    {
                        continue;
                    }

                    // remove the node from the given node group
                    nodeGroup->RemoveNodeById(animGraphNode->GetId());
                }
            }
            else if (*m_nodeAction == NodeAction::Add)
            {
                for (const AZStd::string& nodeName : *m_nodeNames)
                {
                    EMotionFX::AnimGraphNode* animGraphNode = animGraph->RecursiveFindNodeByName(nodeName.c_str());
                    if (!animGraphNode)
                    {
                        continue;
                    }

                    // remove the node from all node groups
                    const size_t numNodeGroups = animGraph->GetNumNodeGroups();
                    for (size_t n = 0; n < numNodeGroups; ++n)
                    {
                        animGraph->GetNodeGroup(n)->RemoveNodeById(animGraphNode->GetId());
                    }

                    // add the node to the given node group afterwards
                    nodeGroup->AddNode(animGraphNode->GetId());
                }
            }
            else if (*m_nodeAction == NodeAction::Replace)
            {
                // clear the node group upfront
                nodeGroup->RemoveAllNodes();

                for (const AZStd::string& nodeName : *m_nodeNames)
                {
                    EMotionFX::AnimGraphNode* animGraphNode = animGraph->RecursiveFindNodeByName(nodeName.c_str());
                    if (!animGraphNode)
                    {
                        continue;
                    }

                    // remove the node from all node groups
                    const size_t numNodeGroups = animGraph->GetNumNodeGroups();
                    for (size_t n = 0; n < numNodeGroups; ++n)
                    {
                        animGraph->GetNodeGroup(n)->RemoveNodeById(animGraphNode->GetId());
                    }

                    // add the node to the given node group afterwards
                    nodeGroup->AddNode(animGraphNode->GetId());
                }
            }
        }

        // save the current dirty flag and tell the anim graph that something got changed
        m_oldDirtyFlag = animGraph->GetDirtyFlag();
        animGraph->SetDirtyFlag(true);
        return true;
    }


    // undo the command
    bool CommandAnimGraphAdjustNodeGroup::Undo(const MCore::CommandLine&, AZStd::string& outResult)
    {
        EMotionFX::AnimGraph* animGraph = EMotionFX::GetAnimGraphManager().FindAnimGraphByID(m_animGraphId);
        if (!animGraph)
        {
            return false;
        }

        CommandAnimGraphAdjustNodeGroup* command = aznew CommandAnimGraphAdjustNodeGroup(
            GetCommandManager()->FindCommand(CommandAnimGraphAdjustNodeGroup::s_commandName),
            /*animGraphId = */ m_animGraphId,
            /*name = */ m_newName.has_value() ? *m_newName : m_name,
            /*visible = */ m_isVisible.has_value() ? AZStd::optional<bool>(m_oldIsVisible) : AZStd::nullopt,
            /*newName = */ m_newName.has_value() ? AZStd::optional<AZStd::string>(m_name) : AZStd::nullopt,
            /*nodeNames = */ m_nodeNames.has_value() ? AZStd::optional<AZStd::vector<AZStd::string>>(GenerateNodeNameVector(animGraph, m_oldNodeIds)) : AZStd::nullopt,
            /*nodeAction = */ m_nodeNames.has_value() ? AZStd::optional<NodeAction>(NodeAction::Replace) : AZStd::nullopt,
            /*color = */ m_color.has_value() ? AZStd::optional<AZ::u32>(m_oldColor) : AZStd::nullopt
        );

        // execute the command
        if (!GetCommandManager()->ExecuteCommandInsideCommand(command, outResult))
        {
            AZ_Error("EMotionFX", false, outResult.c_str());
        }

        // set the dirty flag back to the old value
        animGraph->SetDirtyFlag(m_oldDirtyFlag);

        return true;
    }


    void CommandAnimGraphAdjustNodeGroup::InitSyntax()
    {
        GetSyntax().ReserveParameters(8);
        GetSyntax().AddRequiredParameter("name", "The name of the node group to adjust.", MCore::CommandSyntax::PARAMTYPE_STRING);
        EMotionFX::ParameterMixinAnimGraphId::InitSyntax(GetSyntax(), /*isParameterRequired=*/ false);
        GetSyntax().AddParameter("isVisible", "The visibility flag of the node group.", MCore::CommandSyntax::PARAMTYPE_BOOLEAN, "true");
        GetSyntax().AddParameter("newName", "The new name of the node group.", MCore::CommandSyntax::PARAMTYPE_STRING, "");
        GetSyntax().AddParameter("nodeNames", "A list of node names that should be added/removed to/from the node group.", MCore::CommandSyntax::PARAMTYPE_STRING, "");
        GetSyntax().AddParameter("nodeAction", "The action to perform with the nodes passed to the command.", MCore::CommandSyntax::PARAMTYPE_STRING, "select");
        GetSyntax().AddParameter("color", "The color to render the node group with.", MCore::CommandSyntax::PARAMTYPE_VECTOR4, "(1.0, 1.0, 1.0, 1.0)");
        GetSyntax().AddParameter("updateUI", "Setting this to true will trigger a refresh of the node groups UI.", MCore::CommandSyntax::PARAMTYPE_BOOLEAN, "true");
    }

    bool CommandAnimGraphAdjustNodeGroup::SetCommandParameters(const MCore::CommandLine& parameters)
    {
        EMotionFX::ParameterMixinAnimGraphId::SetCommandParameters(parameters);
        m_name = parameters.GetValue("name", this);

        if (parameters.CheckIfHasParameter("isVisible"))
        {
            m_isVisible = parameters.GetValueAsBool("isVisible", this);
        }
        if (parameters.CheckIfHasParameter("newName"))
        {
            m_newName = parameters.GetValue("newName", this);
        }
        if (parameters.CheckIfHasParameter("nodeNames"))
        {
            m_nodeNames.emplace();
            AzFramework::StringFunc::Tokenize(parameters.GetValue("nodeNames", this), m_nodeNames.value(), ";", false, true);
        }
        if (parameters.CheckIfHasValue("nodeAction"))
        {
            const AZStd::string& nodeActionStr = parameters.GetValue("nodeAction", this);
            if (nodeActionStr == "add")
            {
                m_nodeAction = NodeAction::Add;
            }
            else if (nodeActionStr == "remove")
            {
                m_nodeAction = NodeAction::Remove;
            }
            else if (nodeActionStr == "replace")
            {
                m_nodeAction = NodeAction::Replace;
            }
        }
        if (parameters.CheckIfHasParameter("color"))
        {
            m_color = AZ::Color(parameters.GetValueAsVector4("color", this)).ToU32();
        }
        if (parameters.CheckIfHasParameter("updateUI"))
        {
            m_updateUI = parameters.GetValueAsBool("updateUI", this);
        }

        return true;
    }

    const char* CommandAnimGraphAdjustNodeGroup::GetDescription() const
    {
        return "This command can be used to adjust the node groups of the given anim graph.";
    }

    //--------------------------------------------------------------------------------
    // CommandAnimGraphAddNodeGroup
    //--------------------------------------------------------------------------------
    CommandAnimGraphAddNodeGroup::CommandAnimGraphAddNodeGroup(MCore::Command* orgCommand)
        : MCore::Command("AnimGraphAddNodeGroup", orgCommand)
    {
    }


    CommandAnimGraphAddNodeGroup::~CommandAnimGraphAddNodeGroup()
    {
    }


    bool CommandAnimGraphAddNodeGroup::Execute(const MCore::CommandLine& parameters, AZStd::string& outResult)
    {
        EMotionFX::AnimGraph* animGraph = CommandsGetAnimGraph(parameters, this, outResult);
        if (!animGraph)
        {
            return false;
        }

        AZStd::string valueString;
        if (parameters.CheckIfHasParameter("name"))
        {
            parameters.GetValue("name", this, &valueString);
        }
        else
        {
            // generate a unique parameter name
            valueString = MCore::GenerateUniqueString("NodeGroup", [&](const AZStd::string& value)
                {
                    return (animGraph->FindNodeGroupByName(value.c_str()) == nullptr);
                });
        }

        // add new node group to the anim graph
        EMotionFX::AnimGraphNodeGroup* nodeGroup = aznew EMotionFX::AnimGraphNodeGroup(valueString.c_str());
        animGraph->AddNodeGroup(nodeGroup);

        // give the node group a random color
        nodeGroup->SetColor(EMotionFX::AnimGraph::RandomGraphColor().ToU32());

        // save the current dirty flag and tell the anim graph that something got changed
        m_oldDirtyFlag   = animGraph->GetDirtyFlag();
        m_oldName        = nodeGroup->GetName();
        animGraph->SetDirtyFlag(true);
        return true;
    }


    bool CommandAnimGraphAddNodeGroup::Undo(const MCore::CommandLine& parameters, AZStd::string& outResult)
    {
        EMotionFX::AnimGraph* animGraph = CommandsGetAnimGraph(parameters, this, outResult);
        if (!animGraph)
        {
            return false;
        }

        AZStd::string commandString = AZStd::string::format("AnimGraphRemoveNodeGroup -animGraphID %i -name \"%s\"", animGraph->GetID(), m_oldName.c_str());

        // execute the command
        AZStd::string result;
        if (!GetCommandManager()->ExecuteCommandInsideCommand(commandString, result))
        {
            AZ_Error("EMotionFX", false, result.c_str());
        }

        // set the dirty flag back to the old value
        animGraph->SetDirtyFlag(m_oldDirtyFlag);
        return true;
    }


    void CommandAnimGraphAddNodeGroup::InitSyntax()
    {
        GetSyntax().ReserveParameters(3);
        GetSyntax().AddRequiredParameter("animGraphID", "The id of the blend set the node group belongs to.", MCore::CommandSyntax::PARAMTYPE_INT);
        GetSyntax().AddParameter("name", "The name of the node group.", MCore::CommandSyntax::PARAMTYPE_STRING, "Unnamed Node Group");
        GetSyntax().AddParameter("updateUI", "Setting this to true will trigger a refresh of the node groups UI.", MCore::CommandSyntax::PARAMTYPE_BOOLEAN, "true");
    }


    const char* CommandAnimGraphAddNodeGroup::GetDescription() const
    {
        return "This command can be used to add a new node group to the given anim graph.";
    }

    //--------------------------------------------------------------------------------
    // CommandAnimGraphRemoveNodeGroup
    //--------------------------------------------------------------------------------
    CommandAnimGraphRemoveNodeGroup::CommandAnimGraphRemoveNodeGroup(MCore::Command* orgCommand)
        : MCore::Command("AnimGraphRemoveNodeGroup", orgCommand)
    {
    }


    CommandAnimGraphRemoveNodeGroup::~CommandAnimGraphRemoveNodeGroup()
    {
    }


    bool CommandAnimGraphRemoveNodeGroup::Execute(const MCore::CommandLine& parameters, AZStd::string& outResult)
    {
        EMotionFX::AnimGraph* animGraph = CommandsGetAnimGraph(parameters, this, outResult);
        if (!animGraph)
        {
            return false;
        }

        AZStd::string groupName;
        parameters.GetValue("name", this, groupName);

        // find the node group index and remove it
        const size_t groupIndex = animGraph->FindNodeGroupIndexByName(groupName.c_str());
        if (groupIndex == InvalidIndex)
        {
            outResult = AZStd::string::format("Cannot add node group to anim graph. Node group index %zu is invalid.", groupIndex);
            return false;
        }

        // read out information for the command undo
        EMotionFX::AnimGraphNodeGroup* nodeGroup = animGraph->GetNodeGroup(groupIndex);
        m_oldName        = nodeGroup->GetName();
        m_oldColor       = nodeGroup->GetColor();
        m_oldIsVisible   = nodeGroup->GetIsVisible();
        m_oldNodeIds     = CommandAnimGraphAdjustNodeGroup::CollectNodeIdsFromGroup(nodeGroup);

        // remove the node group
        animGraph->RemoveNodeGroup(groupIndex);

        // save the current dirty flag and tell the anim graph that something got changed
        m_oldDirtyFlag = animGraph->GetDirtyFlag();
        animGraph->SetDirtyFlag(true);
        return true;
    }


    bool CommandAnimGraphRemoveNodeGroup::Undo(const MCore::CommandLine& parameters, AZStd::string& outResult)
    {
        EMotionFX::AnimGraph* animGraph = CommandsGetAnimGraph(parameters, this, outResult);
        if (!animGraph)
        {
            return false;
        }
        const AZStd::string updateWindow = parameters.GetValue("updateUI",this);
        
        MCore::CommandGroup commandGroup;

        commandGroup.AddCommandString(AZStd::string::format("AnimGraphAddNodeGroup -animGraphID %i -name \"%s\" -updateUI %s",animGraph->GetID(), m_oldName.c_str(), updateWindow.c_str()));

        auto* command = aznew CommandAnimGraphAdjustNodeGroup(
            GetCommandManager()->FindCommand(CommandAnimGraphAdjustNodeGroup::s_commandName),
            /*animGraphId = */ animGraph->GetID(),
            /*name = */ m_oldName,
            /*visible = */ m_oldIsVisible,
            /*newName = */ AZStd::nullopt,
            /*nodeNames = */ CommandAnimGraphAdjustNodeGroup::GenerateNodeNameVector(animGraph, m_oldNodeIds),
            /*nodeAction = */ CommandAnimGraphAdjustNodeGroup::NodeAction::Add,
            /*color = */ m_oldColor
        );

        commandGroup.AddCommand(command);

        AZStd::string result;
        if (!GetCommandManager()->ExecuteCommandGroupInsideCommand(commandGroup, result))
        {
            AZ_Error("EMotionFX", false, result.c_str());
        }

        // set the dirty flag back to the old value
        animGraph->SetDirtyFlag(m_oldDirtyFlag);
        return true;
    }


    void CommandAnimGraphRemoveNodeGroup::InitSyntax()
    {
        GetSyntax().ReserveParameters(3);
        GetSyntax().AddRequiredParameter("animGraphID", "The id of the blend set the node group belongs to.", MCore::CommandSyntax::PARAMTYPE_INT);
        GetSyntax().AddRequiredParameter("name", "The name of the node group to remove.", MCore::CommandSyntax::PARAMTYPE_STRING);
        GetSyntax().AddParameter("updateUI", "Setting this to true will trigger a refresh of the node groups UI.", MCore::CommandSyntax::PARAMTYPE_BOOLEAN, "true");
    }


    const char* CommandAnimGraphRemoveNodeGroup::GetDescription() const
    {
        return "This command can be used to remove a node group from the given anim graph.";
    }


    //--------------------------------------------------------------------------------
    // Helper functions
    //--------------------------------------------------------------------------------
    void ClearNodeGroups(EMotionFX::AnimGraph* animGraph, MCore::CommandGroup* commandGroup)
    {
        // get number of node groups
        const size_t numNodeGroups = animGraph->GetNumNodeGroups();
        if (numNodeGroups == 0)
        {
            return;
        }

        // create the command group
        MCore::CommandGroup internalCommandGroup("Clear anim graph node groups");

        // get rid of all node groups
        AZStd::string commandString;
        for (size_t i = 0; i < numNodeGroups; ++i)
        {
            // get pointer to the current actor instance
            EMotionFX::AnimGraphNodeGroup* nodeGroup = animGraph->GetNodeGroup(i);

            // Only update/reinit node group window during first and last command to reduce runtime.
            if (i == 0 || i == numNodeGroups - 1)
            {
                commandString = AZStd::string::format("AnimGraphRemoveNodeGroup -animGraphID %i -name \"%s\"", animGraph->GetID(), nodeGroup->GetName());
            }
            else
            {
                commandString = AZStd::string::format("AnimGraphRemoveNodeGroup -animGraphID %i -name \"%s\" -updateUI false", animGraph->GetID(), nodeGroup->GetName());
            }
            // add the command to the command group
            if (!commandGroup)
            {
                internalCommandGroup.AddCommandString(commandString);
            }
            else
            {
                commandGroup->AddCommandString(commandString);
            }
        }

        // execute the command or add it to the given command group
        if (!commandGroup)
        {
            AZStd::string result;
            if (!GetCommandManager()->ExecuteCommandGroup(internalCommandGroup, result))
            {
                AZ_Error("EMotionFX", false, result.c_str());
            }
        }
    }
} // namespace CommandSystem
