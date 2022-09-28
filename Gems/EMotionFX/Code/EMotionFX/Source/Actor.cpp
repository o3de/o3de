/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Asset/AssetManagerBus.h>
#include <AzCore/Math/Quaternion.h>
#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/containers/unordered_set.h>
#include <AzCore/StringFunc/StringFunc.h>
#include <EMotionFX/Source/EMotionFXConfig.h>
#include <EMotionFX/Source/Actor.h>
#include <EMotionFX/Source/ActorBus.h>
#include <EMotionFX/Source/Motion.h>
#include <EMotionFX/Source/MeshDeformerStack.h>
#include <EMotionFX/Source/Mesh.h>
#include <EMotionFX/Source/SubMesh.h>
#include <EMotionFX/Source/MorphMeshDeformer.h>
#include <EMotionFX/Source/SkinningInfoVertexAttributeLayer.h>
#include <EMotionFX/Source/ActorInstance.h>
#include <EMotionFX/Source/MotionSystem.h>
#include <EMotionFX/Source/EventManager.h>
#include <EMotionFX/Source/EventHandler.h>
#include <EMotionFX/Source/NodeMap.h>
#include <EMotionFX/Source/NodeGroup.h>
#include <EMotionFX/Source/ActorManager.h>
#include <EMotionFX/Source/MorphSetup.h>
#include <EMotionFX/Source/MorphTargetStandard.h>
#include <EMotionFX/Source/Node.h>
#include <EMotionFX/Source/Skeleton.h>
#include <EMotionFX/Source/SoftSkinDeformer.h>
#include <EMotionFX/Source/DualQuatSkinDeformer.h>
#include <EMotionFX/Source/DebugDraw.h>
#include <EMotionFX/Source/SimulatedObjectSetup.h>
#include <EMotionFX/Source/SoftSkinManager.h>

#include <MCore/Source/IDGenerator.h>
#include <MCore/Source/Compare.h>
#include <MCore/Source/LogManager.h>

#include <Atom/RPI.Reflect/Model/MorphTargetDelta.h>

namespace EMotionFX
{
    AZ_CLASS_ALLOCATOR_IMPL(Actor, ActorAllocator, 0)

    Actor::MeshLODData::MeshLODData()
    {
        // Create the default LOD level
        m_lodLevels.emplace_back();
    }

    Actor::NodeLODInfo::NodeLODInfo()
    {
        m_mesh   = nullptr;
        m_stack  = nullptr;
    }

    Actor::NodeLODInfo::~NodeLODInfo()
    {
        MCore::Destroy(m_mesh);
        MCore::Destroy(m_stack);
    }

    //----------------------------------------------------

    Actor::Actor(const char* name)
    {
        SetName(name);

        m_skeleton = Skeleton::Create();

        m_motionExtractionNode       = InvalidIndex;
        m_retargetRootNode           = InvalidIndex;
        m_threadIndex                = 0;
        m_customData                 = nullptr;
        m_id                         = aznumeric_caster(MCore::GetIDGenerator().GenerateID());
        m_unitType                   = GetEMotionFX().GetUnitType();
        m_fileUnitType               = m_unitType;
        m_staticAabb                = AZ::Aabb::CreateNull();

        m_usedForVisualization       = false;
        m_dirtyFlag                  = false;

        m_physicsSetup              = AZStd::make_shared<PhysicsSetup>();
        m_simulatedObjectSetup      = AZStd::make_shared<SimulatedObjectSetup>(this);

        m_optimizeSkeleton          = false;

        // make sure we have at least allocated the first LOD of materials and facial setups
        m_morphSetups.reserve(4); //
        m_morphSetups.emplace_back(nullptr);

        GetEventManager().OnCreateActor(this);
        ActorNotificationBus::Broadcast(&ActorNotificationBus::Events::OnActorCreated, this);
    }

    Actor::~Actor()
    {
        ActorNotificationBus::Broadcast(&ActorNotificationBus::Events::OnActorDestroyed, this);
        GetEventManager().OnDeleteActor(this);

        m_nodeMirrorInfos.clear();

        RemoveAllMorphSetups();
        RemoveAllNodeGroups();

        m_invBindPoseTransforms.clear();

        MCore::Destroy(m_skeleton);
    }

    // creates a clone of the actor (a copy).
    // does NOT copy the motions and motion tree
    AZStd::unique_ptr<Actor> Actor::Clone() const
    {
        // create the new actor and set the name and filename
        AZStd::unique_ptr<Actor> result = AZStd::make_unique<Actor>(GetName());
        result->SetFileName(GetFileName());

        // copy the actor attributes
        result->m_motionExtractionNode   = m_motionExtractionNode;
        result->m_unitType               = m_unitType;
        result->m_fileUnitType           = m_fileUnitType;
        result->m_staticAabb            = m_staticAabb;
        result->m_retargetRootNode       = m_retargetRootNode;
        result->m_invBindPoseTransforms  = m_invBindPoseTransforms;
        result->m_optimizeSkeleton      = m_optimizeSkeleton;
        result->m_skinToSkeletonIndexMap = m_skinToSkeletonIndexMap;

        result->RecursiveAddDependencies(this);

        // clone all nodes groups
        for (const NodeGroup* nodeGroup : m_nodeGroups)
        {
            result->AddNodeGroup(aznew NodeGroup(*nodeGroup));
        }

        // clone the skeleton
        MCore::Destroy(result->m_skeleton);
        result->m_skeleton = m_skeleton->Clone();

        // clone lod data
        const size_t numNodes = m_skeleton->GetNumNodes();

        const size_t numLodLevels = m_meshLodData.m_lodLevels.size();
        MeshLODData& resultMeshLodData = result->m_meshLodData;

        result->SetNumLODLevels(static_cast<uint32>(numLodLevels));
        for (size_t lodLevel = 0; lodLevel < numLodLevels; ++lodLevel)
        {
            const AZStd::vector<NodeLODInfo>& nodeInfos = m_meshLodData.m_lodLevels[lodLevel].m_nodeInfos;
            AZStd::vector<NodeLODInfo>& resultNodeInfos = resultMeshLodData.m_lodLevels[lodLevel].m_nodeInfos;

            resultNodeInfos.resize(numNodes);
            for (size_t n = 0; n < numNodes; ++n)
            {
                NodeLODInfo& resultNodeInfo = resultNodeInfos[n];
                const NodeLODInfo& sourceNodeInfo = nodeInfos[n];
                resultNodeInfo.m_mesh = (sourceNodeInfo.m_mesh) ? sourceNodeInfo.m_mesh->Clone() : nullptr;
                resultNodeInfo.m_stack = (sourceNodeInfo.m_stack) ? sourceNodeInfo.m_stack->Clone(resultNodeInfo.m_mesh) : nullptr;
            }
        }

        // clone the morph setups
        result->m_morphSetups.resize(m_morphSetups.size());
        for (size_t i = 0; i < m_morphSetups.size(); ++i)
        {
            if (m_morphSetups[i])
            {
                result->SetMorphSetup(i, m_morphSetups[i]->Clone());
            }
            else
            {
                result->SetMorphSetup(i, nullptr);
            }
        }

        // make sure the number of root nodes is still the same
        MCORE_ASSERT(result->GetSkeleton()->GetNumRootNodes() == m_skeleton->GetNumRootNodes());

        // copy the transform data
        result->CopyTransformsFrom(this);

        result->m_nodeMirrorInfos = m_nodeMirrorInfos;
        result->m_physicsSetup = m_physicsSetup;
        result->SetSimulatedObjectSetup(m_simulatedObjectSetup->Clone(result.get()));

        GetEMotionFX().GetEventManager()->OnPostCreateActor(result.get());

        return result;
    }

    void Actor::SetSimulatedObjectSetup(const AZStd::shared_ptr<SimulatedObjectSetup>& setup)
    {
        m_simulatedObjectSetup = setup;
    }

    // init node mirror info
    void Actor::AllocateNodeMirrorInfos()
    {
        const size_t numNodes = m_skeleton->GetNumNodes();
        m_nodeMirrorInfos.resize(numNodes);

        // init the data
        for (size_t i = 0; i < numNodes; ++i)
        {
            m_nodeMirrorInfos[i].m_sourceNode = static_cast<uint16>(i);
            m_nodeMirrorInfos[i].m_axis       = MCORE_INVALIDINDEX8;
            m_nodeMirrorInfos[i].m_flags      = 0;
        }
    }

    // remove the node mirror info
    void Actor::RemoveNodeMirrorInfos()
    {
        m_nodeMirrorInfos.clear();
        m_nodeMirrorInfos.shrink_to_fit();
    }


    // check if we have our axes detected
    bool Actor::GetHasMirrorAxesDetected() const
    {
        if (m_nodeMirrorInfos.empty())
        {
            return false;
        }

        return AZStd::all_of(begin(m_nodeMirrorInfos), end(m_nodeMirrorInfos), [](const NodeMirrorInfo& nodeMirrorInfo)
        {
            return nodeMirrorInfo.m_axis != MCORE_INVALIDINDEX8;
        });
    }

    // add a LOD level and copy the data from the last LOD level to the new one
    void Actor::AddLODLevel(bool copyFromLastLODLevel)
    {
        AZStd::vector<LODLevel>& lodLevels = m_meshLodData.m_lodLevels;

        lodLevels.emplace_back();
        LODLevel& newLOD = lodLevels.back();
        const size_t numNodes = m_skeleton->GetNumNodes();
        newLOD.m_nodeInfos.resize(numNodes);

        const size_t numLODs    = lodLevels.size();
        const size_t lodIndex   = numLODs - 1;

        // get the number of nodes, iterate through them, create a new LOD level and copy over the meshes from the last LOD level
        for (size_t i = 0; i < numNodes; ++i)
        {
            NodeLODInfo& newLODInfo = lodLevels[lodIndex].m_nodeInfos[static_cast<uint32>(i)];
            if (copyFromLastLODLevel && lodIndex > 0)
            {
                const NodeLODInfo& prevLODInfo = lodLevels[lodIndex - 1].m_nodeInfos[static_cast<uint32>(i)];
                newLODInfo.m_mesh        = (prevLODInfo.m_mesh)        ? prevLODInfo.m_mesh->Clone()                    : nullptr;
                newLODInfo.m_stack       = (prevLODInfo.m_stack)       ? prevLODInfo.m_stack->Clone(newLODInfo.m_mesh)   : nullptr;
            }
            else
            {
                newLODInfo.m_mesh        = nullptr;
                newLODInfo.m_stack       = nullptr;
            }
        }

        // create an empty morph setup for the new LOD level
        m_morphSetups.emplace_back(nullptr);

        // copy data from the previous LOD level if wanted
        if (copyFromLastLODLevel && numLODs > 0)
        {
            CopyLODLevel(this, static_cast<uint32>(lodIndex - 1), static_cast<uint32>(numLODs - 1), true);
        }
    }

    // insert a LOD level at a given position
    void Actor::InsertLODLevel(size_t insertAt)
    {
        AZStd::vector<LODLevel>& lodLevels = m_meshLodData.m_lodLevels;

        lodLevels.emplace(lodLevels.begin()+insertAt);
        LODLevel& newLOD = lodLevels[insertAt];
        const size_t lodIndex   = insertAt;
        const size_t numNodes   = m_skeleton->GetNumNodes();
        newLOD.m_nodeInfos.resize(numNodes);

        // get the number of nodes, iterate through them, create a new LOD level and copy over the meshes from the last LOD level
        for (size_t i = 0; i < numNodes; ++i)
        {
            NodeLODInfo& lodInfo    = lodLevels[lodIndex].m_nodeInfos[i];
            lodInfo.m_mesh           = nullptr;
            lodInfo.m_stack          = nullptr;
        }

        // create an empty morph setup for the new LOD level
        m_morphSetups.emplace(AZStd::next(begin(m_morphSetups), insertAt), nullptr);
    }

