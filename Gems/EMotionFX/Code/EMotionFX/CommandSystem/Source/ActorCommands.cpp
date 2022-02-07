/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <EMotionFX/Source/Actor.h>
#include <EMotionFX/Source/EventManager.h>
#include <EMotionFX/Source/Recorder.h>
#include <EMotionFX/Exporters/ExporterLib/Exporter/ExporterFileProcessor.h>
#include <EMotionFX/Exporters/ExporterLib/Exporter/Exporter.h>
#include <EMotionFX/Source/ActorManager.h>
#include <EMotionFX/Source/AnimGraphManager.h>
#include <EMotionFX/Source/AnimGraph.h>
#include <EMotionFX/Source/AnimGraphObject.h>
#include <EMotionFX/Source/TransformData.h>
#include <EMotionFX/Source/Mesh.h>
#include <MCore/Source/FileSystem.h>
#include "ActorCommands.h"
#include "CommandManager.h"
#include <EMotionFX/Source/EMotionFXManager.h>
#include <AzFramework/API/ApplicationAPI.h>
#include <Source/Integration/Assets/ActorAsset.h>


namespace CommandSystem
{
    //--------------------------------------------------------------------------------
    // CommandAdjustActor
    //--------------------------------------------------------------------------------
    CommandAdjustActor::CommandAdjustActor(MCore::Command* orgCommand)
        : MCore::Command("AdjustActor", orgCommand)
    {
    }


    CommandAdjustActor::~CommandAdjustActor()
    {
    }


