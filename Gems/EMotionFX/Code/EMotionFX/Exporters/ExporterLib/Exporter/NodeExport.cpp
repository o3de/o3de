/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "Exporter.h"

#include <AzCore/Serialization/Locale.h>

#include <EMotionFX/Source/Node.h>
#include <EMotionFX/Source/Actor.h>
#include <EMotionFX/Source/NodeGroup.h>
#include <EMotionFX/Source/Importer/ActorFileFormat.h>
#include <MCore/Source/LogManager.h>
#include <MCore/Source/StringConversions.h>
#include <MCore/Source/AzCoreConversions.h>


namespace ExporterLib
{
    void SaveNode(MCore::Stream* file, EMotionFX::Actor* actor, EMotionFX::Node* node, MCore::Endian::EEndianType targetEndianType)
    {
        AZ::Locale::ScopedSerializationLocale scopedLocale; // Ensures that %f uses "." as decimal separator

        MCORE_ASSERT(file);
        MCORE_ASSERT(actor);
        MCORE_ASSERT(node);

        // get some information from the node
        const size_t                nodeIndex           = node->GetNodeIndex();
        const size_t                parentIndex         = node->GetParentIndex();
        const size_t                numChilds           = node->GetNumChildNodes();
        const EMotionFX::Transform& transform           = actor->GetBindPose()->GetLocalSpaceTransform(nodeIndex);
        AZ::PackedVector3f          position            = AZ::PackedVector3f(transform.m_position);
        AZ::Quaternion              rotation            = transform.m_rotation.GetNormalized();

        #ifndef EMFX_SCALE_DISABLED
            AZ::PackedVector3f scale = AZ::PackedVector3f(transform.m_scale);
        #else
            AZ::PackedVector3f scale(1.0f, 1.0f, 1.0f);
        #endif

        // create the node chunk and copy over the information
        EMotionFX::FileFormat::Actor_Node2 nodeChunk;
        memset(&nodeChunk, 0, sizeof(EMotionFX::FileFormat::Actor_Node2));

        CopyVector(nodeChunk.m_localPos,    position);
        CopyQuaternion(nodeChunk.m_localQuat,   rotation);
        CopyVector(nodeChunk.m_localScale,  scale);

        nodeChunk.m_numChilds        = aznumeric_caster(numChilds);
        nodeChunk.m_parentIndex      = aznumeric_caster(parentIndex);

        // calculate and copy over the skeletal LODs
        uint32 skeletalLODs = 0;
        for (uint32 l = 0; l < 32; ++l)
        {
            if (node->GetSkeletalLODStatus(l))
            {
                skeletalLODs |= (1 << l);
            }
        }
        nodeChunk.m_skeletalLoDs = skeletalLODs;

        // will this node be involved in the bounding volume calculations?
        if (node->GetIncludeInBoundsCalc())
        {
            nodeChunk.m_nodeFlags |= EMotionFX::Node::ENodeFlags::FLAG_INCLUDEINBOUNDSCALC;// first bit
        }
        else
        {
            nodeChunk.m_nodeFlags  &= ~EMotionFX::Node::ENodeFlags::FLAG_INCLUDEINBOUNDSCALC;
        }

        // Add an isCritical option in node flag so it won't be optimized out. 
        if (node->GetIsCritical())
        {
            nodeChunk.m_nodeFlags |= EMotionFX::Node::ENodeFlags::FLAG_CRITICAL; // third bit
        }
        else
        {
            nodeChunk.m_nodeFlags &= ~EMotionFX::Node::ENodeFlags::FLAG_CRITICAL;
        }

        // log the node chunk information
        MCore::LogDetailedInfo("- Node: name='%s' index=%i", actor->GetSkeleton()->GetNode(nodeIndex)->GetName(), nodeIndex);
        if (parentIndex == InvalidIndex)
        {
            MCore::LogDetailedInfo("    + Parent: Has no parent(root).");
        }
        else
        {
            MCore::LogDetailedInfo("    + Parent: name='%s' index=%i", actor->GetSkeleton()->GetNode(parentIndex)->GetName(), parentIndex);
        }
        MCore::LogDetailedInfo("    + NumChilds: %i", nodeChunk.m_numChilds);
        MCore::LogDetailedInfo("    + Position: x=%f y=%f z=%f", nodeChunk.m_localPos.m_x, nodeChunk.m_localPos.m_y, nodeChunk.m_localPos.m_z);
        MCore::LogDetailedInfo("    + Rotation: x=%f y=%f z=%f w=%f", nodeChunk.m_localQuat.m_x, nodeChunk.m_localQuat.m_y, nodeChunk.m_localQuat.m_z, nodeChunk.m_localQuat.m_w);
        const AZ::Vector3 euler = MCore::AzQuaternionToEulerAngles(rotation);
        MCore::LogDetailedInfo("    + Rotation Euler: x=%f y=%f z=%f",
            float(euler.GetX()) * 180.0 / MCore::Math::pi,
            float(euler.GetY()) * 180.0 / MCore::Math::pi,
            float(euler.GetZ()) * 180.0 / MCore::Math::pi);
        MCore::LogDetailedInfo("    + Scale: x=%f y=%f z=%f", nodeChunk.m_localScale.m_x, nodeChunk.m_localScale.m_y, nodeChunk.m_localScale.m_z);
        MCore::LogDetailedInfo("    + IncludeInBoundsCalc: %d", node->GetIncludeInBoundsCalc());

        // log skeletal lods
        AZStd::string lodString = "    + Skeletal LODs: ";
        for (uint32 l = 0; l < 32; ++l)
        {
            int32 flag = node->GetSkeletalLODStatus(l);
            lodString += AZStd::to_string(flag);
        }
        MCore::LogDetailedInfo(lodString.c_str());

        // endian conversion
        ConvertFileVector3(&nodeChunk.m_localPos,           targetEndianType);
        ConvertFileQuaternion(&nodeChunk.m_localQuat,       targetEndianType);
        ConvertFileVector3(&nodeChunk.m_localScale,         targetEndianType);
        ConvertUnsignedInt(&nodeChunk.m_parentIndex,        targetEndianType);
        ConvertUnsignedInt(&nodeChunk.m_numChilds,          targetEndianType);
        ConvertUnsignedInt(&nodeChunk.m_skeletalLoDs,       targetEndianType);

        // write it
        file->Write(&nodeChunk, sizeof(EMotionFX::FileFormat::Actor_Node2));

        // write the name of the node and parent
        SaveString(node->GetName(), file, targetEndianType);
    }