    // replace existing LOD level with the current actor
    void Actor::CopyLODLevel(Actor* copyActor, size_t copyLODLevel, size_t replaceLODLevel, bool copySkeletalLODFlags)
    {
        AZStd::vector<LODLevel>& lodLevels = m_meshLodData.m_lodLevels;
        AZStd::vector<LODLevel>& copyLodLevels = copyActor->m_meshLodData.m_lodLevels;

        const LODLevel& sourceLOD = copyLodLevels[copyLODLevel];
        LODLevel& targetLOD = lodLevels[replaceLODLevel];

        const size_t numNodes = m_skeleton->GetNumNodes();
        for (size_t i = 0; i < numNodes; ++i)
        {
            Node* node     = m_skeleton->GetNode(i);
            Node* copyNode = copyActor->GetSkeleton()->FindNodeByID(node->GetID());

            if (copyNode == nullptr)
            {
                MCore::LogWarning("Actor::CopyLODLevel() - Failed to find node '%s' in the actor we want to copy from.", node->GetName());
            }

            const NodeLODInfo& sourceNodeInfo = sourceLOD.m_nodeInfos[ copyNode->GetNodeIndex() ];
            NodeLODInfo& targetNodeInfo = targetLOD.m_nodeInfos[i];

            // first get rid of existing data
            MCore::Destroy(targetNodeInfo.m_mesh);
            targetNodeInfo.m_mesh        = nullptr;
            MCore::Destroy(targetNodeInfo.m_stack);
            targetNodeInfo.m_stack       = nullptr;

            // if the node exists in both models
            if (copyNode)
            {
                // copy over the mesh and collision mesh
                if (sourceNodeInfo.m_mesh)
                {
                    targetNodeInfo.m_mesh = sourceNodeInfo.m_mesh->Clone();
                }

                // handle the stacks
                if (sourceNodeInfo.m_stack)
                {
                    targetNodeInfo.m_stack = sourceNodeInfo.m_stack->Clone(targetNodeInfo.m_mesh);
                }

                // copy the skeletal LOD flag
                if (copySkeletalLODFlags)
                {
                    node->SetSkeletalLODStatus(replaceLODLevel, copyNode->GetSkeletalLODStatus(copyLODLevel));
                }
            }
        }

        // copy the morph setup
        if (m_morphSetups[replaceLODLevel])
        {
            m_morphSetups[replaceLODLevel]->Destroy();
        }

        if (copyActor->GetMorphSetup(copyLODLevel))
        {
            m_morphSetups[replaceLODLevel] = copyActor->GetMorphSetup(copyLODLevel)->Clone();
        }
        else
        {
            m_morphSetups[replaceLODLevel] = nullptr;
        }
    }

    // preallocate memory for all LOD levels
    void Actor::SetNumLODLevels(size_t numLODs, bool adjustMorphSetup)
    {
        m_meshLodData.m_lodLevels.resize(numLODs);

        if (adjustMorphSetup)
        {
            m_morphSetups.resize(numLODs);
            AZStd::fill(begin(m_morphSetups), AZStd::next(begin(m_morphSetups), numLODs), nullptr);
        }
        else
        {
            if (m_morphSetups.size() < numLODs)
            {
                AZ::u32 num = m_morphSetups.empty() ? 0 : (AZ::u32)m_morphSetups.size();
                for (AZ::u32 i = num; i < numLODs; ++i)
                {
                    m_morphSetups.push_back(nullptr);
                }
            }
        }
    }

    // removes all node meshes and stacks
    void Actor::RemoveAllNodeMeshes()
    {
        const size_t numNodes = m_skeleton->GetNumNodes();

        AZStd::vector<LODLevel>& lodLevels = m_meshLodData.m_lodLevels;
        for (LODLevel& lodLevel : lodLevels)
        {
            for (size_t i = 0; i < numNodes; ++i)
            {
                NodeLODInfo& info = lodLevel.m_nodeInfos[i];
                MCore::Destroy(info.m_mesh);
                info.m_mesh = nullptr;
                MCore::Destroy(info.m_stack);
                info.m_stack = nullptr;
            }
        }
    }


    void Actor::CalcMeshTotals(size_t lodLevel, uint32* outNumPolygons, uint32* outNumVertices, uint32* outNumIndices) const
    {
        uint32 totalPolys   = 0;
        uint32 totalVerts   = 0;
        uint32 totalIndices = 0;

        const size_t numNodes = m_skeleton->GetNumNodes();
        for (size_t i = 0; i < numNodes; ++i)
        {
            Mesh* mesh = GetMesh(lodLevel, i);
            if (!mesh)
            {
                continue;
            }

            totalVerts   += mesh->GetNumVertices();
            totalIndices += mesh->GetNumIndices();
            totalPolys   += mesh->GetNumPolygons();
        }

        if (outNumPolygons)
        {
            *outNumPolygons = totalPolys;
        }

        if (outNumVertices)
        {
            *outNumVertices = totalVerts;
        }

        if (outNumIndices)
        {
            *outNumIndices = totalIndices;
        }
    }


    void Actor::CalcStaticMeshTotals(size_t lodLevel, uint32* outNumVertices, uint32* outNumIndices)
    {
        // the totals
        uint32 totalVerts   = 0;
        uint32 totalIndices = 0;

        // for all nodes
        const size_t numNodes = m_skeleton->GetNumNodes();
        for (size_t i = 0; i < numNodes; ++i)
        {
            Mesh* mesh = GetMesh(lodLevel, i);

            // if there is no mesh at this LOD level, skip to the next node
            if (mesh == nullptr)
            {
                continue;
            }

            // the node is dynamic, and we only want static meshes, so skip to the next node
            MeshDeformerStack* stack = GetMeshDeformerStack(lodLevel, i);
            if (stack && stack->GetNumDeformers() > 0)
            {
                continue;
            }

            // sum the values to the totals
            totalVerts   += mesh->GetNumVertices();
            totalIndices += mesh->GetNumIndices();
        }

        // output the number of vertices
        if (outNumVertices)
        {
            *outNumVertices = totalVerts;
        }

        // output the number of indices
        if (outNumIndices)
        {
            *outNumIndices = totalIndices;
        }
    }


    void Actor::CalcDeformableMeshTotals(size_t lodLevel, uint32* outNumVertices, uint32* outNumIndices)
    {
        // the totals
        uint32 totalVerts   = 0;
        uint32 totalIndices = 0;

        // for all nodes
        const size_t numNodes = m_skeleton->GetNumNodes();
        for (size_t i = 0; i < numNodes; ++i)
        {
            Mesh* mesh = GetMesh(lodLevel, i);

            // if there is no mesh at this LOD level, skip to the next node
            if (mesh == nullptr)
            {
                continue;
            }

            // the node is not dynamic (so static), and we only want dynamic meshes, so skip to the next node
            MeshDeformerStack* stack = GetMeshDeformerStack(lodLevel, i);
            if (stack == nullptr || stack->GetNumDeformers() == 0)
            {
                continue;
            }

            // sum the values to the totals
            totalVerts   += mesh->GetNumVertices();
            totalIndices += mesh->GetNumIndices();
        }

        // output the number of vertices
        if (outNumVertices)
        {
            *outNumVertices = totalVerts;
        }

        // output the number of indices
        if (outNumIndices)
        {
            *outNumIndices = totalIndices;
        }
    }


    size_t Actor::CalcMaxNumInfluences(size_t lodLevel) const
    {
        size_t maxInfluences = 0;

        const size_t numNodes = m_skeleton->GetNumNodes();
        for (size_t i = 0; i < numNodes; ++i)
        {
            Mesh* mesh = GetMesh(lodLevel, i);
            if (!mesh)
            {
                continue;
            }

            maxInfluences = AZStd::max(maxInfluences, mesh->CalcMaxNumInfluences());
        }

        return maxInfluences;
    }


    // verify if the skinning will look correctly in the given geometry LOD for a given skeletal LOD level
    void Actor::VerifySkinning(AZStd::vector<uint8>& conflictNodeFlags, size_t skeletalLODLevel, size_t geometryLODLevel)
    {
        // get the number of nodes
        const size_t numNodes = m_skeleton->GetNumNodes();

        // check if the conflict node flag array's size is set to the number of nodes inside the actor
        if (conflictNodeFlags.size() != numNodes)
        {
            conflictNodeFlags.resize(numNodes);
        }

        // reset the conflict node array to zero which means we don't have any conflicting nodes yet
        MCore::MemSet(conflictNodeFlags.data(), 0, numNodes * sizeof(int8));

        // iterate over the all nodes in the actor
        for (size_t n = 0; n < numNodes; ++n)
        {
            // get the current node and the pointer to the mesh for the given lod level
            Node* node = m_skeleton->GetNode(n);
            Mesh* mesh = GetMesh(geometryLODLevel, n);

            // skip nodes without meshes
            if (mesh == nullptr)
            {
                continue;
            }

            // find the skinning information, if it doesn't exist, skip to the next node
            SkinningInfoVertexAttributeLayer* skinningLayer = (SkinningInfoVertexAttributeLayer*)mesh->FindSharedVertexAttributeLayer(SkinningInfoVertexAttributeLayer::TYPE_ID);
            if (skinningLayer == nullptr)
            {
                continue;
            }

            // get the number of original vertices and iterate through them
            const uint32 numOrgVerts = mesh->GetNumOrgVertices();
            for (uint32 v = 0; v < numOrgVerts; ++v)
            {
                // for all influences for this vertex
                const size_t numInfluences = skinningLayer->GetNumInfluences(v);
                for (size_t i = 0; i < numInfluences; ++i)
                {
                    // get the node number of the bone
                    uint16 nodeNr = skinningLayer->GetInfluence(v, i)->GetNodeNr();

                    // if the current skinning influence is linked to a node which is disabled in the given
                    // skeletal LOD we will end up with a badly skinned character, set its flag to conflict true
                    if (node->GetSkeletalLODStatus(skeletalLODLevel) == false)
                    {
                        conflictNodeFlags[nodeNr] = 1;
                    }
                }
            }
        }
    }


    size_t Actor::CalcMaxNumInfluences(size_t lodLevel, AZStd::vector<size_t>& outVertexCounts) const
    {
        // Reset the values.
        outVertexCounts.resize(CalcMaxNumInfluences(lodLevel) + 1);
        AZStd::fill(begin(outVertexCounts), end(outVertexCounts), 0);

        // Get the vertex counts for the influences. (e.g. 500 vertices have 1 skinning influence, 300 vertices have 2 skinning influences etc.)
        size_t maxInfluences = 0;
        AZStd::vector<size_t> meshVertexCounts;
        const size_t numNodes = GetNumNodes();
        for (size_t i = 0; i < numNodes; ++i)
        {
            Mesh* mesh = GetMesh(lodLevel, i);
            if (!mesh)
            {
                continue;
            }

            const size_t meshMaxInfluences = mesh->CalcMaxNumInfluences(meshVertexCounts);
            maxInfluences = AZStd::max(maxInfluences, meshMaxInfluences);

            for (size_t j = 0; j < meshVertexCounts.size(); ++j)
            {
                outVertexCounts[j] += meshVertexCounts[j];
            }
        }

        return maxInfluences;
    }

    // check if there is any mesh available
    bool Actor::CheckIfHasMeshes(size_t lodLevel) const
    {
        // check if any of the nodes has a mesh
        const size_t numNodes = m_skeleton->GetNumNodes();
        for (size_t i = 0; i < numNodes; ++i)
        {
            if (GetMesh(lodLevel, i))
            {
                return true;
            }
        }

        // aaaah, no meshes found
        return false;
    }


    bool Actor::CheckIfHasSkinnedMeshes(size_t lodLevel) const
    {
        const size_t numNodes = m_skeleton->GetNumNodes();
        for (size_t i = 0; i < numNodes; ++i)
        {
            const Mesh* mesh = GetMesh(lodLevel, i);
            if (mesh && mesh->FindSharedVertexAttributeLayer(SkinningInfoVertexAttributeLayer::TYPE_ID))
            {
                return true;
            }
        }

        return false;
    }


    void Actor::SetPhysicsSetup(const AZStd::shared_ptr<PhysicsSetup>& physicsSetup)
    {
        m_physicsSetup = physicsSetup;
    }