    bool CommandAdjustActor::Execute(const MCore::CommandLine& parameters, AZStd::string& outResult)
    {
        const uint32 actorID = parameters.GetValueAsInt("actorID", MCORE_INVALIDINDEX32);

        // Find the actor based on the given id.
        EMotionFX::Actor* actor = EMotionFX::GetActorManager().FindActorByID(actorID);
        if (!actor)
        {
            outResult = AZStd::string::format("Cannot adjust actor. Actor ID %i is not valid.", actorID);
            return false;
        }

        const EMotionFX::Skeleton* skeleton = actor->GetSkeleton();

        // Set motion extraction node.
        if (parameters.CheckIfHasParameter("motionExtractionNodeName"))
        {
            m_oldMotionExtractionNodeIndex = actor->GetMotionExtractionNodeIndex();

            AZStd::string motionExtractionNodeName;
            parameters.GetValue("motionExtractionNodeName", this, motionExtractionNodeName);
            if (motionExtractionNodeName.empty() || motionExtractionNodeName == "$NULL$")
            {
                actor->SetMotionExtractionNode(nullptr);
            }
            else
            {
                EMotionFX::Node* node = skeleton->FindNodeByName(motionExtractionNodeName);
                actor->SetMotionExtractionNode(node);
            }

            // Inform all animgraph nodes about this.
            const size_t numAnimGraphs = EMotionFX::GetAnimGraphManager().GetNumAnimGraphs();
            for (size_t i = 0; i < numAnimGraphs; ++i)
            {
                EMotionFX::AnimGraph* animGraph = EMotionFX::GetAnimGraphManager().GetAnimGraph(i);
                if (animGraph->GetIsOwnedByRuntime())
                {
                    continue;
                }
                const size_t numObjects = animGraph->GetNumObjects();
                for (size_t n = 0; n < numObjects; ++n)
                {
                    animGraph->GetObject(n)->OnActorMotionExtractionNodeChanged();
                }
            }
        }

        // Set retarget root node.
        if (parameters.CheckIfHasParameter("retargetRootNodeName"))
        {
            m_oldRetargetRootNodeIndex = actor->GetRetargetRootNodeIndex();

            AZStd::string retargetRootNodeName = parameters.GetValue("retargetRootNodeName", this);
            if (retargetRootNodeName.empty() || retargetRootNodeName == "$NULL$")
            {
                actor->SetRetargetRootNode(nullptr);
            }
            else
            {
                EMotionFX::Node* node = skeleton->FindNodeByName(retargetRootNodeName);
                actor->SetRetargetRootNode(node);
            }
        }

        // Set actor name.
        if (parameters.CheckIfHasParameter("name"))
        {
            m_oldName = actor->GetName();

            AZStd::string actorName;
            parameters.GetValue("name", this, actorName);
            actor->SetName(actorName.c_str());
        }

        // Adjust the attachment nodes.
        if (parameters.CheckIfHasParameter("attachmentNodes"))
        {
            // Store old attachment nodes for undo.
            m_oldAttachmentNodes = "";
            const size_t numNodes = actor->GetNumNodes();
            for (size_t i = 0; i < numNodes; ++i)
            {
                EMotionFX::Node* node = skeleton->GetNode(i);
                if (!node)
                {
                    continue;
                }

                // Check if the node has the attachment flag enabled and add it.
                if (node->GetIsAttachmentNode())
                {
                    m_oldAttachmentNodes += node->GetName();
                    m_oldAttachmentNodes += ";";
                }
            }

            AZStd::string nodeAction;
            parameters.GetValue("nodeAction", "select", nodeAction);

            AZStd::string attachmentNodes;
            parameters.GetValue("attachmentNodes", this, attachmentNodes);

            AZStd::vector<AZStd::string> nodeNames;
            AzFramework::StringFunc::Tokenize(attachmentNodes.c_str(), nodeNames, ";", false, true);

            // Remove the given nodes from the attachment node list by unsetting the flag.
            if (AzFramework::StringFunc::Equal(nodeAction.c_str(), "remove"))
            {
                for (const AZStd::string& nodeName : nodeNames)
                {
                    EMotionFX::Node* node = skeleton->FindNodeByName(nodeName);
                    if (!node)
                    {
                        continue;
                    }

                    node->SetIsAttachmentNode(false);
                }
            }
            // Add the given nodes to the attachment node list by setting attachment flag.
            else if (AzFramework::StringFunc::Equal(nodeAction.c_str(), "add"))
            {
                for (const AZStd::string& nodeName : nodeNames)
                {
                    EMotionFX::Node* node = skeleton->FindNodeByName(nodeName);
                    if (!node)
                    {
                        continue;
                    }

                    node->SetIsAttachmentNode(true);
                }
            }
            else
            {
                // Disable attachment node flag for all nodes.
                SetIsAttachmentNode(actor, false);

                // Set attachment node flag based on selection list.
                for (const AZStd::string& nodeName : nodeNames)
                {
                    EMotionFX::Node* node = skeleton->FindNodeByName(nodeName);
                    if (!node)
                    {
                        continue;
                    }

                    node->SetIsAttachmentNode(true);
                }
            }
        }

        // Adjust the nodes that are excluded from the bounding volume calculations.
        if (parameters.CheckIfHasParameter("nodesExcludedFromBounds"))
        {
            // Store old nodes for undo.
            m_oldExcludedFromBoundsNodes = "";
            const size_t numNodes = actor->GetNumNodes();
            for (size_t i = 0; i < numNodes; ++i)
            {
                EMotionFX::Node* node = skeleton->GetNode(i);
                if (!node)
                {
                    continue;
                }

                // Check if the node has the attachment flag enabled and add it.
                if (!node->GetIncludeInBoundsCalc())
                {
                    m_oldExcludedFromBoundsNodes += node->GetName();
                    m_oldExcludedFromBoundsNodes += ";";
                }
            }

            AZStd::string nodeAction;
            parameters.GetValue("nodeAction", "select", nodeAction);

            AZStd::string nodesExcludedFromBounds;
            parameters.GetValue("nodesExcludedFromBounds", this, nodesExcludedFromBounds);

            AZStd::vector<AZStd::string> nodeNames;
            AzFramework::StringFunc::Tokenize(nodesExcludedFromBounds.c_str(), nodeNames, ";", false, true);

            // Remove the selected nodes from the bounding volume calculations.
            if (AzFramework::StringFunc::Equal(nodeAction.c_str(), "remove"))
            {
                for (const AZStd::string& nodeName : nodeNames)
                {
                    EMotionFX::Node* node = skeleton->FindNodeByName(nodeName);
                    if (!node)
                    {
                        continue;
                    }

                    node->SetIncludeInBoundsCalc(true);
                }
            }
            // Add the given nodes to the bounding volume calculations.
            if (AzFramework::StringFunc::Equal(nodeAction.c_str(), "add"))
            {
                for (const AZStd::string& nodeName : nodeNames)
                {
                    EMotionFX::Node* node = skeleton->FindNodeByName(nodeName);
                    if (!node)
                    {
                        continue;
                    }

                    node->SetIncludeInBoundsCalc(false);
                }
            }
            else
            {
                // Remove all nodes from bounding volume calculations.
                SetIsExcludedFromBoundsNode(actor, false);

                // Remove the nodes from bounding volume calculation based on the selection.
                for (const AZStd::string& nodeName : nodeNames)
                {
                    EMotionFX::Node* node = skeleton->FindNodeByName(nodeName);
                    if (!node)
                    {
                        continue;
                    }

                    node->SetIncludeInBoundsCalc(false);
                }
            }
        }

        // Adjust the mirror setup.
        if (parameters.CheckIfHasParameter("mirrorSetup"))
        {
            m_oldMirrorSetup = actor->GetNodeMirrorInfos();

            AZStd::string mirrorSetupString;
            parameters.GetValue("mirrorSetup", this, mirrorSetupString);

            if (mirrorSetupString.empty())
            {
                actor->RemoveNodeMirrorInfos();
            }
            else
            {
                // Allocate the node mirror info table.
                actor->AllocateNodeMirrorInfos();

                AZStd::vector<AZStd::string> pairs;
                AzFramework::StringFunc::Tokenize(mirrorSetupString.c_str(), pairs, ";", false, true);

                // Parse the mirror setup string, which is like "nodeA,nodeB;nodeC,nodeD;".
                for (const AZStd::string& pair : pairs)
                {
                    // Split the pairs into the node names.
                    AZStd::vector<AZStd::string> pairValues;
                    AzFramework::StringFunc::Tokenize(pair.c_str(), pairValues, ",", false, true);
                    if (pairValues.size() != 2)
                    {
                        continue;
                    }

                    EMotionFX::Node* nodeA = actor->GetSkeleton()->FindNodeByName(pairValues[0]);
                    EMotionFX::Node* nodeB = actor->GetSkeleton()->FindNodeByName(pairValues[1]);
                    if (nodeA && nodeB)
                    {
                        actor->GetNodeMirrorInfo(nodeA->GetNodeIndex()).m_sourceNode = static_cast<uint16>(nodeB->GetNodeIndex());
                        actor->GetNodeMirrorInfo(nodeB->GetNodeIndex()).m_sourceNode = static_cast<uint16>(nodeA->GetNodeIndex());
                    }
                }

                // Update the mirror axes.
                actor->AutoDetectMirrorAxes();
            }
        }

        m_oldDirtyFlag = actor->GetDirtyFlag();
        actor->SetDirtyFlag(true);
        return true;
    }


