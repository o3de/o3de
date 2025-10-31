/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <EMotionFX/Source/Motion.h>
#include <EMotionFX/Source/Actor.h>
#include <EMotionFX/Source/MotionEventTrack.h>
#include <EMotionFX/Source/EventManager.h>
#include <EMotionFX/Source/NodeGroup.h>
#include <EMotionFX/Source/Motion.h>
#include <EMotionFX/Source/MorphSetup.h>
#include <EMotionFX/Source/MotionEventTable.h>
#include <EMotionFX/CommandSystem/Source/ActorCommands.h>
#include <EMotionFX/CommandSystem/Source/MetaData.h>
#include <EMotionFX/Exporters/ExporterLib/Exporter/Exporter.h>
#include <EMotionFX/CommandSystem/Source/MotionCommands.h>
#include <EMotionFX/CommandSystem/Source/MotionEventCommands.h>
#include <MCore/Source/LogManager.h>

#include <AzCore/Serialization/Locale.h>
#include <AzCore/Serialization/Utils.h>

namespace CommandSystem
{
    bool MetaData::ApplyMetaDataOnMotion(EMotionFX::Motion* motion, const AZStd::vector<MCore::Command*>& metaDataCommands)
    {
        for (MCore::Command* command : metaDataCommands)
        {
            if (MotionIdCommandMixin* motionIdCommand = azrtti_cast<MotionIdCommandMixin*>(command))
            {
                motionIdCommand->SetMotionID(motion->GetID());
            }
        }
        return ApplyMetaData(metaDataCommands);
    }


    void MetaData::GenerateNodeGroupMetaData(EMotionFX::Actor* actor, AZStd::string& outMetaDataString)
    {
        AZStd::string nodeNameList;
        const size_t numNodeGroups = actor->GetNumNodeGroups();
        for (size_t i = 0; i < numNodeGroups; ++i)
        {
            EMotionFX::NodeGroup* nodeGroup = actor->GetNodeGroup(i);
            outMetaDataString += AZStd::string::format("AddNodeGroup -actorID $(ACTORID) -name \"%s\"\n", nodeGroup->GetName());

            nodeNameList.clear();
            const size_t numNodes = nodeGroup->GetNumNodes();
            for (size_t n = 0; n < numNodes; ++n)
            {
                const AZ::u16    nodeIndex = nodeGroup->GetNode(n);
                EMotionFX::Node* node = actor->GetSkeleton()->GetNode(nodeIndex);

                if (n > 0)
                {
                    nodeNameList += ";";
                }

                nodeNameList += node->GetName();
            }

            const bool enabledOnDefault = nodeGroup->GetIsEnabledOnDefault();
            outMetaDataString += AZStd::string::format("AdjustNodeGroup -actorID $(ACTORID) -name \"%s\" -nodeAction \"add\" ", nodeGroup->GetName());
            outMetaDataString += AZStd::string::format("-nodeNames \"%s\" -enabledOnDefault \"%s\"\n", nodeNameList.c_str(), enabledOnDefault ? "true" : "false");
        }
    }


    void MetaData::GeneratePhonemeMetaData(EMotionFX::Actor* actor, AZStd::string& outMetaDataString)
    {
        AZ::Locale::ScopedSerializationLocale localeScope; // ensures that %f uses '.' as decimal separator

        const size_t numLODLevels = actor->GetNumLODLevels();
        for (size_t lodLevel = 0; lodLevel < numLODLevels; ++lodLevel)
        {
            EMotionFX::MorphSetup* morphSetup = actor->GetMorphSetup(lodLevel);
            if (!morphSetup)
            {
                continue;
            }

            const size_t numMorphTargets = morphSetup->GetNumMorphTargets();
            for (size_t i = 0; i < numMorphTargets; ++i)
            {
                EMotionFX::MorphTarget* morphTarget = morphSetup->GetMorphTarget(i);
                if (!morphTarget)
                {
                    continue;
                }

                outMetaDataString += AZStd::string::format("AdjustMorphTarget -actorID $(ACTORID) -lodLevel %zu -name \"%s\" -phonemeAction \"replace\" ", lodLevel, morphTarget->GetName());
                outMetaDataString += AZStd::string::format("-phonemeSets \"%s\" ", morphTarget->GetPhonemeSetString(morphTarget->GetPhonemeSets()).c_str());
                outMetaDataString += AZStd::string::format("-rangeMin %f -rangeMax %f\n", morphTarget->GetRangeMin(), morphTarget->GetRangeMax());
            }
        }
    }


    void MetaData::GenerateAttachmentMetaData(EMotionFX::Actor* actor, AZStd::string& outMetaDataString)
    {
        AZStd::string attachmentNodeNameList;

        const size_t numNodes = actor->GetNumNodes();
        for (size_t i = 0; i < numNodes; ++i)
        {
            EMotionFX::Node* node = actor->GetSkeleton()->GetNode(i);
            if (!node)
            {
                continue;
            }

            // Check if the node is an attachment node, if yes add it to our string node name list.
            if (node->GetIsAttachmentNode())
            {
                attachmentNodeNameList += node->GetName();
                attachmentNodeNameList += ";";
            }
        }

        outMetaDataString += AZStd::string::format("AdjustActor -actorID $(ACTORID) -nodeAction \"replace\" -attachmentNodes \"%s\"\n", attachmentNodeNameList.c_str());
    }