    void SaveNodes(MCore::Stream* file, EMotionFX::Actor* actor, MCore::Endian::EEndianType targetEndianType)
    {
        // get the number of nodes
        const size_t numNodes = actor->GetNumNodes();

        MCore::LogDetailedInfo("============================================================");
        MCore::LogInfo("Nodes (%i)", actor->GetNumNodes());
        MCore::LogDetailedInfo("============================================================");

        // chunk information
        EMotionFX::FileFormat::FileChunk chunkHeader;
        chunkHeader.m_chunkId = EMotionFX::FileFormat::ACTOR_CHUNK_NODES;
        chunkHeader.m_version = 2;

        // get the nodes chunk size
        chunkHeader.m_sizeInBytes = aznumeric_caster(sizeof(EMotionFX::FileFormat::Actor_Nodes2) + numNodes * sizeof(EMotionFX::FileFormat::Actor_Node2));
        for (size_t i = 0; i < numNodes; i++)
        {
            chunkHeader.m_sizeInBytes += GetStringChunkSize(actor->GetSkeleton()->GetNode(i)->GetName());
        }

        // endian conversion and write it
        ConvertFileChunk(&chunkHeader, targetEndianType);
        file->Write(&chunkHeader, sizeof(EMotionFX::FileFormat::FileChunk));

        // nodes chunk
        EMotionFX::FileFormat::Actor_Nodes2 nodesChunk;
        nodesChunk.m_numNodes        = aznumeric_caster(numNodes);
        nodesChunk.m_numRootNodes    = aznumeric_caster(actor->GetSkeleton()->GetNumRootNodes());

        // endian conversion and write it
        ConvertUnsignedInt(&nodesChunk.m_numNodes, targetEndianType);
        ConvertUnsignedInt(&nodesChunk.m_numRootNodes, targetEndianType);

        file->Write(&nodesChunk, sizeof(EMotionFX::FileFormat::Actor_Nodes2));

        // write the nodes
        for (size_t n = 0; n < numNodes; n++)
        {
            SaveNode(file, actor, actor->GetSkeleton()->GetNode(n), targetEndianType);
        }
    }