    const AZStd::shared_ptr<PhysicsSetup>& Actor::GetPhysicsSetup() const
    {
        return m_physicsSetup;
    }

    const AZStd::shared_ptr<SimulatedObjectSetup>& Actor::GetSimulatedObjectSetup() const
    {
        return m_simulatedObjectSetup;
    }

    // remove all morph setups
    void Actor::RemoveAllMorphSetups(bool deleteMeshDeformers)
    {
        // get the number of lod levels
        const size_t numLODs = GetNumLODLevels();

        // for all LODs, get rid of all the morph setups for each geometry LOD
        for (MorphSetup* morphSetup : m_morphSetups)
        {
            if (morphSetup)
            {
                morphSetup->Destroy();
            }

            morphSetup = nullptr;
        }

        // remove all modifiers from the stacks for each lod in all nodes
        if (deleteMeshDeformers)
        {
            // for all nodes
            const size_t numNodes = m_skeleton->GetNumNodes();
            for (size_t i = 0; i < numNodes; ++i)
            {
                // process all LOD levels
                for (size_t lod = 0; lod < numLODs; ++lod)
                {
                    // if we have a modifier stack
                    MeshDeformerStack* stack = GetMeshDeformerStack(lod, i);
                    if (stack)
                    {
                        // remove all smart mesh morph deformers from that mesh deformer stack
                        stack->RemoveAllDeformersByType(MorphMeshDeformer::TYPE_ID);

                        // if there are no deformers left in the stack, remove the stack
                        if (stack->GetNumDeformers() == 0)
                        {
                            MCore::Destroy(stack);
                            SetMeshDeformerStack(lod, i, nullptr);
                        }
                    }
                }
            }
        }
    }

    // try to find the motion extraction node automatically
    Node* Actor::FindBestMotionExtractionNode() const
    {
        Node* result = nullptr;

        // the maximum number of children of a root node, the node with the most children
        // will become our repositioning node
        size_t maxNumChilds = 0;

        // traverse through all root nodes
        const size_t numRootNodes = m_skeleton->GetNumRootNodes();
        for (size_t i = 0; i < numRootNodes; ++i)
        {
            // get the given root node from the actor
            Node* rootNode = m_skeleton->GetNode(m_skeleton->GetRootNodeIndex(i));

            // get the number of child nodes recursively
            const size_t numChildNodes = rootNode->GetNumChildNodesRecursive();

            // if the number of child nodes of this node is bigger than the current max number
            // this is our new candidate for the repositioning node
            if (numChildNodes > maxNumChilds)
            {
                maxNumChilds = numChildNodes;
                result = rootNode;
            }
        }

        return result;
    }


    // automatically find and set the best motion extraction
    void Actor::AutoSetMotionExtractionNode()
    {
        SetMotionExtractionNode(FindBestMotionExtractionNode());
    }


    // extract a bone list
    void Actor::ExtractBoneList(size_t lodLevel, AZStd::vector<size_t>* outBoneList) const
    {
        // clear the existing items
        outBoneList->clear();

        // for all nodes
        const size_t numNodes = m_skeleton->GetNumNodes();
        for (size_t n = 0; n < numNodes; ++n)
        {
            Mesh* mesh = GetMesh(lodLevel, n);

            // skip nodes without meshes
            if (mesh == nullptr)
            {
                continue;
            }

            // find the skinning information, if it doesn't exist, skip to the next node
            SkinningInfoVertexAttributeLayer* skinningLayer = (SkinningInfoVertexAttributeLayer*)mesh->FindSharedVertexAttributeLayer(SkinningInfoVertexAttributeLayer::TYPE_ID);
            if (skinningLayer == nullptr)
            {
                continue;
            }

            // iterate through all skinning data
            const uint32 numOrgVerts = mesh->GetNumOrgVertices();
            for (uint32 v = 0; v < numOrgVerts; ++v)
            {
                // for all influences for this vertex
                const size_t numInfluences = skinningLayer->GetNumInfluences(v);
                for (size_t i = 0; i < numInfluences; ++i)
                {
                    // get the node number of the bone
                    uint16 nodeNr = skinningLayer->GetInfluence(v, i)->GetNodeNr();

                    // check if it is already in the bone list, if not, add it
                    if (AZStd::find(begin(*outBoneList), end(*outBoneList), nodeNr) == end(*outBoneList))
                    {
                        outBoneList->emplace_back(nodeNr);
                    }
                }
            }
        }
    }


    // recursively add dependencies
    void Actor::RecursiveAddDependencies(const Actor* actor)
    {
        // process all dependencies of the given actor
        const size_t numDependencies = actor->GetNumDependencies();
        for (size_t i = 0; i < numDependencies; ++i)
        {
            // add it to the actor instance
            m_dependencies.emplace_back(*actor->GetDependency(i));

            // recursive into the actor we are dependent on
            RecursiveAddDependencies(actor->GetDependency(i)->m_actor);
        }
    }

    // remove all node groups
    void Actor::RemoveAllNodeGroups()
    {
        for (NodeGroup*& nodeGroup : m_nodeGroups)
        {
            delete nodeGroup;
        }
        m_nodeGroups.clear();
    }


    // try to find a match for a given node with a given name
    // for example find "Bip01 L Hand" for node "Bip01 R Hand"
    uint16 Actor::FindBestMatchForNode(const char* nodeName, const char* subStringA, const char* subStringB, bool firstPass) const
    {
        char newString[1024];
        AZStd::string nameA;
        AZStd::string nameB;

        // search through all nodes to find the best match
        const size_t numNodes = m_skeleton->GetNumNodes();
        for (size_t n = 0; n < numNodes; ++n)
        {
            // get the node name
            const char* name = m_skeleton->GetNode(n)->GetName();

            // check if a substring appears inside this node's name
            if (strstr(name, subStringB))
            {
                // remove the substrings from the names
                nameA = nodeName;
                nameB = name;

                uint32 offset = 0;
                char* stringData = nameA.data();
                MCore::MemSet(newString, 0, 1024 * sizeof(char));
                while (offset < nameA.size())
                {
                    // locate the substring
                    stringData = strstr(stringData, subStringA);
                    if (stringData == nullptr)
                    {
                        break;
                    }

                    // replace the substring
                    // replace subStringA with subStringB
                    offset = static_cast<uint32>(stringData - nameA.data());

                    azstrncpy(newString, 1024, nameA.c_str(), offset);
                    azstrcat(newString, 1024, subStringB);
                    azstrcat(newString, 1024, stringData + strlen(subStringA));

                    stringData += strlen(subStringA);

                    // we found a match
                    if (nameB == newString)
                    {
                        return static_cast<uint16>(n);
                    }
                }
            }
        }

        if (firstPass)
        {
            return FindBestMatchForNode(nodeName, subStringB, subStringA, false); // try it the other way around (substring wise)
        }
        // return the best match
        return MCORE_INVALIDINDEX16;
    }


    // map motion source data of node 'sourceNodeName' to 'destNodeName' and the other way around
    bool Actor::MapNodeMotionSource(const char* sourceNodeName, const char* destNodeName)
    {
        // find the source node index
        const size_t sourceNodeIndex = m_skeleton->FindNodeByNameNoCase(sourceNodeName)->GetNodeIndex();
        if (sourceNodeIndex == InvalidIndex)
        {
            return false;
        }

        // find the dest node index
        const size_t destNodeIndex = m_skeleton->FindNodeByNameNoCase(destNodeName)->GetNodeIndex();
        if (destNodeIndex == InvalidIndex)
        {
            return false;
        }

        // allocate the data if we haven't already
        if (m_nodeMirrorInfos.empty())
        {
            AllocateNodeMirrorInfos();
        }

        // apply the mapping
        m_nodeMirrorInfos[ destNodeIndex   ].m_sourceNode = static_cast<uint16>(sourceNodeIndex);
        m_nodeMirrorInfos[ sourceNodeIndex ].m_sourceNode = static_cast<uint16>(destNodeIndex);

        // we succeeded, because both source and dest have been found
        return true;
    }


    // map two nodes for mirroring
    bool Actor::MapNodeMotionSource(uint16 sourceNodeIndex, uint16 targetNodeIndex)
    {
        // allocate the data if we haven't already
        if (m_nodeMirrorInfos.empty())
        {
            AllocateNodeMirrorInfos();
        }

        // apply the mapping
        m_nodeMirrorInfos[ targetNodeIndex   ].m_sourceNode   = static_cast<uint16>(sourceNodeIndex);
        m_nodeMirrorInfos[ sourceNodeIndex ].m_sourceNode     = static_cast<uint16>(targetNodeIndex);

        // we succeeded, because both source and dest have been found
        return true;
    }



    // match the node motion sources
    // substrings could be "Left " and "Right " to map the nodes "Left Hand" and "Right Hand" to eachother
    void Actor::MatchNodeMotionSources(const char* subStringA, const char* subStringB)
    {
        // try to map all nodes
        const size_t numNodes = m_skeleton->GetNumNodes();
        for (size_t i = 0; i < numNodes; ++i)
        {
            Node* node = m_skeleton->GetNode(i);

            // find the best match
            const uint16 bestIndex = FindBestMatchForNode(node->GetName(), subStringA, subStringB);

            // if a best match has been found
            if (bestIndex != MCORE_INVALIDINDEX16)
            {
                MCore::LogDetailedInfo("%s <---> %s", node->GetName(), m_skeleton->GetNode(bestIndex)->GetName());
                MapNodeMotionSource(node->GetName(), m_skeleton->GetNode(bestIndex)->GetName());
            }
        }
    }


    // set the name of the actor
    void Actor::SetName(const char* name)
    {
        m_name = name;
    }


    // set the filename of the actor
    void Actor::SetFileName(const char* filename)
    {
        m_fileName = filename;
    }


    // find the first active parent node in a given skeletal LOD
    size_t Actor::FindFirstActiveParentBone(size_t skeletalLOD, size_t startNodeIndex) const
    {
        size_t curNodeIndex = startNodeIndex;

        do
        {
            curNodeIndex = m_skeleton->GetNode(curNodeIndex)->GetParentIndex();
            if (curNodeIndex == InvalidIndex)
            {
                return curNodeIndex;
            }

            if (m_skeleton->GetNode(curNodeIndex)->GetSkeletalLODStatus(skeletalLOD))
            {
                return curNodeIndex;
            }
        } while (curNodeIndex != InvalidIndex);

        return InvalidIndex;
    }

