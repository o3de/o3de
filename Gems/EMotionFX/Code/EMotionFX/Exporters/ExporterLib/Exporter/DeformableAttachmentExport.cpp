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

#include "Exporter.h"
#include <AzCore/Debug/Timer.h>
#include <EMotionFX/Source/Actor.h>
#include <EMotionFX/Source/Mesh.h>
#include <EMotionFX/Source/SubMesh.h>
#include <EMotionFX/Source/SkinningInfoVertexAttributeLayer.h>
#include <EMotionFX/Source/MorphTargetStandard.h>
#include <EMotionFX/Source/EventManager.h>
#include <EMotionFX/Source/MorphSetup.h>
#include <EMotionFX/Source/MeshDeformerStack.h>
#include <MCore/Source/FileSystem.h>
#include "ExporterFileProcessor.h"


namespace ExporterLib
{
    void RemapSubMeshAndSkin(EMotionFX::Actor* actor, EMotionFX::Mesh* mesh, const MCore::Array<uint32>& newNodeNumbers)
    {
        const uint32 numNodes = actor->GetNumNodes();
        MCORE_ASSERT(numNodes == newNodeNumbers.GetLength());

        // remap all bones in the submesh
        const uint32 numSubMeshes = mesh->GetNumSubMeshes();
        for (uint32 subMeshNr = 0; subMeshNr < numSubMeshes; ++subMeshNr)
        {
            EMotionFX::SubMesh* subMesh = mesh->GetSubMesh(subMeshNr);

            for (uint32 i = 0; i < numNodes; ++i)
            {
                const uint16    oldNodeIndex    = static_cast<uint16>(i);
                uint16          newNodeIndex    = MCORE_INVALIDINDEX16;

                if (newNodeNumbers[oldNodeIndex] != MCORE_INVALIDINDEX32)
                {
                    newNodeIndex = static_cast<uint16>(newNodeNumbers[oldNodeIndex]);
                }

                subMesh->RemapBone(oldNodeIndex, newNodeIndex);
            }
        }

        // remap all skin influences
        EMotionFX::SkinningInfoVertexAttributeLayer* meshSkinningInfo = (EMotionFX::SkinningInfoVertexAttributeLayer*)mesh->FindSharedVertexAttributeLayer(EMotionFX::SkinningInfoVertexAttributeLayer::TYPE_ID);
        if (meshSkinningInfo)
        {
            for (uint32 i = 0; i < numNodes; ++i)
            {
                const uint16    oldNodeIndex    = static_cast<uint16>(i);
                uint16          newNodeIndex    = MCORE_INVALIDINDEX16;

                if (newNodeNumbers[oldNodeIndex] != MCORE_INVALIDINDEX32)
                {
                    newNodeIndex = static_cast<uint16>(newNodeNumbers[oldNodeIndex]);
                }

                meshSkinningInfo->RemapInfluences(oldNodeIndex, newNodeIndex);
            }
        }
    }


    void RemoveInvalidNodesFromSubMeshAndSkin(EMotionFX::Actor* actor, EMotionFX::Mesh* mesh)
    {
        MCORE_UNUSED(actor);

        // iterate through the submeshes and remove the bone links for the given mesh
        const uint32 numSubMeshes = mesh->GetNumSubMeshes();
        for (uint32 subMeshNr = 0; subMeshNr < numSubMeshes; ++subMeshNr)
        {
            EMotionFX::SubMesh* subMesh = mesh->GetSubMesh(subMeshNr);

            // get the invalid bones remove it when it is in the bone list
            while (subMesh->FindBoneIndex(MCORE_INVALIDINDEX16) != MCORE_INVALIDINDEX32)
            {
                const uint32 invalidBoneIndex = subMesh->FindBoneIndex(MCORE_INVALIDINDEX16);
                subMesh->RemoveBone(static_cast<uint16>(invalidBoneIndex));
                //LogInfo( "Removing node from submesh." );
            }
        }

        // remove all skin influences from the mesh for the node which will be removed from the actor
        EMotionFX::SkinningInfoVertexAttributeLayer* meshSkinningInfo = (EMotionFX::SkinningInfoVertexAttributeLayer*)mesh->FindSharedVertexAttributeLayer(EMotionFX::SkinningInfoVertexAttributeLayer::TYPE_ID);
        if (meshSkinningInfo)
        {
            meshSkinningInfo->RemoveAllInfluencesForNode(MCORE_INVALIDINDEX16);
        }
    }


