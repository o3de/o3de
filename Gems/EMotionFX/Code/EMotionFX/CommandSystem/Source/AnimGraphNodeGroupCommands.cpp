/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

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
    //--------------------------------------------------------------------------------
    // CommandAnimGraphAdjustNodeGroup
    //--------------------------------------------------------------------------------
    CommandAnimGraphAdjustNodeGroup::CommandAnimGraphAdjustNodeGroup(MCore::Command* orgCommand)
        : MCore::Command("AnimGraphAdjustNodeGroup", orgCommand)
    {
    }


    CommandAnimGraphAdjustNodeGroup::~CommandAnimGraphAdjustNodeGroup()
    {
    }


    AZStd::string CommandAnimGraphAdjustNodeGroup::GenerateNodeNameString(EMotionFX::AnimGraph* animGraph, const AZStd::vector<EMotionFX::AnimGraphNodeId>& nodeIDs)
    {
        if (nodeIDs.empty())
        {
            return "";
        }

        AZStd::string result;

        const size_t numNodes = nodeIDs.size();
        for (size_t i = 0; i < numNodes; ++i)
        {
            EMotionFX::AnimGraphNode* animGraphNode = animGraph->RecursiveFindNodeById(nodeIDs[i]);
            if (!animGraphNode)
            {
                continue;
            }

            result += animGraphNode->GetName();
            if (i < numNodes - 1)
            {
                result += ';';
            }
        }

        return result;
    }


    AZStd::vector<EMotionFX::AnimGraphNodeId> CommandAnimGraphAdjustNodeGroup::CollectNodeIdsFromGroup(EMotionFX::AnimGraphNodeGroup* nodeGroup)
    {
        AZStd::vector<EMotionFX::AnimGraphNodeId> result;

        const uint32 numNodes = nodeGroup->GetNumNodes();
        result.reserve(numNodes);
        for (uint32 i = 0; i < numNodes; ++i)
        {
            result.push_back(nodeGroup->GetNode(i));
        }

        return result;
    }


    bool CommandAnimGraphAdjustNodeGroup::Execute(const MCore::CommandLine& parameters, AZStd::string& outResult)
    {
        EMotionFX::AnimGraph* animGraph = CommandsGetAnimGraph(parameters, this, outResult);
        if (!animGraph)
        {
            return false;
        }

        // get the node group name
        AZStd::string groupName;
        parameters.GetValue("name", this, groupName);

        // find the node group index
        const uint32 groupIndex = animGraph->FindNodeGroupIndexByName(groupName.c_str());
        if (groupIndex == MCORE_INVALIDINDEX32)
        {
            outResult = AZStd::string::format("Node group \"%s\" can not be found.", groupName.c_str());
            return false;
        }

        // get a pointer to the node group and keep the old name
        EMotionFX::AnimGraphNodeGroup* nodeGroup = animGraph->GetNodeGroup(groupIndex);
        mOldName = nodeGroup->GetName();

        // is visible?
        if (parameters.CheckIfHasParameter("isVisible"))
        {
            const bool isVisible = parameters.GetValueAsBool("isVisible", this);
            mOldIsVisible = nodeGroup->GetIsVisible();
            nodeGroup->SetIsVisible(isVisible);
        }

        // background color
        if (parameters.CheckIfHasParameter("color"))
        {
            const AZ::Vector4 colorVector4 = parameters.GetValueAsVector4("color", this);
            const AZ::u32 color = AZ::Color(static_cast<float>(colorVector4.GetX()), static_cast<float>(colorVector4.GetY()), static_cast<float>(colorVector4.GetZ()), static_cast<float>(colorVector4.GetW())).ToU32();
            mOldColor = nodeGroup->GetColor();
            nodeGroup->SetColor(color);
        }

        // set the new name
        // if the new name is empty, the name is not changed
        AZStd::string newGroupName;
        parameters.GetValue("newName", this, newGroupName);
        if (!newGroupName.empty())
        {
            nodeGroup->SetName(newGroupName.c_str());
        }

        // check if parametes nodeNames is set
        if (parameters.CheckIfHasParameter("nodeNames"))
        {
            // keep the old nodes IDs
            mOldNodeIds = CollectNodeIdsFromGroup(nodeGroup);

            // get the node action
            AZStd::string nodeAction;
            parameters.GetValue("nodeAction", this, nodeAction);

            // get the node names and split the string
            AZStd::string nodeNamesString;
            parameters.GetValue("nodeNames", this, nodeNamesString);

            
            AZStd::vector<AZStd::string> nodeNames;
            AzFramework::StringFunc::Tokenize(nodeNamesString.c_str(), nodeNames, ";", false, true);

            // remove the selected nodes from the given node group
            if (AzFramework::StringFunc::Equal(nodeAction.c_str(), "remove"))
            {
                for (const AZStd::string& nodeName : nodeNames)
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
            else if (AzFramework::StringFunc::Equal(nodeAction.c_str(), "add")) // add the selected nodes to the given node group
            {
                for (const AZStd::string& nodeName : nodeNames)
                {
                    EMotionFX::AnimGraphNode* animGraphNode = animGraph->RecursiveFindNodeByName(nodeName.c_str());
                    if (!animGraphNode)
                    {
                        continue;
                    }

                    // remove the node from all node groups
                    const uint32 numNodeGroups = animGraph->GetNumNodeGroups();
                    for (uint32 n = 0; n < numNodeGroups; ++n)
                    {
                        animGraph->GetNodeGroup(n)->RemoveNodeById(animGraphNode->GetId());
                    }

                    // add the node to the given node group afterwards
                    nodeGroup->AddNode(animGraphNode->GetId());
                }
            }
            else if (AzFramework::StringFunc::Equal(nodeAction.c_str(), "replace")) // clear the node group and then add the selected nodes to the given node group
            {
                // clear the node group upfront
                nodeGroup->RemoveAllNodes();

                for (const AZStd::string& nodeName : nodeNames)
                {
                    EMotionFX::AnimGraphNode* animGraphNode = animGraph->RecursiveFindNodeByName(nodeName.c_str());
                    if (!animGraphNode)
                    {
                        continue;
                    }

                    // remove the node from all node groups
                    const uint32 numNodeGroups = animGraph->GetNumNodeGroups();
                    for (uint32 n = 0; n < numNodeGroups; ++n)
                    {
                        animGraph->GetNodeGroup(n)->RemoveNodeById(animGraphNode->GetId());
                    }

                    // add the node to the given node group afterwards
                    nodeGroup->AddNode(animGraphNode->GetId());
                }
            }
        }

        // save the current dirty flag and tell the anim graph that something got changed
        mOldDirtyFlag = animGraph->GetDirtyFlag();
        animGraph->SetDirtyFlag(true);
        return true;
    }


    // undo the command
    bool CommandAnimGraphAdjustNodeGroup::Undo(const MCore::CommandLine& parameters, AZStd::string& outResult)
    {
        EMotionFX::AnimGraph* animGraph = CommandsGetAnimGraph(parameters, this, outResult);
        if (!animGraph)
        {
            return false;
        }

        AZStd::string commandString = AZStd::string::format("AnimGraphAdjustNodeGroup -animGraphID %i", animGraph->GetID());

        // set the old name or simply set the name if the name is not changed
        if (parameters.CheckIfHasParameter("newName"))
        {
            AZStd::string newName;
            parameters.GetValue("newName", this, newName);

            commandString += AZStd::string::format(" -name \"%s\"", newName.c_str());
            commandString += AZStd::string::format(" -newName \"%s\"", mOldName.c_str());
        }
        else
        {
            commandString += AZStd::string::format(" -name \"%s\"", mOldName.c_str());
        }

        // set the old visible flag
        if (parameters.CheckIfHasParameter("isVisible"))
        {
            commandString += AZStd::string::format(" -isVisible %i", mOldIsVisible);
        }

        // set the old color
        if (parameters.CheckIfHasParameter("color"))
        {
            AZ::Color oldColor;
            oldColor.FromU32(mOldColor);
            const AZStd::string oldColorString = AZStd::string::format("%.8f,%.8f,%.8f,%.8f", static_cast<float>(oldColor.GetR()), static_cast<float>(oldColor.GetG()), static_cast<float>(oldColor.GetB()), static_cast<float>(oldColor.GetA()));

            commandString += AZStd::string::format(" -color \"%s\"", oldColorString.c_str());
        }

        // set the old nodes
        if (parameters.CheckIfHasParameter("nodeNames"))
        {
            const AZStd::string nodeNamesString = CommandAnimGraphAdjustNodeGroup::GenerateNodeNameString(animGraph, mOldNodeIds);
            commandString += AZStd::string::format(" -nodeNames \"%s\" -nodeAction \"replace\"", nodeNamesString.c_str());
        }

        // execute the command
        if (!GetCommandManager()->ExecuteCommandInsideCommand(commandString, outResult))
        {
            AZ_Error("EMotionFX", false, outResult.c_str());
        }

        // set the dirty flag back to the old value
        animGraph->SetDirtyFlag(mOldDirtyFlag);

        return true;
    }


    void CommandAnimGraphAdjustNodeGroup::InitSyntax()
    {
        GetSyntax().ReserveParameters(8);
        GetSyntax().AddRequiredParameter("name", "The name of the node group to adjust.", MCore::CommandSyntax::PARAMTYPE_STRING);
        GetSyntax().AddParameter("animGraphID", "The id of the blend set the node group belongs to.", MCore::CommandSyntax::PARAMTYPE_INT, "-1");
        GetSyntax().AddParameter("isVisible", "The visibility flag of the node group.", MCore::CommandSyntax::PARAMTYPE_BOOLEAN, "true");
        GetSyntax().AddParameter("newName", "The new name of the node group.", MCore::CommandSyntax::PARAMTYPE_STRING, "");
        GetSyntax().AddParameter("nodeNames", "A list of node names that should be added/removed to/from the node group.", MCore::CommandSyntax::PARAMTYPE_STRING, "");
        GetSyntax().AddParameter("nodeAction", "The action to perform with the nodes passed to the command.", MCore::CommandSyntax::PARAMTYPE_STRING, "select");
        GetSyntax().AddParameter("color", "The color to render the node group with.", MCore::CommandSyntax::PARAMTYPE_VECTOR4, "(1.0, 1.0, 1.0, 1.0)");
        GetSyntax().AddParameter("updateUI", "Setting this to true will trigger a refresh of the node groups UI.", MCore::CommandSyntax::PARAMTYPE_BOOLEAN, "true");
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
        const AZ::Color color = MCore::EmfxColorToAzColor(MCore::RGBAColor(MCore::GenerateColor()));
        nodeGroup->SetColor(color.ToU32());

        // save the current dirty flag and tell the anim graph that something got changed
        mOldDirtyFlag   = animGraph->GetDirtyFlag();
        mOldName        = nodeGroup->GetName();
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

        AZStd::string commandString = AZStd::string::format("AnimGraphRemoveNodeGroup -animGraphID %i -name \"%s\"", animGraph->GetID(), mOldName.c_str());

        // execute the command
        AZStd::string result;
        if (!GetCommandManager()->ExecuteCommandInsideCommand(commandString, result))
        {
            AZ_Error("EMotionFX", false, result.c_str());
        }

        // set the dirty flag back to the old value
        animGraph->SetDirtyFlag(mOldDirtyFlag);
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
        const uint32 groupIndex = animGraph->FindNodeGroupIndexByName(groupName.c_str());
        if (groupIndex == MCORE_INVALIDINDEX32)
        {
            outResult = AZStd::string::format("Cannot add node group to anim graph. Node group index %u is invalid.", groupIndex);
            return false;
        }

        // read out information for the command undo
        EMotionFX::AnimGraphNodeGroup* nodeGroup = animGraph->GetNodeGroup(groupIndex);
        mOldName        = nodeGroup->GetName();
        mOldColor       = nodeGroup->GetColor();
        mOldIsVisible   = nodeGroup->GetIsVisible();
        mOldNodeIds     = CommandAnimGraphAdjustNodeGroup::CollectNodeIdsFromGroup(nodeGroup);

        // remove the node group
        animGraph->RemoveNodeGroup(groupIndex);

        // save the current dirty flag and tell the anim graph that something got changed
        mOldDirtyFlag = animGraph->GetDirtyFlag();
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

        AZStd::string commandString = AZStd::string::format("AnimGraphAddNodeGroup -animGraphID %i -name \"%s\" -updateUI %s",animGraph->GetID(), mOldName.c_str(), updateWindow.c_str());
        commandGroup.AddCommandString(commandString);

        const AZStd::string nodeNamesString = CommandAnimGraphAdjustNodeGroup::GenerateNodeNameString(animGraph, mOldNodeIds);

        AZ::Color oldColor;
        oldColor.FromU32(mOldColor);
        const AZStd::string oldColorString = AZStd::string::format("%.8f,%.8f,%.8f,%.8f",
            static_cast<float>(oldColor.GetR()), static_cast<float>(oldColor.GetG()), static_cast<float>(oldColor.GetB()), static_cast<float>(oldColor.GetA()));

        commandString = AZStd::string::format(
            "AnimGraphAdjustNodeGroup -animGraphID %i -name \"%s\" -isVisible %s -color \"%s\" -nodeNames \"%s\" -nodeAction \"add\" -updateUI %s",
            animGraph->GetID(), mOldName.c_str(), AZStd::to_string(mOldIsVisible).c_str(), oldColorString.c_str(), nodeNamesString.c_str(), updateWindow.c_str());

        commandGroup.AddCommandString(commandString);

        AZStd::string result;
        if (!GetCommandManager()->ExecuteCommandGroupInsideCommand(commandGroup, result))
        {
            AZ_Error("EMotionFX", false, result.c_str());
        }

        // set the dirty flag back to the old value
        animGraph->SetDirtyFlag(mOldDirtyFlag);
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
        const uint32 numNodeGroups = animGraph->GetNumNodeGroups();
        if (numNodeGroups == 0)
        {
            return;
        }

        // create the command group
        MCore::CommandGroup internalCommandGroup("Clear anim graph node groups");

        // get rid of all node groups
        AZStd::string commandString;
        for (uint32 i = 0; i < numNodeGroups; ++i)
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