    // make the geometry LOD levels compatible with the skeletal LOD levels
    // it remaps skinning influences of vertices that are linked to disabled bones, to other enabled bones
    void Actor::MakeGeomLODsCompatibleWithSkeletalLODs()
    {
        const AZStd::vector<LODLevel>& lodLevels = m_meshLodData.m_lodLevels;

        // for all geometry lod levels
        const size_t numGeomLODs = lodLevels.size();
        for (size_t geomLod = 0; geomLod < numGeomLODs; ++geomLod)
        {
            // for all nodes
            const size_t numNodes = m_skeleton->GetNumNodes();
            for (size_t n = 0; n < numNodes; ++n)
            {
                Node* node = m_skeleton->GetNode(n);

                // check if this node has a mesh, if not we can skip it
                Mesh* mesh = GetMesh(static_cast<uint32>(geomLod), n);
                if (mesh == nullptr)
                {
                    continue;
                }

                // check if the mesh is skinned, if not, we don't need to do anything
                SkinningInfoVertexAttributeLayer* layer = (SkinningInfoVertexAttributeLayer*)mesh->FindSharedVertexAttributeLayer(SkinningInfoVertexAttributeLayer::TYPE_ID);
                if (layer == nullptr)
                {
                    continue;
                }

                // get shortcuts to the original vertex numbers
                const uint32* orgVertices = (uint32*)mesh->FindOriginalVertexData(Mesh::ATTRIB_ORGVTXNUMBERS);

                // for all submeshes
                const size_t numSubMeshes = mesh->GetNumSubMeshes();
                for (size_t s = 0; s < numSubMeshes; ++s)
                {
                    SubMesh* subMesh = mesh->GetSubMesh(s);

                    // for all vertices in the submesh
                    const uint32 startVertex = subMesh->GetStartVertex();
                    const uint32 numVertices = subMesh->GetNumVertices();
                    for (uint32 v = 0; v < numVertices; ++v)
                    {
                        const uint32 vertexIndex    = startVertex + v;
                        const uint32 orgVertex      = orgVertices[vertexIndex];

                        // for all skinning influences of the vertex
                        const size_t numInfluences = layer->GetNumInfluences(orgVertex);
                        for (size_t i = 0; i < numInfluences; ++i)
                        {
                            // if the bone is disabled
                            SkinInfluence* influence = layer->GetInfluence(orgVertex, i);
                            if (m_skeleton->GetNode(influence->GetNodeNr())->GetSkeletalLODStatus(static_cast<uint32>(geomLod)) == false)
                            {
                                // find the first parent bone that is enabled in this LOD
                                const size_t newNodeIndex = FindFirstActiveParentBone(geomLod, influence->GetNodeNr());
                                if (newNodeIndex == InvalidIndex)
                                {
                                    MCore::LogWarning("EMotionFX::Actor::MakeGeomLODsCompatibleWithSkeletalLODs() - Failed to find an enabled parent for node '%s' in skeletal LOD %d of actor '%s' (0x%x)", node->GetName(), geomLod, GetFileName(), this);
                                    continue;
                                }

                                // set the new node index
                                influence->SetNodeNr(static_cast<uint16>(newNodeIndex));
                            }
                        } // for all influences

                        // optimize the influences
                        // if they all use the same bone, just make one influence of it with weight 1.0
                        for (uint32 x = 0; x < numVertices; ++x)
                        {
                            layer->CollapseInfluences(orgVertices[startVertex + x]);
                        }
                    } // for all verts

                    // clear the bones array
                    subMesh->ReinitBonesArray(layer);
                } // for all submeshes

                // reinit the mesh deformer stacks
                MeshDeformerStack* stack = GetMeshDeformerStack(static_cast<uint32>(geomLod), node->GetNodeIndex());
                if (stack)
                {
                    stack->ReinitializeDeformers(this, node, static_cast<uint32>(geomLod));
                }
            } // for all nodes
        }
    }


    // generate a path from the current node towards the root
    void Actor::GenerateUpdatePathToRoot(size_t endNodeIndex, AZStd::vector<size_t>& outPath) const
    {
        outPath.clear();
        outPath.reserve(32);

        // start at the end effector
        Node* currentNode = m_skeleton->GetNode(endNodeIndex);
        while (currentNode)
        {
            // add the current node to the update list
            outPath.emplace_back(currentNode->GetNodeIndex());

            // move up the hierarchy, towards the root and end node
            currentNode = currentNode->GetParentNode();
        }
    }

    void Actor::SetMotionExtractionNode(Node* node)
    {
        if (node)
        {
            SetMotionExtractionNodeIndex(node->GetNodeIndex());
        }
        else
        {
            SetMotionExtractionNodeIndex(InvalidIndex);
        }
    }

    void Actor::SetMotionExtractionNodeIndex(size_t nodeIndex)
    {
        m_motionExtractionNode = nodeIndex;
        ActorNotificationBus::Broadcast(&ActorNotificationBus::Events::OnMotionExtractionNodeChanged, this, GetMotionExtractionNode());
    }

    Node* Actor::GetMotionExtractionNode() const
    {
        if (m_motionExtractionNode != InvalidIndex &&
            m_motionExtractionNode < m_skeleton->GetNumNodes())
        {
            return m_skeleton->GetNode(m_motionExtractionNode);
        }

        return nullptr;
    }

    void Actor::ReinitializeMeshDeformers()
    {
        const size_t numLODLevels = GetNumLODLevels();
        const size_t numNodes = m_skeleton->GetNumNodes();
        for (size_t i = 0; i < numNodes; ++i)
        {
            Node* node = m_skeleton->GetNode(i);

            // iterate through all LOD levels
            for (size_t lodLevel = 0; lodLevel < numLODLevels; ++lodLevel)
            {
                // reinit the mesh deformer stacks
                MeshDeformerStack* stack = GetMeshDeformerStack(lodLevel, i);
                if (stack)
                {
                    stack->ReinitializeDeformers(this, node, lodLevel);
                }
            }
        }
    }

    // post init
    void Actor::PostCreateInit(bool makeGeomLodsCompatibleWithSkeletalLODs, bool convertUnitType)
    {
        if (m_threadIndex == MCORE_INVALIDINDEX32)
        {
            m_threadIndex = 0;
        }

        // calculate the inverse bind pose matrices
        const Pose* bindPose = GetBindPose();
        const size_t numNodes = m_skeleton->GetNumNodes();
        m_invBindPoseTransforms.resize(numNodes);
        for (size_t i = 0; i < numNodes; ++i)
        {
            m_invBindPoseTransforms[i] = bindPose->GetModelSpaceTransform(i).Inversed();
        }

        // make sure the skinning info doesn't use any disabled bones
        if (makeGeomLodsCompatibleWithSkeletalLODs)
        {
            MakeGeomLODsCompatibleWithSkeletalLODs();
        }

        // initialize the mesh deformers
        ReinitializeMeshDeformers();

        // make sure our world space bind pose is updated too
        if (m_morphSetups.size() > 0 && m_morphSetups[0])
        {
            m_skeleton->GetBindPose()->ResizeNumMorphs(m_morphSetups[0]->GetNumMorphTargets());
        }
        m_skeleton->GetBindPose()->ForceUpdateFullModelSpacePose();
        m_skeleton->GetBindPose()->ZeroMorphWeights();

        if (!GetHasMirrorInfo())
        {
            AllocateNodeMirrorInfos();
        }

        // auto detect mirror axes
        if(GetHasMirrorAxesDetected() == false)
        {
            AutoDetectMirrorAxes();
        }

        m_simulatedObjectSetup->InitAfterLoad(this);

        // rescale all content if needed
        if (convertUnitType)
        {
            ScaleToUnitType(GetEMotionFX().GetUnitType());
        }

        // post create actor
        EMotionFX::GetEventManager().OnPostCreateActor(this);
    }

    AZ::Data::AssetId Actor::ConstructSkinMetaAssetId(const AZ::Data::AssetId& meshAssetId)
    {
        AZStd::string meshAssetPath;
        AZ::Data::AssetCatalogRequestBus::BroadcastResult(meshAssetPath, &AZ::Data::AssetCatalogRequests::GetAssetPathById, meshAssetId);
        AZStd::string meshAssetFileName;
        AzFramework::StringFunc::Path::GetFileName(meshAssetPath.c_str(), meshAssetFileName);

        return AZ::RPI::SkinMetaAsset::ConstructAssetId(meshAssetId, meshAssetFileName);
    }

    bool Actor::DoesSkinMetaAssetExist(const AZ::Data::AssetId& meshAssetId)
    {
        const AZ::Data::AssetId skinMetaAssetId = ConstructSkinMetaAssetId(meshAssetId);

        AZ::Data::AssetInfo skinMetaAssetInfo;
        AZ::Data::AssetCatalogRequestBus::BroadcastResult(skinMetaAssetInfo, &AZ::Data::AssetCatalogRequestBus::Events::GetAssetInfoById, skinMetaAssetId);

        return skinMetaAssetInfo.m_assetId.IsValid();
    }

    AZ::Data::AssetId Actor::ConstructMorphTargetMetaAssetId(const AZ::Data::AssetId& meshAssetId)
    {
        AZStd::string meshAssetPath;
        AZ::Data::AssetCatalogRequestBus::BroadcastResult(meshAssetPath, &AZ::Data::AssetCatalogRequests::GetAssetPathById, meshAssetId);
        AZStd::string meshAssetFileName;
        AzFramework::StringFunc::Path::GetFileName(meshAssetPath.c_str(), meshAssetFileName);

        return AZ::RPI::MorphTargetMetaAsset::ConstructAssetId(meshAssetId, meshAssetFileName);
    }

    bool Actor::DoesMorphTargetMetaAssetExist(const AZ::Data::AssetId& meshAssetId)
    {
        const AZ::Data::AssetId morphTargetMetaAssetId = ConstructMorphTargetMetaAssetId(meshAssetId);

        AZ::Data::AssetInfo morphTargetMetaAssetInfo;
        AZ::Data::AssetCatalogRequestBus::BroadcastResult(morphTargetMetaAssetInfo, &AZ::Data::AssetCatalogRequestBus::Events::GetAssetInfoById, morphTargetMetaAssetId);

        return morphTargetMetaAssetInfo.m_assetId.IsValid();
    }

    void Actor::Finalize(LoadRequirement loadReq)
    {
        AZStd::scoped_lock<AZStd::recursive_mutex> lock(m_mutex);

        // Load the mesh asset, skin meta asset and morph target asset.
        // Those sub assets should have already been setup as dependency of actor asset, so they should already be loaded when we reach here.
        // Only exception is that when the actor is not loaded by an actor asset, for which we need to do a blocking load.
        if (m_meshAssetId.IsValid())
        {
            // Get the mesh asset.
            m_meshAsset = AZ::Data::AssetManager::Instance().GetAsset<AZ::RPI::ModelAsset>(m_meshAssetId, AZ::Data::AssetLoadBehavior::PreLoad);

            // Get the skin meta asset.
            const AZ::Data::AssetId skinMetaAssetId = ConstructSkinMetaAssetId(m_meshAssetId);
            if (DoesSkinMetaAssetExist(m_meshAssetId) && skinMetaAssetId.IsValid())
            {
                m_skinMetaAsset = AZ::Data::AssetManager::Instance().GetAsset<AZ::RPI::SkinMetaAsset>(
                    skinMetaAssetId, AZ::Data::AssetLoadBehavior::PreLoad);
            }

            // Get the morph target meta asset.
            const AZ::Data::AssetId morphTargetMetaAssetId = ConstructMorphTargetMetaAssetId(m_meshAssetId);
            if (DoesMorphTargetMetaAssetExist(m_meshAssetId) && morphTargetMetaAssetId.IsValid())
            {
                m_morphTargetMetaAsset = AZ::Data::AssetManager::Instance().GetAsset<AZ::RPI::MorphTargetMetaAsset>(
                    morphTargetMetaAssetId, AZ::Data::AssetLoadBehavior::PreLoad);
            }

            if (loadReq == LoadRequirement::RequireBlockingLoad)
            {
                if (m_skinMetaAsset.IsLoading())
                {
                    m_skinMetaAsset.BlockUntilLoadComplete();
                }
                if (m_morphTargetMetaAsset.IsLoading())
                {
                    m_morphTargetMetaAsset.BlockUntilLoadComplete();
                }
                if (m_meshAsset.IsLoading())
                {
                    m_meshAsset.BlockUntilLoadComplete();
                }
            }
        }

        if (m_meshAsset.IsReady())
        {
            if (m_skinMetaAsset.IsReady())
            {
                m_skinToSkeletonIndexMap = ConstructSkinToSkeletonIndexMap(m_skinMetaAsset);
            }
            else
            {
                AZ_Error(
                    "Actor", false,
                    "Actor finalization: Skin meta asset was expected to be ready but is not ready yet.  Cannot complete finalizing actor %s",
                    this->m_name.c_str());
            }
            ConstructMeshes();

            if (m_morphTargetMetaAsset.IsReady())
            {
                ConstructMorphTargets();
            }
            else
            {
                // Optional, not all actors have morph targets.
                const size_t numLODLevels = m_meshAsset->GetLodAssets().size();
                m_morphSetups.resize(numLODLevels);
                for (size_t i = 0; i < numLODLevels; ++i)
                {
                    m_morphSetups[i] = nullptr;
                }
            }

            // build the static axis aligned bounding box by creating an actor instance (needed to perform cpu skinning mesh deforms and mesh scaling etc)
            // then copy it over to the actor
            UpdateStaticAabb();
        }

        m_isReady = true;
        ActorNotificationBus::Broadcast(&ActorNotificationBus::Events::OnActorReady, this);
        // Do not release the mesh assets. We need the mesh data to initialize future instances of the render actor instances.
    }