    void MetaData::GenerateMotionExtractionMetaData(EMotionFX::Actor* actor, AZStd::string& outMetaDataString)
    {
        EMotionFX::Node* motionExtractionNode = actor->GetMotionExtractionNode();
        if (motionExtractionNode)
        {
            outMetaDataString += "AdjustActor -actorID $(ACTORID) ";
            outMetaDataString += AZStd::string::format("-motionExtractionNodeName \"%s\"\n", motionExtractionNode->GetName());
        }
    }


    void MetaData::GenerateRetargetRootMetaData(EMotionFX::Actor* actor, AZStd::string& outMetaDataString)
    {
        EMotionFX::Node* retargetRootNode = actor->GetRetargetRootNode();
        if (retargetRootNode)
        {
            outMetaDataString += "AdjustActor -actorID $(ACTORID) ";
            outMetaDataString += AZStd::string::format("-retargetRootNodeName \"%s\"\n", retargetRootNode->GetName());
        }
    }


    void MetaData::GenerateMirrorSetupMetaData(EMotionFX::Actor* actor, AZStd::string& outMetaDataString)
    {
        if (actor->GetHasMirrorInfo())
        {
            outMetaDataString += "AdjustActor -actorID $(ACTORID) -mirrorSetup \"";

            for (uint32 i = 0; i < actor->GetNumNodes(); ++i)
            {
                const EMotionFX::Actor::NodeMirrorInfo& mirrorInfo = actor->GetNodeMirrorInfo(i);
                uint16 sourceNode = mirrorInfo.m_sourceNode;
                if (sourceNode != MCORE_INVALIDINDEX16 && sourceNode != static_cast<uint16>(i))
                {
                    outMetaDataString += actor->GetSkeleton()->GetNode(i)->GetNameString();
                    outMetaDataString += ",";
                    outMetaDataString += actor->GetSkeleton()->GetNode(sourceNode)->GetNameString();
                    outMetaDataString += ";";
                }
            }

            outMetaDataString += "\"";
            outMetaDataString += "\n";
        }
    }


    AZStd::string MetaData::GenerateActorMetaData(EMotionFX::Actor* actor)
    {
        if (!actor)
        {
            AZ_Error("EMotionFX", false, "Cannot generate meta data for actor. Actor invalid.");
            return "";
        }

        AZStd::string metaDataString;

        // actor name
        metaDataString += AZStd::string::format("AdjustActor -actorID $(ACTORID) -name \"%s\"\n", actor->GetName());

        // collision mesh for LOD 0
        AZStd::string tempString;
        PrepareCollisionMeshesNodesString(actor, 0, &tempString);
        metaDataString += AZStd::string::format("ActorSetCollisionMeshes -actorID $(ACTORID) -lod 0 -nodeList \"");
        metaDataString += tempString;
        metaDataString += "\"\n";

        // nodes excluded from the bounding volume calculation
        PrepareExcludedNodesString(actor, &tempString);
        metaDataString += "AdjustActor -actorID $(ACTORID) -nodesExcludedFromBounds \"";
        metaDataString += tempString;
        metaDataString += "\" -nodeAction \"select\"\n";

        GenerateNodeGroupMetaData(actor, metaDataString);
        GeneratePhonemeMetaData(actor, metaDataString);
        GenerateAttachmentMetaData(actor, metaDataString);
        GenerateMotionExtractionMetaData(actor, metaDataString);
        GenerateRetargetRootMetaData(actor, metaDataString);
        GenerateMirrorSetupMetaData(actor, metaDataString);

        return metaDataString;
    }


    bool MetaData::ApplyMetaDataOnActor(EMotionFX::Actor* actor, const AZStd::string& metaDataString)
    {
        return ApplyMetaData(actor->GetID(), "$(ACTORID)", metaDataString);
    }


    bool MetaData::ApplyMetaData(AZ::u32 objectId, const char* objectIdKeyword, AZStd::string metaDataString)
    {
        if (metaDataString.empty())
        {
            return true;
        }

        // Convert the runtime object id to a string.
        AZStd::string idString = AZStd::string::format("%i", objectId);

        // Replace all object id string occurrences (e.g. $(MOTIONID)) with the runtime object id (e.g. "54")
        AzFramework::StringFunc::Replace(metaDataString, objectIdKeyword, idString.c_str());

        // Tokenize by line delimiters.
        AZStd::vector<AZStd::string> tokens;
        AzFramework::StringFunc::Replace(metaDataString, "\r\n", "\n");
        AzFramework::StringFunc::Replace(metaDataString, '\r', '\n');
        AzFramework::StringFunc::Tokenize(metaDataString.c_str(), tokens, '\n');

        // Construct a new command group and fill it with all meta data commands.
        MCore::CommandGroup commandGroup;
        for (const AZStd::string& token : tokens)
        {
            commandGroup.AddCommandString(token);
        }

        // Execute the command group and apply the meta data.
        AZStd::string outResult;
        if (GetCommandManager()->ExecuteCommandGroup(commandGroup, outResult) == false)
        {
            if (outResult.empty() == false)
            {
                 MCore::LogError(outResult.c_str());
            }

            return false;
        }

        return true;
    }

    bool MetaData::ApplyMetaData(const AZStd::vector<MCore::Command*>& metaDataCommands)
    {
        AZStd::string outResult;
        for (MCore::Command* command : metaDataCommands)
        {
            if (!GetCommandManager()->ExecuteCommand(command,
                outResult,
                /*addToHistory=*/false,
                /*clearErrors=*/true,
                /*handleErrors=*/true,
                /*autoDeleteCommand=*/false))
            {
                if (!outResult.empty())
                {
                    MCore::LogError(outResult.c_str());
                }
                return false;
            }
        }
        return true;
    }
} // namespace CommandSystem