    void SaveNodeGroup(MCore::Stream* file, const EMotionFX::NodeGroup* nodeGroup, MCore::Endian::EEndianType targetEndianType)
    {
        MCORE_ASSERT(file);
        MCORE_ASSERT(nodeGroup);

        // get the number of nodes in the node group
        const size_t numNodes = nodeGroup->GetNumNodes();

        // the node group chunk
        EMotionFX::FileFormat::Actor_NodeGroup groupChunk;
        memset(&groupChunk, 0, sizeof(EMotionFX::FileFormat::Actor_NodeGroup));

        // set the data
        groupChunk.m_numNodes            = static_cast<uint16>(numNodes);
        groupChunk.m_disabledOnDefault   = nodeGroup->GetIsEnabledOnDefault() ? false : true;

        // logging
        MCore::LogDetailedInfo("- Group: name='%s'", nodeGroup->GetName());
        MCore::LogDetailedInfo("    + DisabledOnDefault: %i", groupChunk.m_disabledOnDefault);
        AZStd::string nodesString;
        for (size_t i = 0; i < numNodes; ++i)
        {
            nodesString += AZStd::to_string(nodeGroup->GetNode(static_cast<uint16>(i)));
            if (i < numNodes - 1)
            {
                nodesString += ", ";
            }
        }
        MCore::LogDetailedInfo("    + Nodes (%i): %s", groupChunk.m_numNodes, nodesString.c_str());

        // endian conversion
        ConvertUnsignedShort(&groupChunk.m_numNodes, targetEndianType);

        // write it
        file->Write(&groupChunk, sizeof(EMotionFX::FileFormat::Actor_NodeGroup));

        // write the name of the node group
        SaveString(nodeGroup->GetNameString(), file, targetEndianType);

        // write the node numbers
        for (size_t i = 0; i < numNodes; ++i)
        {
            uint16 nodeNumber = nodeGroup->GetNode(static_cast<uint16>(i));
            if (nodeNumber == MCORE_INVALIDINDEX16)
            {
                MCore::LogWarning("Node group corrupted. NodeNr #%i not valid.", i);
            }
            ConvertUnsignedShort(&nodeNumber, targetEndianType);
            file->Write(&nodeNumber, sizeof(uint16));
        }
    }


    void SaveNodeGroups(MCore::Stream* file, const AZStd::vector<EMotionFX::NodeGroup*>& nodeGroups, MCore::Endian::EEndianType targetEndianType)
    {
        MCORE_ASSERT(file);

        // get the number of node groups
        const size_t numGroups = nodeGroups.size();

        if (numGroups == 0)
        {
            return;
        }

        MCore::LogDetailedInfo("============================================================");
        MCore::LogInfo("NodeGroups (%i)", numGroups);
        MCore::LogDetailedInfo("============================================================");

        // chunk information
        EMotionFX::FileFormat::FileChunk chunkHeader;
        chunkHeader.m_chunkId = EMotionFX::FileFormat::ACTOR_CHUNK_NODEGROUPS;
        chunkHeader.m_version = 1;

        // calculate the chunk size
        chunkHeader.m_sizeInBytes = sizeof(uint16);
        for (const EMotionFX::NodeGroup* nodeGroup : nodeGroups)
        {
            chunkHeader.m_sizeInBytes += sizeof(EMotionFX::FileFormat::Actor_NodeGroup);
            chunkHeader.m_sizeInBytes += GetStringChunkSize(nodeGroup->GetNameString());
            chunkHeader.m_sizeInBytes += sizeof(uint16) * aznumeric_cast<uint32>(nodeGroup->GetNumNodes());
        }

        // endian conversion
        ConvertFileChunk(&chunkHeader, targetEndianType);

        // write the chunk header
        file->Write(&chunkHeader, sizeof(EMotionFX::FileFormat::FileChunk));

        // write the number of groups to follow
        uint16 numGroupsChunk = static_cast<uint16>(numGroups);
        ConvertUnsignedShort(&numGroupsChunk, targetEndianType);
        file->Write(&numGroupsChunk, sizeof(uint16));

        // iterate through all groups
        for (const EMotionFX::NodeGroup* nodeGroup : nodeGroups)
        {
            SaveNodeGroup(file, nodeGroup, targetEndianType);
        }
    }