    // update the static AABB (very heavy as it has to create an actor instance, update mesh deformers, calculate the mesh based bounds etc)
    void Actor::UpdateStaticAabb()
    {
        if (m_meshAsset && m_meshAsset.IsReady())
        {
            SetStaticAabb(m_meshAsset->GetAabb());
        }
        else
        {
            AZ_Error("Actor", false, "Actor %s is attempting to set the static aabb, but the model asset is not ready yet", m_name.c_str());
        }
    }


    // find the mesh points most influenced by a particular node (pretty expensive function only intended for use in the editor)
    void Actor::FindMostInfluencedMeshPoints(const Node* node, AZStd::vector<AZ::Vector3>& outPoints) const
    {
        outPoints.clear();

        const size_t geomLODLevel = 0;
        const size_t numNodes = m_skeleton->GetNumNodes();

        for (size_t nodeIndex = 0; nodeIndex < numNodes; nodeIndex++)
        {
            // check if this node has a mesh, if not we can skip it
            Mesh* mesh = GetMesh(geomLODLevel, nodeIndex);
            if (mesh == nullptr)
            {
                continue;
            }

            // check if the mesh is skinned, if not, we don't need to do anything
            SkinningInfoVertexAttributeLayer* layer = (SkinningInfoVertexAttributeLayer*)mesh->FindSharedVertexAttributeLayer(SkinningInfoVertexAttributeLayer::TYPE_ID);
            if (layer == nullptr)
            {
                continue;
            }

            // get shortcuts to the original vertex numbers
            const uint32* orgVertices = (uint32*)mesh->FindOriginalVertexData(Mesh::ATTRIB_ORGVTXNUMBERS);
            AZ::Vector3* positions = (AZ::Vector3*)mesh->FindVertexData(EMotionFX::Mesh::ATTRIB_POSITIONS);

            // for all submeshes
            const size_t numSubMeshes = mesh->GetNumSubMeshes();
            for (size_t subMeshIndex = 0; subMeshIndex < numSubMeshes; ++subMeshIndex)
            {
                SubMesh* subMesh = mesh->GetSubMesh(subMeshIndex);

                // for all vertices in the submesh
                const uint32 startVertex = subMesh->GetStartVertex();
                const uint32 numVertices = subMesh->GetNumVertices();
                for (uint32 vertexIndex = 0; vertexIndex < numVertices; ++vertexIndex)
                {
                    const uint32 orgVertex = orgVertices[startVertex + vertexIndex];

                    // for all skinning influences of the vertex
                    const size_t numInfluences = layer->GetNumInfluences(orgVertex);
                    float maxWeight = 0.0f;
                    size_t maxWeightNodeIndex = 0;
                    for (size_t i = 0; i < numInfluences; ++i)
                    {
                        SkinInfluence* influence = layer->GetInfluence(orgVertex, i);
                        float weight = influence->GetWeight();
                        if (weight > maxWeight)
                        {
                            maxWeight = weight;
                            maxWeightNodeIndex = influence->GetNodeNr();
                        }
                    } // for all influences

                    if (maxWeightNodeIndex == node->GetNodeIndex())
                    {
                        outPoints.push_back(positions[vertexIndex + startVertex]);
                    }
                } // for all verts
            } // for all submeshes
        }
    }


    // auto detect the mirror axes
    void Actor::AutoDetectMirrorAxes()
    {
        AZ::Vector3 modelSpaceMirrorPlaneNormal(1.0f, 0.0f, 0.0f);

        Pose pose;
        pose.LinkToActor(this);

        const size_t numNodes = m_nodeMirrorInfos.size();
        for (size_t i = 0; i < numNodes; ++i)
        {
            const uint16 motionSource = (GetHasMirrorInfo()) ? GetNodeMirrorInfo(i).m_sourceNode : static_cast<uint16>(i);

            // displace the local transform a bit, and calculate its mirrored model space position
            pose.InitFromBindPose(this);
            Transform localTransform = pose.GetLocalSpaceTransform(motionSource);
            Transform orgDelta = Transform::CreateIdentity();
            orgDelta.m_position.Set(1.1f, 2.2f, 3.3f);
            orgDelta.m_rotation = MCore::AzEulerAnglesToAzQuat(0.1f, 0.2f, 0.3f);
            Transform delta = orgDelta;
            delta.Multiply(localTransform);
            pose.SetLocalSpaceTransform(motionSource, delta);
            Transform endModelSpaceTransform = pose.GetModelSpaceTransform(motionSource);
            endModelSpaceTransform.Mirror(modelSpaceMirrorPlaneNormal);

            float   minDist     = FLT_MAX;
            uint8   bestAxis    = 0;
            uint8   bestFlags   = 0;
            bool    found       = false;
            for (uint8 a = 0; a < 3; ++a) // mirror along x, y and then z axis
            {
                AZ::Vector3 axis(0.0f, 0.0f, 0.0f);
                axis.SetElement(a, 1.0f);

                // mirror it over the current plane
                pose.InitFromBindPose(this);
                localTransform = pose.GetLocalSpaceTransform(i);
                delta = orgDelta;
                delta.Mirror(axis);
                delta.Multiply(localTransform);
                pose.SetLocalSpaceTransform(i, delta);
                const Transform& modelSpaceResult = pose.GetModelSpaceTransform(i);

                // check if we have a matching distance in model space
                const float dist = MCore::SafeLength(modelSpaceResult.m_position - endModelSpaceTransform.m_position);
                if (dist <= MCore::Math::epsilon)
                {
                    m_nodeMirrorInfos[i].m_axis   = a;
                    m_nodeMirrorInfos[i].m_flags  = 0;
                    found = true;
                    break;
                }

                // record if this is a better match
                if (dist < minDist)
                {
                    minDist     = dist;
                    bestAxis    = a;
                    bestFlags   = 0;
                }
            }

            // try with flipped axes
            if (found == false)
            {
                for (uint8 a = 0; a < 3; ++a) // mirror along x, y and then z axis
                {
                    for (uint8 f = 0; f < 3; ++f) // flip axis
                    {
                        AZ::Vector3 axis(0.0f, 0.0f, 0.0f);
                        axis.SetElement(a, 1.0f);

                        uint8 flags = 0;
                        if (f == 0)
                        {
                            flags = MIRRORFLAG_INVERT_X;
                        }
                        if (f == 1)
                        {
                            flags = MIRRORFLAG_INVERT_Y;
                        }
                        if (f == 2)
                        {
                            flags = MIRRORFLAG_INVERT_Z;
                        }

                        // mirror it over the current plane
                        pose.InitFromBindPose(this);
                        localTransform = pose.GetLocalSpaceTransform(i);
                        delta = orgDelta;
                        delta.MirrorWithFlags(axis, flags);
                        delta.Multiply(localTransform);
                        pose.SetLocalSpaceTransform(i, delta);
                        const Transform& modelSpaceResult = pose.GetModelSpaceTransform(i);

                        // check if we have a matching distance in world space
                        const float dist = MCore::SafeLength(modelSpaceResult.m_position - endModelSpaceTransform.m_position);
                        if (dist <= MCore::Math::epsilon)
                        {
                            m_nodeMirrorInfos[i].m_axis   = a;
                            m_nodeMirrorInfos[i].m_flags  = flags;
                            found = true;
                            break;
                        }

                        // record if this is a better match
                        if (dist < minDist)
                        {
                            minDist     = dist;
                            bestAxis    = a;
                            bestFlags   = flags;
                        }
                    } // for all flips

                    if (found)
                    {
                        break;
                    }
                } // for all mirror axes
            }

            if (found == false)
            {
                m_nodeMirrorInfos[i].m_axis   = bestAxis;
                m_nodeMirrorInfos[i].m_flags  = bestFlags;
            }
        }
    }


    // get the array of node mirror infos
    const AZStd::vector<Actor::NodeMirrorInfo>& Actor::GetNodeMirrorInfos() const
    {
        return m_nodeMirrorInfos;
    }


    // get the array of node mirror infos
    AZStd::vector<Actor::NodeMirrorInfo>& Actor::GetNodeMirrorInfos()
    {
        return m_nodeMirrorInfos;
    }


    // set the node mirror infos directly
    void Actor::SetNodeMirrorInfos(const AZStd::vector<NodeMirrorInfo>& mirrorInfos)
    {
        m_nodeMirrorInfos = mirrorInfos;
    }


    // try to geometrically match left with right nodes
    void Actor::MatchNodeMotionSourcesGeometrical()
    {
        Pose pose;
        pose.InitFromBindPose(this);

        const uint16 numNodes = static_cast<uint16>(m_skeleton->GetNumNodes());
        for (uint16 i = 0; i < numNodes; ++i)
        {
            // find the best match
            const uint16 bestIndex = FindBestMirrorMatchForNode(i, pose);

            // if a best match has been found
            if (bestIndex != MCORE_INVALIDINDEX16)
            {
                //LogDetailedInfo("%s <---> %s", node->GetName(), GetNode(bestIndex)->GetName());
                MapNodeMotionSource(i, bestIndex);
            }
        }
    }


    // find the best matching node index
    uint16 Actor::FindBestMirrorMatchForNode(uint16 nodeIndex, Pose& pose) const
    {
        if (m_skeleton->GetNode(nodeIndex)->GetIsRootNode())
        {
            return MCORE_INVALIDINDEX16;
        }

        // calculate the model space transform and mirror it
        const Transform nodeTransform       = pose.GetModelSpaceTransform(nodeIndex);
        const Transform mirroredTransform   = nodeTransform.Mirrored(AZ::Vector3(1.0f, 0.0f, 0.0f));

        size_t numMatches = 0;
        uint16 result = MCORE_INVALIDINDEX16;

        // find nodes that have the mirrored transform
        const size_t numNodes = m_skeleton->GetNumNodes();
        for (size_t i = 0; i < numNodes; ++i)
        {
            const Transform& curNodeTransform = pose.GetModelSpaceTransform(i);
            if (i != nodeIndex)
            {
                // only check the translation for now
        #ifndef EMFX_SCALE_DISABLED
                if (MCore::Compare<AZ::Vector3>::CheckIfIsClose(
                        curNodeTransform.m_position,
                        mirroredTransform.m_position, MCore::Math::epsilon) &&
                    MCore::Compare<float>::CheckIfIsClose(MCore::SafeLength(curNodeTransform.m_scale),
                        MCore::SafeLength(mirroredTransform.m_scale), MCore::Math::epsilon))
        #else
                if (MCore::Compare<AZ::Vector3>::CheckIfIsClose(curNodeTransform.m_position, mirroredTransform.m_position, MCore::Math::epsilon))
        #endif
                {
                    numMatches++;
                    result = static_cast<uint16>(i);
                }
            }
        }

        if (numMatches == 1)
        {
            const size_t hierarchyDepth         = m_skeleton->CalcHierarchyDepthForNode(nodeIndex);
            const size_t matchingHierarchyDepth = m_skeleton->CalcHierarchyDepthForNode(result);
            if (hierarchyDepth != matchingHierarchyDepth)
            {
                return MCORE_INVALIDINDEX16;
            }

            return result;
        }

        return MCORE_INVALIDINDEX16;
    }


    // resize the transform arrays to the current number of nodes
    void Actor::ResizeTransformData()
    {
        Pose& bindPose = *m_skeleton->GetBindPose();
        bindPose.LinkToActor(this, Pose::FLAG_LOCALTRANSFORMREADY, false);

        const size_t numMorphs = bindPose.GetNumMorphWeights();
        for (size_t i = 0; i < numMorphs; ++i)
        {
            bindPose.SetMorphWeight(i, 0.0f);
        }

        m_invBindPoseTransforms.resize(m_skeleton->GetNumNodes());
    }


    // release any transform data
    void Actor::ReleaseTransformData()
    {
        m_skeleton->GetBindPose()->Clear();
        m_invBindPoseTransforms.clear();
    }