    bool CommandAdjustActor::Undo(const MCore::CommandLine& parameters, AZStd::string& outResult)
    {
        const uint32 actorID = parameters.GetValueAsInt("actorID", MCORE_INVALIDINDEX32);

        // Find the actor based on the given id.
        EMotionFX::Actor* actor = EMotionFX::GetActorManager().FindActorByID(actorID);
        if (!actor)
        {
            outResult = AZStd::string::format("Cannot adjust actor. Actor ID %i is not valid.", actorID);
            return false;
        }

        if (parameters.CheckIfHasParameter("motionExtractionNodeName"))
        {
            actor->SetMotionExtractionNodeIndex(m_oldMotionExtractionNodeIndex);
        }

        if (parameters.CheckIfHasParameter("retargetRootNodeName"))
        {
            actor->SetRetargetRootNodeIndex(m_oldRetargetRootNodeIndex);
        }

        if (parameters.CheckIfHasParameter("name"))
        {
            actor->SetName(m_oldName.c_str());
        }

        if (parameters.CheckIfHasParameter("mirrorSetup"))
        {
            actor->SetNodeMirrorInfos(m_oldMirrorSetup);
            actor->AutoDetectMirrorAxes();
        }

        // Set the attachment nodes.
        if (parameters.CheckIfHasParameter("attachmentNodes"))
        {
            const AZStd::string command = AZStd::string::format("AdjustActor -actorID %i -nodeAction \"select\" -attachmentNodes \"%s\"", actorID, m_oldAttachmentNodes.c_str());

            if (!GetCommandManager()->ExecuteCommandInsideCommand(command, outResult))
            {
                AZ_Error("EMotionFX", false, outResult.c_str());
                return false;
            }
        }

        // Set the nodes that are not taken into account in the bounding volume calculations.
        if (parameters.CheckIfHasParameter("nodesExcludedFromBounds"))
        {
            const AZStd::string command = AZStd::string::format("AdjustActor -actorID %i -nodeAction \"select\" -nodesExcludedFromBounds \"%s\"", actorID, m_oldExcludedFromBoundsNodes.c_str());

            if (!GetCommandManager()->ExecuteCommandInsideCommand(command, outResult))
            {
                AZ_Error("EMotionFX", false, outResult.c_str());
                return false;
            }
        }

        // Set the dirty flag back to the old value.
        actor->SetDirtyFlag(m_oldDirtyFlag);
        return true;
    }


    void CommandAdjustActor::InitSyntax()
    {
        GetSyntax().ReserveParameters(7);
        GetSyntax().AddRequiredParameter("actorID",            "The actor identification number of the actor to work on.", MCore::CommandSyntax::PARAMTYPE_INT);
        GetSyntax().AddParameter("motionExtractionNodeName",   "The node from which to transfer a filtered part of the motion onto the actor instance.", MCore::CommandSyntax::PARAMTYPE_STRING, "");
        GetSyntax().AddParameter("retargetRootNodeName",       "The node that controls vertical movement of the character, most likely the hip or pelvis.", MCore::CommandSyntax::PARAMTYPE_STRING, "");
        GetSyntax().AddParameter("attachmentNodes",            "A list of nodes that should be set to attachment nodes.", MCore::CommandSyntax::PARAMTYPE_STRING, "");
        GetSyntax().AddParameter("nodesExcludedFromBounds",    "A list of nodes that are excluded from all bounding volume calculations.", MCore::CommandSyntax::PARAMTYPE_STRING, "");
        GetSyntax().AddParameter("name",                       "The name of the actor.", MCore::CommandSyntax::PARAMTYPE_STRING, "");
        GetSyntax().AddParameter("nodeAction",                 "The action to perform with the nodes passed to the command.", MCore::CommandSyntax::PARAMTYPE_STRING, "select");
        GetSyntax().AddParameter("mirrorSetup",                "The list of mirror pairs in form of \"leftFoot,rightFoot;leftArm,rightArm;\". Or an empty string to clear the mirror table", MCore::CommandSyntax::PARAMTYPE_STRING, "");
    }