    void SaveNodeGroups(MCore::Stream* file, EMotionFX::Actor* actor, MCore::Endian::EEndianType targetEndianType)
    {
        MCORE_ASSERT(file);
        MCORE_ASSERT(actor);

        // get the number of node groups
        const size_t numGroups = actor->GetNumNodeGroups();

        // create the node group array and reserve some elements
        AZStd::vector<EMotionFX::NodeGroup*> nodeGroups;
        nodeGroups.reserve(numGroups);

        // iterate through the node groups and add them to the array
        for (size_t i = 0; i < numGroups; ++i)
        {
            nodeGroups.emplace_back(actor->GetNodeGroup(i));
        }

        // save the node groups
        SaveNodeGroups(file, nodeGroups, targetEndianType);
    }


    void SaveNodeMotionSources(MCore::Stream* file, EMotionFX::Actor* actor, AZStd::vector<EMotionFX::Actor::NodeMirrorInfo>* nodeMirrorInfos, MCore::Endian::EEndianType targetEndianType)
    {
        MCORE_ASSERT(file);

        if (actor)
        {
            nodeMirrorInfos = &actor->GetNodeMirrorInfos();
        }

        MCORE_ASSERT(nodeMirrorInfos);

        const size_t numNodes = nodeMirrorInfos->size();

        // chunk information
        EMotionFX::FileFormat::FileChunk chunkHeader;
        chunkHeader.m_chunkId        = EMotionFX::FileFormat::ACTOR_CHUNK_NODEMOTIONSOURCES;
        chunkHeader.m_sizeInBytes    = aznumeric_caster(sizeof(EMotionFX::FileFormat::Actor_NodeMotionSources2) + (numNodes * sizeof(uint16)) + (numNodes * sizeof(uint8) * 2));
        chunkHeader.m_version        = 1;

        // endian conversion and write it
        ConvertFileChunk(&chunkHeader, targetEndianType);
        file->Write(&chunkHeader, sizeof(EMotionFX::FileFormat::FileChunk));


        // the node motion sources chunk data
        EMotionFX::FileFormat::Actor_NodeMotionSources2 nodeMotionSourcesChunk;
        nodeMotionSourcesChunk.m_numNodes = aznumeric_caster(numNodes);

        // convert endian and save to the file
        ConvertUnsignedInt(&nodeMotionSourcesChunk.m_numNodes, targetEndianType);
        file->Write(&nodeMotionSourcesChunk, sizeof(EMotionFX::FileFormat::Actor_NodeMotionSources2));


        // log details
        MCore::LogInfo("============================================================");
        MCore::LogInfo("- Node Motion Sources (%i):", numNodes);
        MCore::LogInfo("============================================================");

        // write all node motion sources and convert endian
        for (const EMotionFX::Actor::NodeMirrorInfo& nodeMirrorInfo : *nodeMirrorInfos)
        {
            // get the motion node source
            uint16 nodeMotionSource = nodeMirrorInfo.m_sourceNode;

            // convert endian and save to the file
            ConvertUnsignedShort(&nodeMotionSource, targetEndianType);
            file->Write(&nodeMotionSource, sizeof(uint16));
        }

        // write all axes
        for (const EMotionFX::Actor::NodeMirrorInfo& nodeMirrorInfo : *nodeMirrorInfos)
        {
            uint8 axis = static_cast<uint8>(nodeMirrorInfo.m_axis);
            file->Write(&axis, sizeof(uint8));
        }

        // write all flags
        for (const EMotionFX::Actor::NodeMirrorInfo& nodeMirrorInfo : *nodeMirrorInfos)
        {
            uint8 flags = static_cast<uint8>(nodeMirrorInfo.m_flags);
            file->Write(&flags, sizeof(uint8));
        }
    }