    // copy transforms from another actor
    void Actor::CopyTransformsFrom(const Actor* other)
    {
        MCORE_ASSERT(other->GetNumNodes() == m_skeleton->GetNumNodes());
        ResizeTransformData();
        m_invBindPoseTransforms = other->m_invBindPoseTransforms;
        *m_skeleton->GetBindPose() = *other->GetSkeleton()->GetBindPose();
    }

    void Actor::SetNumNodes(size_t numNodes)
    {
        m_skeleton->SetNumNodes(numNodes);

        AZStd::vector<LODLevel>& lodLevels = m_meshLodData.m_lodLevels;
        for (LODLevel& lodLevel : lodLevels)
        {
            lodLevel.m_nodeInfos.resize(numNodes);
        }

        Pose* bindPose = m_skeleton->GetBindPose();
        bindPose->LinkToActor(this, Pose::FLAG_LOCALTRANSFORMREADY, false);
    }

    void Actor::AddNode(Node* node)
    {
        m_skeleton->AddNode(node);
        m_skeleton->GetBindPose()->LinkToActor(this, Pose::FLAG_LOCALTRANSFORMREADY, false);

        // initialize the LOD data
        AZStd::vector<LODLevel>& lodLevels = m_meshLodData.m_lodLevels;
        for (LODLevel& lodLevel : lodLevels)
        {
            lodLevel.m_nodeInfos.emplace_back();
        }

        m_skeleton->GetBindPose()->LinkToActor(this, Pose::FLAG_LOCALTRANSFORMREADY, false);
        m_skeleton->GetBindPose()->SetLocalSpaceTransform(m_skeleton->GetNumNodes() - 1, Transform::CreateIdentity());
    }

    Node* Actor::AddNode(size_t nodeIndex, const char* name, size_t parentIndex)
    {
        Node* node = Node::Create(name, GetSkeleton());
        node->SetNodeIndex(nodeIndex);
        node->SetParentIndex(parentIndex);
        AddNode(node);
        if (parentIndex == InvalidIndex)
        {
            GetSkeleton()->AddRootNode(node->GetNodeIndex());
        }
        else
        {
            node->GetParentNode()->AddChild(nodeIndex);
        }
        return node;
    }

    void Actor::RemoveNode(size_t nr, bool delMem)
    {
        m_skeleton->RemoveNode(nr, delMem);

        AZStd::vector<LODLevel>& lodLevels = m_meshLodData.m_lodLevels;
        for (LODLevel& lodLevel : lodLevels)
        {
            lodLevel.m_nodeInfos.erase(AZStd::next(begin(lodLevel.m_nodeInfos), nr));
        }
    }

    void Actor::DeleteAllNodes()
    {
        m_skeleton->RemoveAllNodes();

        AZStd::vector<LODLevel>& lodLevels = m_meshLodData.m_lodLevels;
        for (LODLevel& lodLevel : lodLevels)
        {
            lodLevel.m_nodeInfos.clear();
        }
    }


    size_t Actor::GetNumLODLevels() const
    {
        return m_meshLodData.m_lodLevels.size();
    }


    void* Actor::GetCustomData() const
    {
        return m_customData;
    }


    void Actor::SetCustomData(void* dataPointer)
    {
        m_customData = dataPointer;
    }


    const char* Actor::GetName() const
    {
        return m_name.c_str();
    }


    const AZStd::string& Actor::GetNameString() const
    {
        return m_name;
    }


    const char* Actor::GetFileName() const
    {
        return m_fileName.c_str();
    }


    const AZStd::string& Actor::GetFileNameString() const
    {
        return m_fileName;
    }


    void Actor::AddDependency(const Dependency& dependency)
    {
        m_dependencies.emplace_back(dependency);
    }


    void Actor::SetMorphSetup(size_t lodLevel, MorphSetup* setup)
    {
        m_morphSetups[lodLevel] = setup;
    }


    size_t Actor::GetNumNodeGroups() const
    {
        return m_nodeGroups.size();
    }


    NodeGroup* Actor::GetNodeGroup(size_t index) const
    {
        return m_nodeGroups[index];
    }


    void Actor::AddNodeGroup(NodeGroup* newGroup)
    {
        m_nodeGroups.emplace_back(newGroup);
    }


    void Actor::RemoveNodeGroup(size_t index, bool delFromMem)
    {
        if (delFromMem)
        {
            delete m_nodeGroups[index];
        }

        m_nodeGroups.erase(AZStd::next(begin(m_nodeGroups), index));
    }


    void Actor::RemoveNodeGroup(NodeGroup* group, bool delFromMem)
    {
        const auto found = AZStd::find(begin(m_nodeGroups), end(m_nodeGroups), group);
        if (found != end(m_nodeGroups))
        {
            m_nodeGroups.erase(found);
            if (delFromMem)
            {
                delete group;
            }
        }
    }


    // find a group index by its name
    size_t Actor::FindNodeGroupIndexByName(const char* groupName) const
    {
        const auto found = AZStd::find_if(begin(m_nodeGroups), end(m_nodeGroups), [groupName](const NodeGroup* nodeGroup)
        {
            return nodeGroup->GetNameString() == groupName;
        });
        return found != end(m_nodeGroups) ? AZStd::distance(begin(m_nodeGroups), found) : InvalidIndex;
    }


    // find a group index by its name, but not case sensitive
    size_t Actor::FindNodeGroupIndexByNameNoCase(const char* groupName) const
    {
        const auto found = AZStd::find_if(begin(m_nodeGroups), end(m_nodeGroups), [groupName](const NodeGroup* nodeGroup)
        {
            return AzFramework::StringFunc::Equal(nodeGroup->GetNameString(), groupName, false /* no case */);
        });
        return found != end(m_nodeGroups) ? AZStd::distance(begin(m_nodeGroups), found) : InvalidIndex;
    }


    // find a group by its name
    NodeGroup* Actor::FindNodeGroupByName(const char* groupName) const
    {
        const auto found = AZStd::find_if(begin(m_nodeGroups), end(m_nodeGroups), [groupName](const NodeGroup* nodeGroup)
        {
            return nodeGroup->GetNameString() == groupName;
        });
        return found != end(m_nodeGroups) ? *found : nullptr;
    }


    // find a group by its name, but without case sensitivity
    NodeGroup* Actor::FindNodeGroupByNameNoCase(const char* groupName) const
    {
        const auto found = AZStd::find_if(begin(m_nodeGroups), end(m_nodeGroups), [groupName](const NodeGroup* nodeGroup)
        {
            return AzFramework::StringFunc::Equal(nodeGroup->GetNameString(), groupName, false /* no case */);
        });
        return found != end(m_nodeGroups) ? *found : nullptr;
    }


    void Actor::SetDirtyFlag(bool dirty)
    {
        m_dirtyFlag = dirty;
    }


    bool Actor::GetDirtyFlag() const
    {
        return m_dirtyFlag;
    }


    void Actor::SetIsUsedForVisualization(bool flag)
    {
        m_usedForVisualization = flag;
    }


    bool Actor::GetIsUsedForVisualization() const
    {
        return m_usedForVisualization;
    }

    const AZ::Aabb& Actor::GetStaticAabb() const
    {
        return m_staticAabb;
    }

    void Actor::SetStaticAabb(const AZ::Aabb& aabb)
    {
        m_staticAabb = aabb;
    }

    //---------------------------------

    Mesh* Actor::GetMesh(size_t lodLevel, size_t nodeIndex) const
    {
        const AZStd::vector<LODLevel>& lodLevels = m_meshLodData.m_lodLevels;
        return lodLevels[lodLevel].m_nodeInfos[nodeIndex].m_mesh;
    }

    MeshDeformerStack* Actor::GetMeshDeformerStack(size_t lodLevel, size_t nodeIndex) const
    {
        const AZStd::vector<LODLevel>& lodLevels = m_meshLodData.m_lodLevels;
        return lodLevels[lodLevel].m_nodeInfos[nodeIndex].m_stack;
    }

    // set the mesh for a given node in a given LOD
    void Actor::SetMesh(size_t lodLevel, size_t nodeIndex, Mesh* mesh)
    {
        AZStd::vector<LODLevel>& lodLevels = m_meshLodData.m_lodLevels;
        lodLevels[lodLevel].m_nodeInfos[nodeIndex].m_mesh = mesh;
    }


    // set the mesh deformer stack for a given node in a given LOD
    void Actor::SetMeshDeformerStack(size_t lodLevel, size_t nodeIndex, MeshDeformerStack* stack)
    {
        AZStd::vector<LODLevel>& lodLevels = m_meshLodData.m_lodLevels;
        lodLevels[lodLevel].m_nodeInfos[nodeIndex].m_stack = stack;
    }

    // check if the mesh has a skinning deformer (either linear or dual quat)
    bool Actor::CheckIfHasSkinningDeformer(size_t lodLevel, size_t nodeIndex) const
    {
        // check if there is a mesh
        Mesh* mesh = GetMesh(lodLevel, nodeIndex);
        if (!mesh)
        {
            return false;
        }

        // check if there is a mesh deformer stack
        MeshDeformerStack* stack = GetMeshDeformerStack(lodLevel, nodeIndex);
        if (!stack)
        {
            return false;
        }

        return (stack->CheckIfHasDeformerOfType(SoftSkinDeformer::TYPE_ID) || stack->CheckIfHasDeformerOfType(DualQuatSkinDeformer::TYPE_ID));
    }

    // remove the mesh for a given node in a given LOD
    void Actor::RemoveNodeMeshForLOD(size_t lodLevel, size_t nodeIndex, bool destroyMesh)
    {
        AZStd::vector<LODLevel>& lodLevels = m_meshLodData.m_lodLevels;

        LODLevel&       lod         = lodLevels[lodLevel];
        NodeLODInfo&    nodeInfo    = lod.m_nodeInfos[nodeIndex];

        if (destroyMesh && nodeInfo.m_mesh)
        {
            MCore::Destroy(nodeInfo.m_mesh);
        }

        if (destroyMesh && nodeInfo.m_stack)
        {
            MCore::Destroy(nodeInfo.m_stack);
        }

        nodeInfo.m_mesh  = nullptr;
        nodeInfo.m_stack = nullptr;
    }

    void Actor::SetUnitType(MCore::Distance::EUnitType unitType)
    {
        m_unitType = unitType;
    }


    MCore::Distance::EUnitType Actor::GetUnitType() const
    {
        return m_unitType;
    }


    void Actor::SetFileUnitType(MCore::Distance::EUnitType unitType)
    {
        m_fileUnitType = unitType;
    }


    MCore::Distance::EUnitType Actor::GetFileUnitType() const
    {
        return m_fileUnitType;
    }


    // scale all data
    void Actor::Scale(float scaleFactor)
    {
        // if we don't need to adjust the scale, do nothing
        if (MCore::Math::IsFloatEqual(scaleFactor, 1.0f))
        {
            return;
        }

        // scale the bind pose positions
        Pose* bindPose = GetBindPose();
        const size_t numNodes = GetNumNodes();
        for (size_t i = 0; i < numNodes; ++i)
        {
            Transform transform = bindPose->GetLocalSpaceTransform(i);
            transform.m_position *= scaleFactor;
            bindPose->SetLocalSpaceTransform(i, transform);
        }
        bindPose->ForceUpdateFullModelSpacePose();

        // calculate the inverse bind pose matrices
        for (size_t i = 0; i < numNodes; ++i)
        {
            m_invBindPoseTransforms[i] = bindPose->GetModelSpaceTransform(i).Inversed();
        }

        // update static aabb
        m_staticAabb.SetMin(m_staticAabb.GetMin() * scaleFactor);
        m_staticAabb.SetMax(m_staticAabb.GetMax() * scaleFactor);

        // update mesh data for all LOD levels
        const size_t numLODs = GetNumLODLevels();
        for (size_t lod = 0; lod < numLODs; ++lod)
        {
            for (size_t i = 0; i < numNodes; ++i)
            {
                Mesh* mesh = GetMesh(lod, i);
                if (mesh)
                {
                    mesh->Scale(scaleFactor);
                }
            }
        }

        // scale morph target data
        for (size_t lod = 0; lod < numLODs; ++lod)
        {
            MorphSetup* morphSetup = GetMorphSetup(lod);
            if (morphSetup)
            {
                morphSetup->Scale(scaleFactor);
            }
        }

        // initialize the mesh deformers just to be sure
        ReinitializeMeshDeformers();

        // trigger the event
        GetEventManager().OnScaleActorData(this, scaleFactor);
    }