    void PrepareDeformableAttachment(EMotionFX::Actor* actor, const MCore::Array<EMotionFX::Node*>& meshNodes)
    {
        uint32 i;

        // get the number of nodes (full skeleton, before attachment removal), mesh nodes and the number of LODs
        const uint32 numOldNodes    = actor->GetNumNodes();
        const uint32 numMeshNodes   = meshNodes.GetLength();
        const uint32 numGeomLODs    = actor->GetNumLODLevels();

        // remove all meshes from the nodes which are not in the given mesh nodes list
        for (i = 0; i < numOldNodes; ++i)
        {
            EMotionFX::Node*    node        = actor->GetSkeleton()->GetNode(i);
            uint32              nodeIndex   = node->GetNodeIndex();

            // iterate through all mesh nodes and check if the current node is in the list, if not remove all its meshes
            bool found = false;
            for (uint32 j = 0; j < numMeshNodes; ++j)
            {
                EMotionFX::Node*    meshNode        = meshNodes[j];
                uint32              meshNodeIndex   = meshNode->GetNodeIndex();

                if (nodeIndex == meshNodeIndex)
                {
                    // the current node is in the list of mesh nodes
                    found = true;
                    break;
                }
            }

            // when the current node is not in the list of mesh nodes remove all its meshes
            if (found == false)
            {
                for (uint32 lodLevel = 0; lodLevel < numGeomLODs; ++lodLevel)
                {
                    //actor->RemoveNodeCollisionMeshForLOD(lodLevel, nodeIndex);
                    actor->RemoveNodeMeshForLOD(lodLevel, nodeIndex);
                }
            }
        }

        MCore::Array<uint32> nodes;
        nodes.Reserve(numOldNodes);

        // collect all essential nodes
        for (i = 0; i < numMeshNodes; ++i)
        {
            // get a pointer to the node and its index
            EMotionFX::Node*    node        = meshNodes[i];
            uint32              nodeIndex   = node->GetNodeIndex();

            // add the mesh node itself
            if (nodes.Find(nodeIndex) == MCORE_INVALIDINDEX32)
            {
                nodes.Add(nodeIndex);
            }

            // iterate through all LOD levels
            for (uint32 lodLevel = 0; lodLevel < numGeomLODs; ++lodLevel)
            {
                EMotionFX::Mesh* mesh = actor->GetMesh(lodLevel, nodeIndex);
                if (mesh)
                {
                    EMotionFX::SkinningInfoVertexAttributeLayer* meshSkinningInfo = (EMotionFX::SkinningInfoVertexAttributeLayer*)mesh->FindSharedVertexAttributeLayer(EMotionFX::SkinningInfoVertexAttributeLayer::TYPE_ID);
                    if (meshSkinningInfo)
                    {
                        meshSkinningInfo->CollectInfluencedNodes(nodes, false);
                    }
                }

                /*          EMotionFX::Mesh* collisionMesh = actor->GetCollisionMesh( lodLevel, nodeIndex );
                            if (collisionMesh)
                            {
                                EMotionFX::SkinningInfoVertexAttributeLayer* collisionMeshSkinningInfo = (EMotionFX::SkinningInfoVertexAttributeLayer*)collisionMesh->FindSharedVertexAttributeLayer( EMotionFX::SkinningInfoVertexAttributeLayer::TYPE_ID );
                                if (collisionMeshSkinningInfo)
                                    collisionMeshSkinningInfo->CollectInfluencedNodes( nodes, false );
                            }*/
            }
        }

        // also collect all parents of all newly added skinning nodes
        for (i = 0; i < nodes.GetLength(); ++i)
        {
            EMotionFX::Node* node = actor->GetSkeleton()->GetNode(nodes[i]);
            node->RecursiveCollectParents(nodes, false);
        }

        // sort the node numbers
        nodes.Sort();

        // after all nodes have been collected, get the number of nodes
        const uint32 numNodes = nodes.GetLength();

        // get the node names based on the collected node numbers
        MCore::Array<AZStd::string> nodeNames;
        nodes.Reserve(numNodes);
        for (i = 0; i < numNodes; ++i)
        {
            nodeNames.Add(actor->GetSkeleton()->GetNode(nodes[i])->GetName());
        }

        // storage where we store the new node numbers for each of the nodes
        // if a node doesn't exist anymore it will become MCORE_INVALIDINDEX32
        MCore::Array<uint32> newNodeNumbers;
        newNodeNumbers.Resize(numOldNodes);

        // get the remapping node indices
        for (i = 0; i < numOldNodes; ++i)
        {
            uint32 newNodeNr = nodes.Find(i);
            newNodeNumbers[i] = newNodeNr;
        }

        // remap the motion extraction node
        const uint32 motionExtractionNodeIndex = actor->GetMotionExtractionNodeIndex();
        if (motionExtractionNodeIndex != MCORE_INVALIDINDEX32)
        {
            const uint32 newNodeIndex = newNodeNumbers[motionExtractionNodeIndex];
            actor->SetMotionExtractionNodeIndex(newNodeIndex);
        }

        // clone the transform data
        //TransformData transformBackup;
        //transformBackup.InitFromActor( actor );

        const uint32 numActorNodes = actor->GetNumNodes();
        MCore::Array<EMotionFX::Transform> transformBackup;
        transformBackup.Resize(numActorNodes);
        EMotionFX::Pose& bindPose = *actor->GetBindPose();
        for (i = 0; i < numActorNodes; ++i)
        {
            transformBackup[i] = bindPose.GetLocalSpaceTransform(i);
        }

        // remap the parent indices and the transform data
        for (i = 0; i < numOldNodes; ++i)
        {
            EMotionFX::Node* node           = actor->GetSkeleton()->GetNode(i);
            const uint32    oldNodeIndex    = node->GetNodeIndex();
            const uint32    newNodeIndex    = newNodeNumbers[oldNodeIndex];
            const uint32    oldParentIndex  = node->GetParentIndex();
            uint32          newParentIndex  = MCORE_INVALIDINDEX32;
            if (oldParentIndex != MCORE_INVALIDINDEX32)
            {
                newParentIndex  = newNodeNumbers[oldParentIndex];
            }

            //LogDebug("NodeName='%s', OldNodeNr=%i, NewNodeNr=%i, OldParentNr=%i, NewParentNr=%i", node->GetName(), oldNodeIndex, newNodeIndex, oldParentIndex, newParentIndex);

            // copy over the transform data from the new node nr
            if (newNodeIndex != MCORE_INVALIDINDEX32)
            {
                // copy over correct transform datas as the indices have changed
                //TransformData* transformData = actor->GetTransformData();
                bindPose.SetLocalSpaceTransform(newNodeIndex, transformBackup[oldNodeIndex]);

                /*          transformData->SetLocalPos( newNodeIndex, transformBackup.GetLocalPos( oldNodeIndex ) );
                            transformData->SetLocalRot( newNodeIndex, transformBackup.GetLocalRot( oldNodeIndex ) );
                            transformData->SetLocalScale( newNodeIndex, transformBackup.GetLocalScale( oldNodeIndex ) );
                            transformData->SetLocalScaleRot( newNodeIndex, transformBackup.GetLocalScaleRot( oldNodeIndex ) );
                            transformData->SetOrgPos( newNodeIndex, transformBackup.GetOrgPos( oldNodeIndex ) );
                            transformData->SetOrgRot( newNodeIndex, transformBackup.GetOrgRot( oldNodeIndex ) );
                            transformData->SetOrgScale( newNodeIndex, transformBackup.GetOrgScale( oldNodeIndex ) );
                            transformData->SetOrgScaleRot( newNodeIndex, transformBackup.GetOrgScaleRot( oldNodeIndex ) );*/

                // we made sure all parents are exported along with the nodes
                // remap the parent index
                node->SetParentIndex(newParentIndex);

                // iterate through all LOD levels
                for (uint32 lodLevel = 0; lodLevel < numGeomLODs; ++lodLevel)
                {
                    EMotionFX::Mesh* mesh = actor->GetMesh(lodLevel, node->GetNodeIndex());
                    if (mesh)
                    {
                        RemapSubMeshAndSkin(actor, mesh, newNodeNumbers);
                        RemoveInvalidNodesFromSubMeshAndSkin(actor, mesh);
                    }

                    /*              EMotionFX::Mesh* collisionMesh = actor->GetCollisionMesh( lodLevel, node->GetNodeIndex() );
                                    if (collisionMesh)
                                    {
                                        RemapSubMeshAndSkin( actor, collisionMesh, newNodeNumbers );
                                        RemoveInvalidNodesFromSubMeshAndSkin( actor, collisionMesh );
                                    }*/
                }
            }
        }


        // iterate through all LOD levels and remap corresponding morph targets
        for (uint32 lodLevel = 0; lodLevel < numGeomLODs; ++lodLevel)
        {
            EMotionFX::MorphSetup* morphSetup = actor->GetMorphSetup(lodLevel);
            if (morphSetup)
            {
                // relink morph targets
                const uint32 numMorphTargets = morphSetup->GetNumMorphTargets();
                for (i = 0; i < numMorphTargets; ++i)
                {
                    uint32 j;

                    // check if we are dealing with a standard morph target
                    if (morphSetup->GetMorphTarget(i)->GetType() != EMotionFX::MorphTargetStandard::TYPE_ID)
                    {
                        continue;
                    }

                    // down cast the morph target
                    EMotionFX::MorphTargetStandard* morphTarget = (EMotionFX::MorphTargetStandard*)morphSetup->GetMorphTarget(i);

                    j = 0;
                    while (j < morphTarget->GetNumDeformDatas())
                    {
                        const uint32 oldNodeIndex = morphTarget->GetDeformData(j)->mNodeIndex;
                        const uint32 newNodeIndex = newNodeNumbers[ oldNodeIndex ];

                        // if the new node index is invalid remove the given deform data, else remap the node index
                        if (newNodeIndex == MCORE_INVALIDINDEX32)
                        {
                            // remove the deform data as the given node is not present in the new actor
                            morphTarget->RemoveDeformData(j);
                        }
                        else
                        {
                            // remap the node indices of the deform data
                            morphTarget->GetDeformData(j)->mNodeIndex = newNodeIndex;
                            j++;
                        }
                    }

                    j = 0;
                    while (j < morphTarget->GetNumTransformations())
                    {
                        const uint32 oldNodeIndex = morphTarget->GetTransformation(j).mNodeIndex;
                        const uint32 newNodeIndex = newNodeNumbers[ oldNodeIndex ];

                        // if the new node index is invalid remove the given transformation, else remap the node index
                        if (newNodeIndex == MCORE_INVALIDINDEX32)
                        {
                            // remove the transformation as the given node is not present in the new actor
                            morphTarget->RemoveTransformation(j);
                        }
                        else
                        {
                            // remap the node indices of the transformation
                            morphTarget->GetTransformation(j).mNodeIndex = newNodeIndex;
                            j++;
                        }
                    }
                }
            }
        }


        // delete nodes from actor here
        i = 0;
        while (i < actor->GetNumNodes())
        {
            EMotionFX::Node* node           = actor->GetSkeleton()->GetNode(i);
            const uint32    oldNodeIndex    = node->GetNodeIndex();
            const uint32    newNodeIndex    = newNodeNumbers[oldNodeIndex];

            // when the new node index equals the invalid index we have to remove that node
            if (newNodeIndex == MCORE_INVALIDINDEX32)
            {
                // finally remove the node from the actor itself
                //LogDebug("Removing node '%s'", node->GetName());
                actor->RemoveNode(i);
            }
            else
            {
                i++;
            }
        }

        // update the mesh deformer stacks
        for (i = 0; i < actor->GetNumNodes(); ++i)
        {
            EMotionFX::Node* node = actor->GetSkeleton()->GetNode(i);

            // iterate through all LOD levels
            for (uint32 lodLevel = 0; lodLevel < numGeomLODs; ++lodLevel)
            {
                // reinitialize the mesh deformers
                EMotionFX::MeshDeformerStack* meshDeformerStack = actor->GetMeshDeformerStack(lodLevel, i);
                if (meshDeformerStack)
                {
                    meshDeformerStack->ReinitializeDeformers(actor, node, lodLevel);
                }

                /*          // reinitialize the collision mesh deformers
                            EMotionFX::MeshDeformerStack* collisionMeshDeformerStack = actor->GetCollisionMeshDeformerStack(lodLevel, i);
                            if (collisionMeshDeformerStack)
                                collisionMeshDeformerStack->ReinitializeDeformers(actor, node, lodLevel);*/
            }
        }

        // check if the number of nodes equals or collected number and update the node indices
        MCORE_ASSERT(actor->GetNumNodes() == numNodes);
        actor->GetSkeleton()->UpdateNodeIndexValues();
    }