    void SaveAttachmentNodes(MCore::Stream* file, EMotionFX::Actor* actor, MCore::Endian::EEndianType targetEndianType)
    {
        // get the number of nodes
        const size_t numNodes = actor->GetNumNodes();

        // create our attachment nodes array and preallocate memory
        AZStd::vector<uint16> attachmentNodes;
        attachmentNodes.reserve(numNodes);

        // iterate through the nodes and collect all attachments
        for (size_t i = 0; i < numNodes; ++i)
        {
            // get the current node, check if it is an attachment and add it to the attachment array in that case
            EMotionFX::Node* node = actor->GetSkeleton()->GetNode(i);
            if (node->GetIsAttachmentNode())
            {
                attachmentNodes.push_back(static_cast<uint16>(node->GetNodeIndex()));
            }
        }

        // pass the data along
        SaveAttachmentNodes(file, actor, attachmentNodes, targetEndianType);
    }


    void SaveAttachmentNodes(MCore::Stream* file, EMotionFX::Actor* actor, const AZStd::vector<uint16>& attachmentNodes, MCore::Endian::EEndianType targetEndianType)
    {
        MCORE_ASSERT(file);

        if (attachmentNodes.empty())
        {
            return;
        }

        // get the number of attachment nodes
        const size_t numAttachmentNodes = attachmentNodes.size();

        // chunk information
        EMotionFX::FileFormat::FileChunk chunkHeader;
        chunkHeader.m_chunkId        = EMotionFX::FileFormat::ACTOR_CHUNK_ATTACHMENTNODES;
        chunkHeader.m_sizeInBytes    = aznumeric_caster(sizeof(EMotionFX::FileFormat::Actor_AttachmentNodes) + numAttachmentNodes * sizeof(uint16));
        chunkHeader.m_version        = 1;

        // endian conversion and write it
        ConvertFileChunk(&chunkHeader, targetEndianType);
        file->Write(&chunkHeader, sizeof(EMotionFX::FileFormat::FileChunk));


        // the attachment nodes chunk data
        EMotionFX::FileFormat::Actor_AttachmentNodes attachmentNodesChunk;
        attachmentNodesChunk.m_numNodes = aznumeric_caster(numAttachmentNodes);

        // convert endian and save to the file
        ConvertUnsignedInt(&attachmentNodesChunk.m_numNodes, targetEndianType);
        file->Write(&attachmentNodesChunk, sizeof(EMotionFX::FileFormat::Actor_AttachmentNodes));

        // log details
        MCore::LogInfo("============================================================");
        MCore::LogInfo("Attachment Nodes (%i):", numAttachmentNodes);
        MCore::LogInfo("============================================================");

        // get all nodes that are affected by the skin
        AZStd::vector<size_t> bones;
        if (actor)
        {
            actor->ExtractBoneList(0, &bones);
        }

        // write all attachment nodes and convert endian
        for (uint16 nodeNr : attachmentNodes)
        {
            // get the attachment node index
            if (actor && nodeNr != MCORE_INVALIDINDEX16)
            {
                EMotionFX::Node* node = actor->GetSkeleton()->GetNode(nodeNr);

                MCore::LogInfo("   + '%s' (NodeNr=%i)", node->GetName(), nodeNr);

                // is the attachment really a leaf node?
                if (node->GetNumChildNodes() != 0)
                {
                    MCore::LogWarning("Attachment node '%s' (NodeNr=%i) has got child nodes. Attachment nodes should be leaf nodes and need to not have any children.", node->GetName(), nodeNr);
                }

                // is the attachment node a skinned one?
                if (AZStd::find(begin(bones), end(bones), node->GetNodeIndex()) != end(bones))
                {
                    MCore::LogWarning("Attachment node '%s' (NodeNr=%i) is used by a skin. Skinning will look incorrectly when using motion mirroring.", node->GetName(), nodeNr);
                }
            }

            // convert endian and save to the file
            ConvertUnsignedShort(&nodeNr, targetEndianType);
            file->Write(&nodeNr, sizeof(uint16));
        }
    }
} // namespace ExporterLib