    // scale everything to the given unit type
    void Actor::ScaleToUnitType(MCore::Distance::EUnitType targetUnitType)
    {
        if (m_unitType == targetUnitType)
        {
            return;
        }

        // calculate the scale factor and scale
        const float scaleFactor = static_cast<float>(MCore::Distance::GetConversionFactor(m_unitType, targetUnitType));
        Scale(scaleFactor);

        // update the unit type
        m_unitType = targetUnitType;
    }


    // Try to figure out which axis points "up" for the motion extraction node.
    Actor::EAxis Actor::FindBestMatchingMotionExtractionAxis() const
    {
        MCORE_ASSERT(m_motionExtractionNode != InvalidIndex);
        if (m_motionExtractionNode == InvalidIndex)
        {
            return AXIS_Y;
        }

        // Get the local space rotation matrix of the motion extraction node.
        const Transform& localTransform = GetBindPose()->GetLocalSpaceTransform(m_motionExtractionNode);
        const AZ::Matrix3x3 rotationMatrix = AZ::Matrix3x3::CreateFromQuaternion(localTransform.m_rotation);

        // Calculate angles between the up axis and each of the rotation's basis vectors.
        const AZ::Vector3 globalUpAxis(0.0f, 0.0f, 1.0f);
        const float dotX = rotationMatrix.GetRow(0).Dot(globalUpAxis);
        const float dotY = rotationMatrix.GetRow(1).Dot(globalUpAxis);
        const float dotZ = rotationMatrix.GetRow(2).Dot(globalUpAxis);

        const float difX = 1.0f - MCore::Clamp(MCore::Math::Abs(dotX), 0.0f, 1.0f);
        const float difY = 1.0f - MCore::Clamp(MCore::Math::Abs(dotY), 0.0f, 1.0f);
        const float difZ = 1.0f - MCore::Clamp(MCore::Math::Abs(dotZ), 0.0f, 1.0f);

        // Pick the axis which has the smallest angle difference.
        if (difX <= difY && difY <= difZ)
        {
            return AXIS_X;
        }
        else if (difY <= difX && difX <= difZ)
        {
            return AXIS_Y;
        }
        else
        {
            return AXIS_Z;
        }
    }


    void Actor::SetRetargetRootNodeIndex(size_t nodeIndex)
    {
        m_retargetRootNode = nodeIndex;
    }


    void Actor::SetRetargetRootNode(Node* node)
    {
        m_retargetRootNode = node ? node->GetNodeIndex() : InvalidIndex;
    }

    void Actor::InsertJointAndParents(size_t jointIndex, AZStd::unordered_set<size_t>& includedJointIndices)
    {
        // If our joint is already in, then we can skip things.
        if (includedJointIndices.find(jointIndex) != includedJointIndices.end())
        {
            return;
        }

        // Add the parent.
        const size_t parentIndex = m_skeleton->GetNode(jointIndex)->GetParentIndex();
        if (parentIndex != InvalidIndex)
        {
            InsertJointAndParents(parentIndex, includedJointIndices);
        }

        // Add itself.
        includedJointIndices.emplace(jointIndex);
    }

    void Actor::AutoSetupSkeletalLODsBasedOnSkinningData(const AZStd::vector<AZStd::string>& alwaysIncludeJoints)
    {
        AZStd::unordered_set<size_t> includedJointIndices;

        const size_t numLODs = GetNumLODLevels();
        for (size_t lod = 0; lod < numLODs; ++lod)
        {
            includedJointIndices.clear();

            // If we have no meshes, or only static meshes, we shouldn't do anything.
            if (!CheckIfHasMeshes(lod) || !CheckIfHasSkinnedMeshes(lod))
            {
                continue;
            }

            const size_t numJoints = m_skeleton->GetNumNodes();
            for (size_t jointIndex = 0; jointIndex < numJoints; ++jointIndex)
            {
                const Mesh* mesh = GetMesh(lod, jointIndex);
                if (!mesh)
                {
                    continue;
                }

                // Include the mesh, as we always want to include meshes.
                InsertJointAndParents(jointIndex, includedJointIndices);

                // Look at the joints registered in the submeshes.
                const size_t numSubMeshes = mesh->GetNumSubMeshes();
                for (size_t subMeshIndex = 0; subMeshIndex < numSubMeshes; ++subMeshIndex)
                {
                    const AZStd::vector<size_t>& subMeshJoints = mesh->GetSubMesh(subMeshIndex)->GetBonesArray();
                    for (size_t subMeshJoint : subMeshJoints)
                    {
                        InsertJointAndParents(subMeshJoint, includedJointIndices);
                    }
                }
            } // for all joints

            // Now that we have the list of joints to include for this LOD, let's disable all the joints in this LOD, and enable all the ones in our list.
            if (!includedJointIndices.empty())
            {
                // Force joints in our "always include list" to be included.
                for (const AZStd::string& jointName : alwaysIncludeJoints)
                {
                    size_t jointIndex = InvalidIndex;
                    if (!m_skeleton->FindNodeAndIndexByName(jointName, jointIndex))
                    {
                        if (!jointName.empty())
                        {
                            AZ_Warning("EMotionFX", false, "Cannot find joint '%s' inside the skeleton. This joint name was specified inside the alwaysIncludeJoints list.", jointName.c_str());
                        }
                        continue;
                    }

                    InsertJointAndParents(jointIndex, includedJointIndices);
                }

                // Disable all joints first.
                for (size_t jointIndex = 0; jointIndex < numJoints; ++jointIndex)
                {
                    m_skeleton->GetNode(jointIndex)->SetSkeletalLODStatus(lod, false);
                }

                // Enable all our included joints in this skeletal LOD.
                AZ_TracePrintf("EMotionFX", "[LOD %d] Enabled joints = %zd\n", lod, includedJointIndices.size());
                for (size_t jointIndex : includedJointIndices)
                {
                    m_skeleton->GetNode(jointIndex)->SetSkeletalLODStatus(lod, true);
                }
            }
            else // When we have an empty include list, enable everything.
            {
                AZ_TracePrintf("EMotionFX", "[LOD %d] Enabled joints = %zd\n", lod, m_skeleton->GetNumNodes());
                for (size_t i = 0; i < m_skeleton->GetNumNodes(); ++i)
                {
                    m_skeleton->GetNode(i)->SetSkeletalLODStatus(lod, true);
                }
            }
        } // for each LOD
    }


    void Actor::PrintSkeletonLODs()
    {
        const size_t numLODs = GetNumLODLevels();
        for (size_t lod = 0; lod < numLODs; ++lod)
        {
            AZ_TracePrintf("EMotionFX", "[LOD %d]:", lod);
            const size_t numJoints = m_skeleton->GetNumNodes();
            for (size_t jointIndex = 0; jointIndex < numJoints; ++jointIndex)
            {
                const Node* joint = m_skeleton->GetNode(jointIndex);
                if (joint->GetSkeletalLODStatus(lod))
                {
                    AZ_TracePrintf("EMotionFX", "\t%s (index=%zu)", joint->GetName(), jointIndex);
                }
            }
        }
    }

    void Actor::GenerateOptimizedSkeleton()
    {
        // We should have already removed all the mesh, skinning information, sim object and etc.
        // At this point, we only need to remove extra joint nodes.

        // First, check if we have a hit detection setup.
        // NOTE:: We won't do anything unless the actor has a hit detection collider setup. If you don't have a hit collider associate with animation on server,
        // you should consider not running the anim graph at all.
        if (!m_physicsSetup || m_physicsSetup->GetHitDetectionConfig().m_nodes.empty())
        {
            return;
        }

        // 1) Build a set of nodes that we want to keep in the actor skeleton heirarchy.
        // 2) Mark all the node in the above list and all their predecessors.
        // 3) In actor skeleton, remove every node that hasn't been marked.
        // 4) Meanwhile, build a map that represent the child-parent relationship.
        // 5) After the node index changed, we use the map in 4) to restore the child-parent relationship.
        size_t numNodes = m_skeleton->GetNumNodes();
        AZStd::vector<bool> flags;
        AZStd::unordered_map<AZStd::string, AZStd::string> childParentMap;
        flags.resize(numNodes);

        AZStd::unordered_set<Node*> nodesToKeep;
        // Search the hit detection config to find and keep all the hit detection nodes.
        for (const Physics::CharacterColliderNodeConfiguration& nodeConfig : m_physicsSetup->GetHitDetectionConfig().m_nodes)
        {
            Node* node = m_skeleton->FindNodeByName(nodeConfig.m_name);
            if (node && nodesToKeep.find(node) == nodesToKeep.end())
            {
                nodesToKeep.emplace(node);
            }
        }

        // find our motion extraction node and make sure we keep it.
        Node* motionExtractionNode = GetMotionExtractionNode();
        if (motionExtractionNode)
        {
            nodesToKeep.emplace(motionExtractionNode);
        }

        // Search the actor skeleton to find all the critical nodes.
        for (size_t i = 0; i < numNodes; ++i)
        {
            Node* node = m_skeleton->GetNode(i);
            if (node->GetIsCritical() && nodesToKeep.find(node) == nodesToKeep.end())
            {
                nodesToKeep.emplace(node);
            }
        }

        for (Node* nodeToKeep : nodesToKeep)
        {
            Node* node = nodeToKeep;
            // Mark this node and all its predecessors.
            do
            {
                if (flags[node->GetNodeIndex()])
                {
                   break;
                }
                flags[node->GetNodeIndex()] = true;
                Node* parent = node->GetParentNode();
                if (parent)
                {
                   childParentMap[node->GetNameString()] = parent->GetNameString();
                }
                node = parent;
            } while (node);
        }

        // Remove all the nodes that haven't been marked
        for (size_t nodeIndex = numNodes - 1; nodeIndex > 0; nodeIndex--)
        {
            if (!flags[nodeIndex])
            {
                m_skeleton->RemoveNode(nodeIndex);
            }
        }

        // Update the node index.
        m_skeleton->UpdateNodeIndexValues();

        // After the node index changed, the parent index become invalid. First, clear all information about children because
        // it's not valid anymore.
        for (size_t nodeIndex = 0; nodeIndex < m_skeleton->GetNumNodes(); ++nodeIndex)
        {
            Node* node = m_skeleton->GetNode(nodeIndex);
            node->RemoveAllChildNodes();
        }

        // Then build the child-parent relationship using the prebuild map.
        for (auto& pair : childParentMap)
        {
            Node* child = m_skeleton->FindNodeByName(pair.first);
            Node* parent = m_skeleton->FindNodeByName(pair.second);
            child->SetParentIndex(parent->GetNodeIndex());
            parent->AddChild(child->GetNodeIndex());
        }

        // Resize transform data because the actor nodes has been trimmed down.
        ResizeTransformData();

        //reset the motion extraction node index
        SetMotionExtractionNode(motionExtractionNode);
        FindBestMatchingMotionExtractionAxis();
    }

    void Actor::SetMeshAssetId(const AZ::Data::AssetId& assetId)
    {
        m_meshAssetId = assetId;
    }

    Node* Actor::FindMeshJoint(const AZ::Data::Asset<AZ::RPI::ModelLodAsset>& lodModelAsset) const
    {
        const AZStd::span<const AZ::RPI::ModelLodAsset::Mesh>& sourceMeshes = lodModelAsset->GetMeshes();

        // Use the first joint that we can find for any of the Atom sub meshes and use it as owner of our mesh.
        for (const AZ::RPI::ModelLodAsset::Mesh& sourceMesh : sourceMeshes)
        {
            const AZ::Name& meshName = sourceMesh.GetName();
            Node* joint = FindJointByMeshName(meshName.GetStringView());
            if (joint)
            {
                return joint;
            }
        }

        // In case neither of the mesh joints are present in the actor, just use the root node as fallback.
        AZ_Assert(m_skeleton->GetNode(0), "Actor needs to have at least a single joint.");
        return m_skeleton->GetNode(0);
    }