    const char* CommandAdjustActor::GetDescription() const
    {
        return "This command can be used to adjust the attributes of the given actor.";
    }


    // Static function to set all IsAttachmentNode flags of the actor to the given value.
    void CommandAdjustActor::SetIsAttachmentNode(EMotionFX::Actor* actor, bool isAttachmentNode)
    {
        const size_t numNodes = actor->GetNumNodes();
        for (size_t i = 0; i < numNodes; ++i)
        {
            EMotionFX::Node* node = actor->GetSkeleton()->GetNode(i);
            if (!node)
            {
                continue;
            }

            node->SetIsAttachmentNode(isAttachmentNode);
        }
    }


    // Static function to set all IsAttachmentNode flags of the actor to the given value.
    void CommandAdjustActor::SetIsExcludedFromBoundsNode(EMotionFX::Actor* actor, bool excludedFromBounds)
    {
        const size_t numNodes = actor->GetNumNodes();
        for (size_t i = 0; i < numNodes; ++i)
        {
            EMotionFX::Node* node = actor->GetSkeleton()->GetNode(i);
            if (!node)
            {
                continue;
            }

            node->SetIncludeInBoundsCalc(!excludedFromBounds);
        }
    }


    //--------------------------------------------------------------------------------
    // CommandActorSetCollisionMeshes
    //--------------------------------------------------------------------------------
    CommandActorSetCollisionMeshes::CommandActorSetCollisionMeshes(MCore::Command* orgCommand)
        : MCore::Command("ActorSetCollisionMeshes", orgCommand)
    {
    }


    CommandActorSetCollisionMeshes::~CommandActorSetCollisionMeshes()
    {
    }


    bool CommandActorSetCollisionMeshes::Execute(const MCore::CommandLine& parameters, AZStd::string& outResult)
    {
        const uint32 actorID = parameters.GetValueAsInt("actorID", MCORE_INVALIDINDEX32);

        // Find the actor based on the given id.
        EMotionFX::Actor* actor = EMotionFX::GetActorManager().FindActorByID(actorID);
        if (!actor)
        {
            outResult = AZStd::string::format("Cannot set collision meshes. Actor ID %i is not valid.", actorID);
            return false;
        }

        // Get the LOD level and check if it is valid.
        const uint32 lod = parameters.GetValueAsInt("lod", MCORE_INVALIDINDEX32);
        if (lod > (actor->GetNumLODLevels() - 1))
        {
            outResult = AZStd::string::format("Cannot set collision meshes. LOD %i is not valid.", lod);
            return false;
        }

        const size_t numNodes = actor->GetNumNodes();
        EMotionFX::Skeleton* skeleton = actor->GetSkeleton();

        // Store the old nodes for the undo.
        m_oldNodeList = "";
        for (size_t i = 0; i < numNodes; ++i)
        {
            EMotionFX::Mesh* mesh = actor->GetMesh(lod, i);
            if (mesh && mesh->GetIsCollisionMesh())
            {
                if (!m_oldNodeList.empty())
                {
                    m_oldNodeList += ";";
                }

                m_oldNodeList += skeleton->GetNode(i)->GetName();
            }
        }

        // Get the list of nodes that shall act as collision meshes.
        AZStd::string nodeList;
        parameters.GetValue("nodeList", this, nodeList);

        // Split the string containing the list of nodes.
        AZStd::vector<AZStd::string> nodeNames;
        AzFramework::StringFunc::Tokenize(nodeList.c_str(), nodeNames, ";", false, true);

        // Update the collision mesh flags.
        for (size_t i = 0; i < numNodes; ++i)
        {
            const EMotionFX::Node* node = skeleton->GetNode(i);
            EMotionFX::Mesh* mesh = actor->GetMesh(lod, i);
            if (!node || !mesh)
            {
                continue;
            }

            const bool isCollisionMesh = AZStd::find(nodeNames.begin(), nodeNames.end(), node->GetName()) != nodeNames.end();
            mesh->SetIsCollisionMesh(isCollisionMesh);
        }

        // Save the current dirty flag and tell the actor that something changed.
        m_oldDirtyFlag = actor->GetDirtyFlag();
        actor->SetDirtyFlag(true);

        // Reinit the renderable actors.
        AZStd::string reinitResult;
        GetCommandManager()->ExecuteCommandInsideCommand("ReInitRenderActors -resetViewCloseup false", reinitResult);
        return true;
    }