    // save out each mesh of the given actor to an individual actor file
    void SaveDeformableAttachments(const char* fileNameWithoutExtension, EMotionFX::Actor* actor, MCore::Endian::EEndianType targetEndianType, MCore::CommandManager* commandManager)
    {
        EMotionFX::GetEventManager().OnProgressText("Saving skin attachments");

        // some variables we will need
        AZStd::string tempFileName, nodeName;
        MCore::Array<EMotionFX::Node*> meshNodes;

        // calculate the number of mesh nodes
        uint32 i;
        uint32 numMeshNodes = 0;
        const uint32 numNodes = actor->GetNumNodes();
        for (i = 0; i < numNodes; ++i)
        {
            if (actor->GetHasMesh(0, i))
            {
                numMeshNodes++;
            }
        }

        // iterate through all meshes in the actor
        uint32 currentMeshNode = 0;
        for (i = 0; i < numNodes; ++i)
        {
            // get the node and check if there is a mesh assigned
            EMotionFX::Node* node = actor->GetSkeleton()->GetNode(i);
            if (actor->GetHasMesh(0, i))
            {
                AZ::Debug::Timer saveTimer;
                saveTimer.Stamp();

                // add the mesh name post fix to the filename
                AzFramework::StringFunc::Path::GetFileName(fileNameWithoutExtension, tempFileName);

                nodeName = node->GetName();
                AzFramework::StringFunc::Replace(nodeName, " ", "", true);
                AzFramework::StringFunc::Replace(nodeName, "\t", "", true);
                tempFileName += "_";
                tempFileName += nodeName;

                // set the progress information
                EMotionFX::GetEventManager().OnProgressValue(((float)currentMeshNode / (numMeshNodes + 1)) * 100.0f);
                currentMeshNode++;

                // clone the actor so that we can remove all unneeded meshes on this clone before saving
                AZStd::unique_ptr<EMotionFX::Actor> clone = actor->Clone();

                // put our current node into the mesh node array, all these meshes won't get removed in the skin attachment preparation phase
                meshNodes.Clear(false);
                meshNodes.Add(clone->GetSkeleton()->FindNodeByID(node->GetID()));

                // prepare and save the skin attachment
                PrepareDeformableAttachment(clone.get(), meshNodes);

                // save the actor
                Exporter* exporter = Exporter::Create();
                MCore::FileSystem::SaveToFileSecured(tempFileName.c_str(),
                    [exporter, tempFileName, &clone, targetEndianType]
                    {
                        return exporter->SaveActor(tempFileName, clone.get(), targetEndianType);
                    },
                    commandManager);
                exporter->Destroy();

                const float saveTime = saveTimer.GetDeltaTimeInSeconds() * 1000.0f;
                MCore::LogDetailedInfo("Skin attachment '%s' saved in %.2f ms.", nodeName.c_str(), saveTime);
            }
        }


        // save skeleton
        AZ::Debug::Timer saveTimer;
        saveTimer.Stamp();

        // add the mesh name post fix to the filename
        AzFramework::StringFunc::Path::GetFileName(fileNameWithoutExtension, tempFileName);
        tempFileName += "_Skeleton";

        // set the progress information
        EMotionFX::GetEventManager().OnProgressValue(((float)numMeshNodes / (numMeshNodes + 1)) * 100.0f);

        // clone the actor so that we can remove all unneeded meshes on this clone before saving
        AZStd::unique_ptr<EMotionFX::Actor> skeleton = actor->Clone();

        // remove everything mesh related
        skeleton->RemoveAllNodeMeshes();
        skeleton->RemoveAllMaterials();
        skeleton->RemoveAllMorphSetups();

        // save the actor
        Exporter* exporter = Exporter::Create();
        MCore::FileSystem::SaveToFileSecured(tempFileName.c_str(),
            [exporter, tempFileName, &skeleton, targetEndianType]
            {
                return exporter->SaveActor(tempFileName, skeleton.get(), targetEndianType);
            },
            commandManager);
        exporter->Destroy();

        // finish the progress
        EMotionFX::GetEventManager().OnProgressValue(100.0f);

        const float saveTime = saveTimer.GetDeltaTimeInSeconds() * 1000.0f;
        MCore::LogDetailedInfo("Skeleton was saved in %.2f ms.", saveTime);
    }
} // namespace ExporterLib