    void Actor::ConstructMeshes()
    {
        AZ_Assert(m_meshAsset.IsReady(), "Mesh asset should be fully loaded and ready.");

        AZStd::vector<LODLevel>& lodLevels = m_meshLodData.m_lodLevels;
        const AZStd::span<const AZ::Data::Asset<AZ::RPI::ModelLodAsset>>& lodAssets = m_meshAsset->GetLodAssets();
        const size_t numLODLevels = lodAssets.size();

        lodLevels.clear();
        SetNumLODLevels(numLODLevels, /*adjustMorphSetup=*/false);
        const size_t numNodes = m_skeleton->GetNumNodes();

        for (size_t lodLevel = 0; lodLevel < numLODLevels; ++lodLevel)
        {
            const AZ::Data::Asset<AZ::RPI::ModelLodAsset>& lodAsset = lodAssets[lodLevel];

            lodLevels[lodLevel].m_nodeInfos.resize(numNodes);

            // Create a single mesh for the actor per LOD.
            Mesh* mesh = Mesh::CreateFromModelLod(lodAsset, m_skinToSkeletonIndexMap);

            // Find an owning joint for the mesh.
            Node* meshJoint = FindMeshJoint(lodAsset);
            if (!meshJoint)
            {
                AZ_Error("EMotionFX", false, "Cannot find mesh joint. Skipping to add meshes to the actor.");
                continue;
            }

            const size_t jointIndex = meshJoint->GetNodeIndex();
            NodeLODInfo& jointInfo = lodLevels[lodLevel].m_nodeInfos[jointIndex];

            jointInfo.m_mesh = mesh;

            if (!jointInfo.m_stack)
            {
                jointInfo.m_stack = MeshDeformerStack::Create(mesh);
            }

            // Add the skinning deformers
            const size_t numLayers = mesh->GetNumSharedVertexAttributeLayers();
            for (size_t layerNr = 0; layerNr < numLayers; ++layerNr)
            {
                EMotionFX::VertexAttributeLayer* vertexAttributeLayer = mesh->GetSharedVertexAttributeLayer(layerNr);
                if (vertexAttributeLayer->GetType() != EMotionFX::SkinningInfoVertexAttributeLayer::TYPE_ID)
                {
                    continue;
                }

                const uint16 numLocalJoints = mesh->GetNumUniqueJoints();
                const uint16 highestJointIndex = mesh->GetHighestJointIndex();

                // The information about if we want to use dual quat skinning is baked into the mesh chunk and we don't have access to that
                // anymore. Default to dual quat skinning.
                const bool dualQuatSkinning = true;
                if (dualQuatSkinning)
                {
                    DualQuatSkinDeformer* skinDeformer = DualQuatSkinDeformer::Create(mesh);
                    jointInfo.m_stack->AddDeformer(skinDeformer);
                    skinDeformer->ReserveLocalBones(aznumeric_caster(numLocalJoints));
                    skinDeformer->Reinitialize(this, meshJoint, static_cast<uint32>(lodLevel), highestJointIndex);
                }
                else
                {
                    SoftSkinDeformer* skinDeformer = GetSoftSkinManager().CreateDeformer(mesh);
                    jointInfo.m_stack->AddDeformer(skinDeformer);
                    skinDeformer->ReserveLocalBones(aznumeric_caster(numLocalJoints)); // pre-alloc data to prevent reallocs
                    skinDeformer->Reinitialize(this, meshJoint, static_cast<uint32>(lodLevel), highestJointIndex);
                }
            }

        }
    }

    Node* Actor::FindJointByMeshName(const AZStd::string_view meshName) const
    {
        Node* joint = m_skeleton->FindNodeByName(meshName.data());
        if (!joint)
        {
            // When mesh merging in the model builder is enabled, the name of the mesh is the concatenated version
            // of all the merged meshes with a plus symbol used as delimiter. Try to find any of the merged mesh joint
            // and use the first one to add the mesh to it.
            AZStd::vector<AZStd::string> tokens;
            AZ::StringFunc::Tokenize(meshName, tokens, '+');
            for (const AZStd::string& token : tokens)
            {
                joint = m_skeleton->FindNodeByName(token);
                if (joint)
                {
                    break;
                }
            }
        }

        return joint;
    }

    AZStd::unordered_map<AZ::u16, AZ::u16> Actor::ConstructSkinToSkeletonIndexMap(const AZ::Data::Asset<AZ::RPI::SkinMetaAsset>& skinMetaAsset)
    {
        AZ_Assert(skinMetaAsset && skinMetaAsset.IsReady(), "Cannot construct skin to skeleton index mapping. Skin meta asset needs to be loaded and ready.");

        // Build an atom index to emfx index map
        AZStd::unordered_map<AZ::u16, AZ::u16> result;
        for (const auto& pair : skinMetaAsset->GetJointNameToIndexMap())
        {
            const Node* node = m_skeleton->FindNodeByName(pair.first.c_str());
            if (!node)
            {
                AZ_Assert(node, "Cannot find joint named %s in the skeleton while it is used by the skin.", pair.first.c_str());
                continue;
            }
            result.emplace(pair.second, aznumeric_caster(node->GetNodeIndex()));
        }

        return result;
    }

    void Actor::ConstructMorphTargets()
    {
        AZ_Assert(m_meshAsset.IsReady() && m_morphTargetMetaAsset.IsReady(),
            "Mesh as well as morph target meta asset asset should be fully loaded and ready.");
        AZStd::vector<LODLevel>& lodLevels = m_meshLodData.m_lodLevels;
        const AZStd::span<const AZ::Data::Asset<AZ::RPI::ModelLodAsset>>& lodAssets = m_meshAsset->GetLodAssets();
        const size_t numLODLevels = lodAssets.size();

        AZ_Assert(m_morphSetups.size() == numLODLevels, "There needs to be a morph setup for every single LOD level.");

        for (size_t lodLevel = 0; lodLevel < numLODLevels; ++lodLevel)
        {
            const AZ::Data::Asset<AZ::RPI::ModelLodAsset>& lodAsset = lodAssets[lodLevel];
            const AZStd::span<const AZ::RPI::ModelLodAsset::Mesh>& sourceMeshes = lodAsset->GetMeshes();

            MorphSetup* morphSetup = m_morphSetups[static_cast<uint32>(lodLevel)];
            if (!morphSetup)
            {
                continue;
            }

            // Find the owning joint for the mesh.
            Node* meshJoint = FindMeshJoint(lodAsset);
            if (!meshJoint)
            {
                AZ_Error("EMotionFX", false, "Cannot find mesh joint. Skipping to add meshes to the actor.");
                continue;
            }

            const size_t jointIndex = meshJoint->GetNodeIndex();
            NodeLODInfo& jointInfo = lodLevels[lodLevel].m_nodeInfos[jointIndex];
            Mesh* mesh = jointInfo.m_mesh;

            if (!jointInfo.m_stack)
            {
                jointInfo.m_stack = MeshDeformerStack::Create(mesh);
            }

            // Add the morph deformer to the mesh deformer stack (in case there is none yet).
            MorphMeshDeformer* morphTargetDeformer = (MorphMeshDeformer*)jointInfo.m_stack->FindDeformerByType(MorphMeshDeformer::TYPE_ID);
            if (!morphTargetDeformer)
            {
                morphTargetDeformer = MorphMeshDeformer::Create(mesh);
                // Add insert the deformer at the first position to make sure we apply morph targets before skinning.
                jointInfo.m_stack->InsertDeformer(/*deformerPosition=*/0, morphTargetDeformer);
            }

            // The lod has shared buffers that combine the data from each submesh. In case any of the submeshes has a
            // morph target buffer view we can access the entire morph target buffer via the buffer asset.
            AZStd::span<const uint8_t> morphTargetDeltaView;
            for (const AZ::RPI::ModelLodAsset::Mesh& sourceMesh : sourceMeshes)
            {
                if (const auto* bufferAssetView = sourceMesh.GetSemanticBufferAssetView(AZ::Name("MORPHTARGET_VERTEXDELTAS")))
                {
                    if (const auto* bufferAsset = bufferAssetView->GetBufferAsset().Get())
                    {
                        // The buffer of the view is the buffer of the whole LOD, not just the source mesh.
                        morphTargetDeltaView = bufferAsset->GetBuffer();
                        break;
                    }
                }
            }

            AZ_Assert(morphTargetDeltaView.data(), "Unable to find MORPHTARGET_VERTEXDELTAS buffer");
            const AZ::RPI::PackedCompressedMorphTargetDelta* vertexDeltas = reinterpret_cast<const AZ::RPI::PackedCompressedMorphTargetDelta*>(morphTargetDeltaView.data());

            const size_t numMorphTargets = morphSetup->GetNumMorphTargets();
            for (size_t mtIndex = 0; mtIndex < numMorphTargets; ++mtIndex)
            {
                MorphTargetStandard* morphTarget = static_cast<MorphTargetStandard*>(morphSetup->GetMorphTarget(mtIndex));

                // Remove all previously added deform datas for the given joint as we set a new mesh.
                morphTarget->RemoveAllDeformDatasFor(meshJoint);

                const AZStd::vector<AZ::RPI::MorphTargetMetaAsset::MorphTarget>& metaDatas = m_morphTargetMetaAsset->GetMorphTargets();
                for (const auto& metaData : metaDatas)
                {
                    if (metaData.m_morphTargetName == morphTarget->GetNameString())
                    {
                        const AZ::u32 numDeformedVertices = metaData.m_numVertices;
                        MorphTargetStandard::DeformData* deformData = aznew MorphTargetStandard::DeformData(jointIndex, numDeformedVertices);

                        // Set the compression/quantization range for the positions.
                        deformData->m_minValue = metaData.m_minPositionDelta;
                        deformData->m_maxValue = metaData.m_maxPositionDelta;

                        for (AZ::u32 deformVtx = 0; deformVtx < numDeformedVertices; ++deformVtx)
                        {
                            // Get the index into the morph delta buffer
                            const AZ::u32 vertexIndex = metaData.m_startIndex + deformVtx;

                            // Unpack the delta
                            const AZ::RPI::PackedCompressedMorphTargetDelta& packedCompressedDelta = vertexDeltas[vertexIndex];
                            AZ::RPI::CompressedMorphTargetDelta unpackedCompressedDelta = AZ::RPI::UnpackMorphTargetDelta(packedCompressedDelta);

                            // Set the EMotionFX deform data from the CmopressedMorphTargetDelta
                            deformData->m_deltas[deformVtx].m_vertexNr = unpackedCompressedDelta.m_morphedVertexIndex;

                            deformData->m_deltas[deformVtx].m_position = MCore::Compressed16BitVector3(
                                unpackedCompressedDelta.m_positionX,
                                unpackedCompressedDelta.m_positionY,
                                unpackedCompressedDelta.m_positionZ);

                            deformData->m_deltas[deformVtx].m_normal = MCore::Compressed8BitVector3(
                                unpackedCompressedDelta.m_normalX,
                                unpackedCompressedDelta.m_normalY,
                                unpackedCompressedDelta.m_normalZ);

                            deformData->m_deltas[deformVtx].m_tangent = MCore::Compressed8BitVector3(
                                unpackedCompressedDelta.m_tangentX,
                                unpackedCompressedDelta.m_tangentY,
                                unpackedCompressedDelta.m_tangentZ);

                            deformData->m_deltas[deformVtx].m_bitangent = MCore::Compressed8BitVector3(
                                unpackedCompressedDelta.m_bitangentX,
                                unpackedCompressedDelta.m_bitangentY,
                                unpackedCompressedDelta.m_bitangentZ);
                        }

                        morphTarget->AddDeformData(deformData);
                    }
                }
            }

            // Sync the deformer passes with the morph target deform datas.
            morphTargetDeformer->Reinitialize(this, meshJoint, static_cast<uint32>(lodLevel));
        }
    }
} // namespace EMotionFX