    bool CommandActorSetCollisionMeshes::Undo(const MCore::CommandLine& parameters, AZStd::string& outResult)
    {
        const uint32 actorID = parameters.GetValueAsInt("actorID", MCORE_INVALIDINDEX32);

        // Find the actor based on the given id.
        EMotionFX::Actor* actor = EMotionFX::GetActorManager().FindActorByID(actorID);
        if (!actor)
        {
            outResult = AZStd::string::format("Cannot set collision meshes. Actor ID %i is not valid.", actorID);
            return false;
        }

        const uint32 lod = parameters.GetValueAsInt("lod", MCORE_INVALIDINDEX32);

        // Undo command.
        const AZStd::string command = AZStd::string::format("ActorSetCollisionMeshes -actorID %i -lod %i -nodeList %s", actorID, lod, m_oldNodeList.c_str());
        GetCommandManager()->ExecuteCommandInsideCommand(command, outResult);

        // Set the dirty flag back to the old value
        actor->SetDirtyFlag(m_oldDirtyFlag);
        return true;
    }


    void CommandActorSetCollisionMeshes::InitSyntax()
    {
        GetSyntax().ReserveParameters(3);
        GetSyntax().AddParameter("actorID",     "The identification number of the actor we want to adjust.",    MCore::CommandSyntax::PARAMTYPE_INT,    "-1");
        GetSyntax().AddParameter("lod",         "The lod of the actor we want to adjust.",                      MCore::CommandSyntax::PARAMTYPE_INT,    "0");
        GetSyntax().AddParameter("nodeList",    "The node list.",                                               MCore::CommandSyntax::PARAMTYPE_STRING, "");
    }


    const char* CommandActorSetCollisionMeshes::GetDescription() const
    {
        return "This command can be used to set the collision meshes of the given actor.";
    }


    //--------------------------------------------------------------------------------
    // CommandResetToBindPose
    //--------------------------------------------------------------------------------
    bool CommandResetToBindPose::Execute(const MCore::CommandLine& parameters, AZStd::string& outResult)
    {
        MCORE_UNUSED(parameters);

        const size_t numSelectedActorInstances = GetCommandManager()->GetCurrentSelection().GetNumSelectedActorInstances();
        if (numSelectedActorInstances == 0)
        {
            outResult = "Cannot reset actor instances to bind pose. No actor instance selected.";
            return false;
        }

        // Iterate through all selected actor instances and reset them to bind pose.
        for (size_t i = 0; i < numSelectedActorInstances; ++i)
        {
            EMotionFX::ActorInstance* actorInstance = GetCommandManager()->GetCurrentSelection().GetActorInstance(i);

            actorInstance->GetTransformData()->ResetToBindPoseTransformations();
            actorInstance->SetLocalSpacePosition(AZ::Vector3::CreateZero());
            actorInstance->SetLocalSpaceRotation(AZ::Quaternion::CreateIdentity());

            EMFX_SCALECODE
            (
                actorInstance->SetLocalSpaceScale(AZ::Vector3::CreateOne());
            )
        }

        return true;
    }


    bool CommandResetToBindPose::Undo(const MCore::CommandLine& parameters, AZStd::string& outResult)
    {
        MCORE_UNUSED(parameters);
        MCORE_UNUSED(outResult);
        return true;
    }


    void CommandResetToBindPose::InitSyntax()
    {
    }


    const char* CommandResetToBindPose::GetDescription() const
    {
        return "This command can be used to reset the actor instance back to the bind pose.";
    }


    //--------------------------------------------------------------------------------
    // CommandReInitRenderActors
    //--------------------------------------------------------------------------------
    bool CommandReInitRenderActors::Execute(const MCore::CommandLine& parameters, AZStd::string& outResult)
    {
        MCORE_UNUSED(parameters);
        MCORE_UNUSED(outResult);
        return true;
    }


    bool CommandReInitRenderActors::Undo(const MCore::CommandLine& parameters, AZStd::string& outResult)
    {
        MCORE_UNUSED(parameters);
        MCORE_UNUSED(outResult);
        return true;
    }


    void CommandReInitRenderActors::InitSyntax()
    {
        GetSyntax().ReserveParameters(1);
        GetSyntax().AddParameter("resetViewCloseup", "", MCore::CommandSyntax::PARAMTYPE_BOOLEAN, "true");
    }


    const char* CommandReInitRenderActors::GetDescription() const
    {
        return "This command will be automatically called by the system in case all render actors need to get removed and reconstructed completely.";
    }


    //--------------------------------------------------------------------------------
    // CommandUpdateRenderActors
    //--------------------------------------------------------------------------------
    bool CommandUpdateRenderActors::Execute(const MCore::CommandLine& parameters, AZStd::string& outResult)
    {
        MCORE_UNUSED(parameters);
        MCORE_UNUSED(outResult);
        return true;
    }


    bool CommandUpdateRenderActors::Undo(const MCore::CommandLine& parameters, AZStd::string& outResult)
    {
        MCORE_UNUSED(parameters);
        MCORE_UNUSED(outResult);
        return true;
    }


    void CommandUpdateRenderActors::InitSyntax()
    {
    }


    const char* CommandUpdateRenderActors::GetDescription() const
    {
        return "This command will be automatically called by the system in case an actor got removed and we have to remove a render actor or in case there is a new actor we need to create a render actor for, all current render actors won't get touched.";
    }


    //--------------------------------------------------------------------------------
    // CommandRemoveActor
    //--------------------------------------------------------------------------------
    CommandRemoveActor::CommandRemoveActor(MCore::Command* orgCommand)
        : MCore::Command("RemoveActor", orgCommand)
    {
        m_previouslyUsedId = MCORE_INVALIDINDEX32;
    }


    CommandRemoveActor::~CommandRemoveActor()
    {
    }


    bool CommandRemoveActor::Execute(const MCore::CommandLine& parameters, AZStd::string& outResult)
    {
        EMotionFX::Actor* actor;
        if (parameters.CheckIfHasParameter("actorID"))
        {
            uint32 actorID = parameters.GetValueAsInt("actorID", MCORE_INVALIDINDEX32);

            // find the actor based on the given id
            actor = EMotionFX::GetActorManager().FindActorByID(actorID);
            if (!actor)
            {
                outResult = AZStd::string::format("Cannot create actor instance. Actor ID %i is not valid.", actorID);
                return false;
            }
        }
        else
        {
            // check if there is any actor selected at all
            SelectionList& selection = GetCommandManager()->GetCurrentSelection();
            if (selection.GetNumSelectedActors() == 0)
            {
                outResult = "No actor has been selected, please select one first.";
                return false;
            }

            // get the first selected actor
            actor = selection.GetActor(0);
        }

        // store the previously used id and the actor filename
        m_previouslyUsedId   = actor->GetID();
        m_oldFileName        = actor->GetFileName();
        m_oldDirtyFlag       = actor->GetDirtyFlag();
        m_oldWorkspaceDirtyFlag = GetCommandManager()->GetWorkspaceDirtyFlag();

        // get rid of the actor
        const AZ::Data::AssetId actorAssetId = EMotionFX::GetActorManager().FindAssetIdByActorId(actor->GetID());
        EMotionFX::GetActorManager().UnregisterActor(actorAssetId);

        // mark the workspace as dirty
        GetCommandManager()->SetWorkspaceDirtyFlag(true);

        // update our render actors
        AZStd::string updateRenderActorsResult;
        GetCommandManager()->ExecuteCommandInsideCommand("UpdateRenderActors", updateRenderActorsResult);
        return true;
    }


    bool CommandRemoveActor::Undo(const MCore::CommandLine& parameters, AZStd::string& outResult)
    {
        MCORE_UNUSED(parameters);

        const AZStd::string command = AZStd::string::format("ImportActor -filename \"%s\" -actorID %i", m_oldFileName.c_str(), m_previouslyUsedId);
        if (!GetCommandManager()->ExecuteCommandInsideCommand(command, outResult))
        {
            return false;
        }

        // Update our render actors.
        AZStd::string updateRenderActorsResult;
        if (!GetCommandManager()->ExecuteCommandInsideCommand("UpdateRenderActors", updateRenderActorsResult))
        {
            return false;
        }

        // restore the workspace dirty flag
        GetCommandManager()->SetWorkspaceDirtyFlag(m_oldWorkspaceDirtyFlag);

        return true;
    }


    void CommandRemoveActor::InitSyntax()
    {
        GetSyntax().ReserveParameters(1);
        GetSyntax().AddRequiredParameter("actorID", "The identification number of the actor we want to remove.", MCore::CommandSyntax::PARAMTYPE_INT);
    }


    const char* CommandRemoveActor::GetDescription() const
    {
        return "This command can be used to destruct an actor and all the corresponding actor instances.";
    }

    //-------------------------------------------------------------------------------------
    // Helper Functions
    //-------------------------------------------------------------------------------------
    void ClearScene(bool deleteActors, bool deleteActorInstances, MCore::CommandGroup* commandGroup)
    {
        // return if actorinstance was not set
        if (deleteActors == false && deleteActorInstances == false)
        {
            return;
        }

        // get number of actors and instances
        const size_t numActors          = EMotionFX::GetActorManager().GetNumActors();
        const size_t numActorInstances  = EMotionFX::GetActorManager().GetNumActorInstances();

        // create the command group
        MCore::CommandGroup internalCommandGroup("Clear scene");

        if (!commandGroup)
        {
            internalCommandGroup.AddCommandString("RecorderClear");
        }
        else
        {
            commandGroup->AddCommandString("RecorderClear");
        }

        // get rid of all actor instances in the scene
        if (deleteActors || deleteActorInstances)
        {
            // get rid of all actor instances
            for (size_t i = 0; i < numActorInstances; ++i)
            {
                // get pointer to the current actor instance
                EMotionFX::ActorInstance* actorInstance = EMotionFX::GetActorManager().GetActorInstance(i);

                // ignore runtime-owned actors
                if (actorInstance->GetIsOwnedByRuntime())
                {
                    continue;
                }
                // ignore visualization actor instances
                if (actorInstance->GetIsUsedForVisualization())
                {
                    continue;
                }
                // Ignore actor instances owned by entity
                if (actorInstance->GetEntity())
                {
                    continue;
                }

                // generate command to remove the actor instance
                const AZStd::string command = AZStd::string::format("RemoveActorInstance -actorInstanceID %i", actorInstance->GetID());

                // add the command to the command group
                if (!commandGroup)
                {
                    internalCommandGroup.AddCommandString(command);
                }
                else
                {
                    commandGroup->AddCommandString(command);
                }
            }
        }

        // get rid of all actors in the scene
        if (deleteActors)
        {
            // iterate through all available actors
            for (size_t i = 0; i < numActors; ++i)
            {
                // get the current actor
                EMotionFX::Actor* actor = EMotionFX::GetActorManager().GetActor(i);

                // ignore visualization actors
                if (actor->GetIsUsedForVisualization())
                {
                    continue;
                }

                // create the command to remove the actor
                const AZStd::string command = AZStd::string::format("RemoveActor -actorID %i", actor->GetID());

                // add the command to the command group
                if (!commandGroup)
                {
                    internalCommandGroup.AddCommandString(command);
                }
                else
                {
                    commandGroup->AddCommandString(command);
                }
            }
        }

        // clear the existing selection
        const AZStd::string command = AZStd::string::format("Unselect -actorID SELECT_ALL -actorInstanceID SELECT_ALL");
        if (commandGroup == nullptr)
        {
            internalCommandGroup.AddCommandString(command);
        }
        else
        {
            commandGroup->AddCommandString(command);
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


    // walk over the meshes and check which of them we want to set as collision mesh
    void PrepareCollisionMeshesNodesString(EMotionFX::Actor* actor, size_t lod, AZStd::string* outNodeNames)
    {
        // reset the resulting string
        outNodeNames->clear();
        outNodeNames->reserve(16384);

        // check if the actor is invalid
        if (actor == nullptr)
        {
            return;
        }

        // check if the lod is invalid
        if (lod > (actor->GetNumLODLevels() - 1))
        {
            return;
        }

        // get the number of nodes and iterate through them
        const size_t numNodes = actor->GetNumNodes();
        for (size_t i = 0; i < numNodes; ++i)
        {
            EMotionFX::Mesh* mesh = actor->GetMesh(lod, i);
            if (mesh && mesh->GetIsCollisionMesh())
            {
                *outNodeNames += AZStd::string::format("%s;", actor->GetSkeleton()->GetNode(i)->GetName());
            }
        }

        // make sure there is no semicolon at the end
        AzFramework::StringFunc::Strip(*outNodeNames, MCore::CharacterConstants::semiColon, true /* case sensitive */, false /* beginning */, true /* ending */);
    }


    // walk over the actor nodes and check which of them we want to exclude from the bounding volume calculations
    void PrepareExcludedNodesString(EMotionFX::Actor* actor, AZStd::string* outNodeNames)
    {
        // reset the resulting string
        outNodeNames->clear();
        outNodeNames->reserve(16384);

        // check if the actor is valid
        if (actor == nullptr)
        {
            return;
        }

        // get the number of nodes and iterate through them
        const size_t numNodes = actor->GetNumNodes();
        for (size_t i = 0; i < numNodes; ++i)
        {
            EMotionFX::Node* node = actor->GetSkeleton()->GetNode(i);

            if (node->GetIncludeInBoundsCalc() == false)
            {
                *outNodeNames += node->GetName();
                *outNodeNames += ";";
            }
        }

        // make sure there is no semicolon at the end
        AzFramework::StringFunc::Strip(*outNodeNames, MCore::CharacterConstants::semiColon, true /* case sensitive */, false /* beginning */, true /* ending */);
    }


    //--------------------------------------------------------------------------------
    // CommandScaleActorData
    //--------------------------------------------------------------------------------
    CommandScaleActorData::CommandScaleActorData(MCore::Command* orgCommand)
        : MCore::Command("ScaleActorData", orgCommand)
    {
        m_actorId            = MCORE_INVALIDINDEX32;
        m_scaleFactor        = 1.0f;
        m_oldActorDirtyFlag  = false;
        m_useUnitType        = false;
    }


    CommandScaleActorData::~CommandScaleActorData()
    {
    }


    bool CommandScaleActorData::Execute(const MCore::CommandLine& parameters, AZStd::string& outResult)
    {
        EMotionFX::Actor* actor;
        if (parameters.CheckIfHasParameter("id"))
        {
            uint32 actorID = parameters.GetValueAsInt("id", MCORE_INVALIDINDEX32);

            // find the actor based on the given id
            actor = EMotionFX::GetActorManager().FindActorByID(actorID);
            if (!actor)
            {
                outResult = AZStd::string::format("Cannot find actor with ID %d.", actorID);
                return false;
            }
        }
        else
        {
            // check if there is any actor selected at all
            SelectionList& selection = GetCommandManager()->GetCurrentSelection();
            if (selection.GetNumSelectedActors() == 0)
            {
                outResult = "No actor has been selected, please select one first.";
                return false;
            }

            // get the first selected actor
            actor = selection.GetActor(0);
        }

        if (parameters.CheckIfHasParameter("unitType") == false && parameters.CheckIfHasParameter("scaleFactor") == false)
        {
            outResult = "You have to either specify -unitType or -scaleFactor.";
            return false;
        }

        m_actorId = actor->GetID();
        m_scaleFactor = parameters.GetValueAsFloat("scaleFactor", this);

        AZStd::string targetUnitTypeString;
        parameters.GetValue("unitType", this, &targetUnitTypeString);

        m_useUnitType = parameters.CheckIfHasParameter("unitType");

        MCore::Distance::EUnitType targetUnitType;
        bool stringConvertSuccess = MCore::Distance::StringToUnitType(targetUnitTypeString, &targetUnitType);
        if (m_useUnitType && stringConvertSuccess == false)
        {
            outResult = AZStd::string::format("The passed unitType '%s' is not a valid unit type.", targetUnitTypeString.c_str());
            return false;
        }

        MCore::Distance::EUnitType beforeUnitType = actor->GetUnitType();
        m_oldUnitType = MCore::Distance::UnitTypeToString(beforeUnitType);

        m_oldActorDirtyFlag = actor->GetDirtyFlag();
        actor->SetDirtyFlag(true);

        // perform the scaling
        if (m_useUnitType == false)
        {
            actor->Scale(m_scaleFactor);
        }
        else
        {
            actor->ScaleToUnitType(targetUnitType);
        }

        // update the static aabb's of all actor instances
        const size_t numActorInstances = EMotionFX::GetActorManager().GetNumActorInstances();
        for (size_t i = 0; i < numActorInstances; ++i)
        {
            EMotionFX::ActorInstance* actorInstance = EMotionFX::GetActorManager().GetActorInstance(i);
            if (actorInstance->GetActor() != actor)
            {
                continue;
            }

            actorInstance->SetStaticBasedAabb(actor->GetStaticAabb());  // this is needed as the CalcStaticBasedAabb uses the current AABB as starting point
            AZ::Aabb newAabb;
            actorInstance->CalcStaticBasedAabb(&newAabb);
            actorInstance->SetStaticBasedAabb(newAabb);

            const float factor = (float)MCore::Distance::GetConversionFactor(beforeUnitType, targetUnitType);
            actorInstance->SetVisualizeScale(actorInstance->GetVisualizeScale() * factor);
        }

        // reinit the renderable actors
        AZStd::string reinitResult;
        GetCommandManager()->ExecuteCommandInsideCommand("ReInitRenderActors -resetViewCloseup false", reinitResult);

        return true;
    }


    bool CommandScaleActorData::Undo(const MCore::CommandLine& parameters, AZStd::string& outResult)
    {
        MCORE_UNUSED(parameters);

        if (!m_useUnitType)
        {
            const AZStd::string command = AZStd::string::format("ScaleActorData -id %d -scaleFactor %.8f", m_actorId, 1.0f / m_scaleFactor);
            GetCommandManager()->ExecuteCommandInsideCommand(command, outResult);
        }
        else
        {
            const AZStd::string command = AZStd::string::format("ScaleActorData -id %d -unitType \"%s\"", m_actorId, m_oldUnitType.c_str());
            GetCommandManager()->ExecuteCommandInsideCommand(command, outResult);
        }

        EMotionFX::Actor* actor = EMotionFX::GetActorManager().FindActorByID(m_actorId);
        if (actor)
        {
            actor->SetDirtyFlag(m_oldActorDirtyFlag);
        }

        return true;
    }


    void CommandScaleActorData::InitSyntax()
    {
        GetSyntax().ReserveParameters(3);
        GetSyntax().AddParameter("id",          "The identification number of the actor we want to scale.",             MCore::CommandSyntax::PARAMTYPE_INT,    "-1");
        GetSyntax().AddParameter("scaleFactor", "The scale factor, for example 10.0 to make the actor 10x as large.",   MCore::CommandSyntax::PARAMTYPE_FLOAT,  "1.0");
        GetSyntax().AddParameter("unitType",    "The unit type to convert to, for example 'meters'.",                   MCore::CommandSyntax::PARAMTYPE_STRING, "meters");
    }


    const char* CommandScaleActorData::GetDescription() const
    {
        return "This command can be used to scale all internal actor data. This includes vertex positions, morph targets, bounding volumes, bind pose transforms, etc.";
    }
} // namespace CommandSystem
