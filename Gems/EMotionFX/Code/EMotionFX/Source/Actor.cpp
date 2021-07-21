/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
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
#include <EMotionFX/Source/Material.h>
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
#include <MCore/Source/OBB.h>

#include <Atom/RPI.Reflect/Model/MorphTargetDelta.h>

namespace EMotionFX
{
    AZ_CLASS_ALLOCATOR_IMPL(Actor, ActorAllocator, 0)

    Actor::NodeInfo::NodeInfo()
    {
        mOBB.Init();
    }

    Actor::LODLevel::LODLevel()
    {
    }

    Actor::MeshLODData::MeshLODData()
    {
        // Create the default LOD level
        m_lodLevels.push_back({});
    }

    Actor::NodeLODInfo::NodeLODInfo()
    {
        mMesh   = nullptr;
        mStack  = nullptr;
    }

    Actor::NodeLODInfo::~NodeLODInfo()
    {
        MCore::Destroy(mMesh);
        MCore::Destroy(mStack);
    }

    //----------------------------------------------------

    Actor::Actor(const char* name)
    {
        SetName(name);

        // setup the array memory categories
        mMaterials.SetMemoryCategory(EMFX_MEMCATEGORY_ACTORS);
        mDependencies.SetMemoryCategory(EMFX_MEMCATEGORY_ACTORS);
        mMorphSetups.SetMemoryCategory(EMFX_MEMCATEGORY_ACTORS);

        mSkeleton = Skeleton::Create();

        mMotionExtractionNode       = MCORE_INVALIDINDEX32;
        mRetargetRootNode           = MCORE_INVALIDINDEX32;
        mThreadIndex                = 0;
        mCustomData                 = nullptr;
        mID                         = MCore::GetIDGenerator().GenerateID();
        mUnitType                   = GetEMotionFX().GetUnitType();
        mFileUnitType               = mUnitType;

        mUsedForVisualization       = false;
        mDirtyFlag                  = false;

        m_physicsSetup              = AZStd::make_shared<PhysicsSetup>();
        m_simulatedObjectSetup      = AZStd::make_shared<SimulatedObjectSetup>(this);

        m_optimizeSkeleton          = false;
#if defined(EMFX_DEVELOPMENT_BUILD)
        mIsOwnedByRuntime           = false;
#endif // EMFX_DEVELOPMENT_BUILD

        // make sure we have at least allocated the first LOD of materials and facial setups
        mMaterials.Reserve(4);  // reserve space for 4 lods
        mMorphSetups.Reserve(4); //
        mMaterials.AddEmpty();
        mMaterials[0].SetMemoryCategory(EMFX_MEMCATEGORY_ACTORS);
        mMorphSetups.Add(nullptr);

        GetEventManager().OnCreateActor(this);
        ActorNotificationBus::Broadcast(&ActorNotificationBus::Events::OnActorCreated, this);
    }

    Actor::~Actor()
    {
        ActorNotificationBus::Broadcast(&ActorNotificationBus::Events::OnActorDestroyed, this);
        GetEventManager().OnDeleteActor(this);

        mNodeMirrorInfos.Clear(true);

        RemoveAllMaterials();
        RemoveAllMorphSetups();
        RemoveAllNodeGroups();

        mInvBindPoseTransforms.clear();

        MCore::Destroy(mSkeleton);
    }

    // creates a clone of the actor (a copy).
    // does NOT copy the motions and motion tree
    AZStd::unique_ptr<Actor> Actor::Clone() const
    {
        // create the new actor and set the name and filename
        AZStd::unique_ptr<Actor> result = AZStd::make_unique<Actor>(GetName());
        result->SetFileName(GetFileName());

        // copy the actor attributes
        result->mMotionExtractionNode   = mMotionExtractionNode;
        result->mUnitType               = mUnitType;
        result->mFileUnitType           = mFileUnitType;
        result->mStaticAABB             = mStaticAABB;
        result->mRetargetRootNode       = mRetargetRootNode;
        result->mInvBindPoseTransforms  = mInvBindPoseTransforms;
        result->m_optimizeSkeleton      = m_optimizeSkeleton;
        result->m_skinToSkeletonIndexMap = m_skinToSkeletonIndexMap;

        result->RecursiveAddDependencies(this);

        // clone all nodes groups
        for (uint32 i = 0; i < mNodeGroups.GetLength(); ++i)
        {
            result->AddNodeGroup(aznew NodeGroup(*mNodeGroups[i]));
        }

        // clone the materials
        result->mMaterials.Resize(mMaterials.GetLength());
        for (uint32 i = 0; i < mMaterials.GetLength(); ++i)
        {
            // get the number of materials in the current LOD
            const uint32 numMaterials = mMaterials[i].GetLength();
            result->mMaterials[i].Reserve(numMaterials);
            for (uint32 m = 0; m < numMaterials; ++m)
            {
                // retrieve the current material
                Material* material = mMaterials[i][m];

                // clone the material
                Material* clone = material->Clone();

                // add the cloned material to the cloned actor
                result->AddMaterial(i, clone);
            }
        }

        // clone the skeleton
        MCore::Destroy(result->mSkeleton);
        result->mSkeleton = mSkeleton->Clone();

        // clone lod data
        result->mNodeInfos = mNodeInfos;
        const uint32 numNodes = mSkeleton->GetNumNodes();

        const size_t numLodLevels = m_meshLodData.m_lodLevels.size();
        MeshLODData& resultMeshLodData = result->m_meshLodData;

        result->SetNumLODLevels(numLodLevels);
        for (size_t lodLevel = 0; lodLevel < numLodLevels; ++lodLevel)
        {
            const MCore::Array<NodeLODInfo>& nodeInfos = m_meshLodData.m_lodLevels[lodLevel].mNodeInfos;
            MCore::Array<NodeLODInfo>& resultNodeInfos = resultMeshLodData.m_lodLevels[lodLevel].mNodeInfos;

            resultNodeInfos.Resize(numNodes);
            for (uint32 n = 0; n < numNodes; ++n)
            {
                NodeLODInfo& resultNodeInfo = resultNodeInfos[n];
                const NodeLODInfo& sourceNodeInfo = nodeInfos[n];
                resultNodeInfo.mMesh = (sourceNodeInfo.mMesh) ? sourceNodeInfo.mMesh->Clone() : nullptr;
                resultNodeInfo.mStack = (sourceNodeInfo.mStack) ? sourceNodeInfo.mStack->Clone(resultNodeInfo.mMesh) : nullptr;
            }
        }

        // clone the morph setups
        result->mMorphSetups.Resize(mMorphSetups.GetLength());
        for (uint32 i = 0; i < mMorphSetups.GetLength(); ++i)
        {
            if (mMorphSetups[i])
            {
                result->SetMorphSetup(i, mMorphSetups[i]->Clone());
            }
            else
            {
                result->SetMorphSetup(i, nullptr);
            }
        }

        // make sure the number of root nodes is still the same
        MCORE_ASSERT(result->GetSkeleton()->GetNumRootNodes() == mSkeleton->GetNumRootNodes());

        // copy the transform data
        result->CopyTransformsFrom(this);

        result->mNodeMirrorInfos = mNodeMirrorInfos;
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
        const uint32 numNodes = mSkeleton->GetNumNodes();
        mNodeMirrorInfos.Resize(numNodes);

        // init the data
        for (uint32 i = 0; i < numNodes; ++i)
        {
            mNodeMirrorInfos[i].mSourceNode = static_cast<uint16>(i);
            mNodeMirrorInfos[i].mAxis       = MCORE_INVALIDINDEX8;
            mNodeMirrorInfos[i].mFlags      = 0;
        }
    }

    // remove the node mirror info
    void Actor::RemoveNodeMirrorInfos()
    {
        mNodeMirrorInfos.Clear(true);
    }


    // check if we have our axes detected
    bool Actor::GetHasMirrorAxesDetected() const
    {
        if (mNodeMirrorInfos.GetLength() == 0)
        {
            return false;
        }

        for (uint32 i = 0; i < mNodeMirrorInfos.GetLength(); ++i)
        {
            if (mNodeMirrorInfos[i].mAxis == MCORE_INVALIDINDEX8)
            {
                return false;
            }
        }

        return true;
    }


    // removes all materials from the actor
    void Actor::RemoveAllMaterials()
    {
        // for all LODs
        for (uint32 i = 0; i < mMaterials.GetLength(); ++i)
        {
            // delete all materials
            const uint32 numMats = mMaterials[i].GetLength();
            for (uint32 m = 0; m < numMats; ++m)
            {
                mMaterials[i][m]->Destroy();
            }
        }

        mMaterials.Clear();
    }


    // add a LOD level and copy the data from the last LOD level to the new one
    void Actor::AddLODLevel(bool copyFromLastLODLevel)
    {
        AZStd::vector<LODLevel>& lodLevels = m_meshLodData.m_lodLevels;

        lodLevels.emplace_back();
        LODLevel& newLOD = lodLevels.back();
        const uint32 numNodes = mSkeleton->GetNumNodes();
        newLOD.mNodeInfos.SetMemoryCategory(EMFX_MEMCATEGORY_ACTORS);
        newLOD.mNodeInfos.Resize(numNodes);

        const size_t numLODs    = lodLevels.size();
        const size_t lodIndex   = numLODs - 1;

        // get the number of nodes, iterate through them, create a new LOD level and copy over the meshes from the last LOD level
        for (size_t i = 0; i < numNodes; ++i)
        {
            NodeLODInfo& newLODInfo = lodLevels[lodIndex].mNodeInfos[i];
            if (copyFromLastLODLevel && lodIndex > 0)
            {
                const NodeLODInfo& prevLODInfo = lodLevels[lodIndex - 1].mNodeInfos[i];
                newLODInfo.mMesh        = (prevLODInfo.mMesh)        ? prevLODInfo.mMesh->Clone()                    : nullptr;
                newLODInfo.mStack       = (prevLODInfo.mStack)       ? prevLODInfo.mStack->Clone(newLODInfo.mMesh)   : nullptr;
            }
            else
            {
                newLODInfo.mMesh        = nullptr;
                newLODInfo.mStack       = nullptr;
            }
        }

        // create a new material array for the new LOD level
        mMaterials.Resize(lodLevels.size());
        mMaterials[lodIndex].SetMemoryCategory(EMFX_MEMCATEGORY_ACTORS);

        // create an empty morph setup for the new LOD level
        mMorphSetups.Add(nullptr);

        // copy data from the previous LOD level if wanted
        if (copyFromLastLODLevel && numLODs > 0)
        {
            CopyLODLevel(this, lodIndex - 1, numLODs - 1, true);
        }
    }

    // insert a LOD level at a given position
    void Actor::InsertLODLevel(uint32 insertAt)
    {
        AZStd::vector<LODLevel>& lodLevels = m_meshLodData.m_lodLevels;

        lodLevels.insert(lodLevels.begin()+insertAt, {});
        LODLevel& newLOD = lodLevels[insertAt];
        const uint32 lodIndex   = insertAt;
        const uint32 numNodes   = mSkeleton->GetNumNodes();
        newLOD.mNodeInfos.SetMemoryCategory(EMFX_MEMCATEGORY_ACTORS);
        newLOD.mNodeInfos.Resize(numNodes);

        // get the number of nodes, iterate through them, create a new LOD level and copy over the meshes from the last LOD level
        for (uint32 i = 0; i < numNodes; ++i)
        {
            NodeLODInfo& lodInfo    = lodLevels[lodIndex].mNodeInfos[i];
            lodInfo.mMesh           = nullptr;
            lodInfo.mStack          = nullptr;
        }

        // create a new material array for the new LOD level
        mMaterials.Insert(insertAt);
        mMaterials[lodIndex].SetMemoryCategory(EMFX_MEMCATEGORY_ACTORS);

        // create an empty morph setup for the new LOD level
        mMorphSetups.Insert(insertAt, nullptr);
    }

    // replace existing LOD level with the current actor
    void Actor::CopyLODLevel(Actor* copyActor, uint32 copyLODLevel, uint32 replaceLODLevel, bool copySkeletalLODFlags)
    {
        AZStd::vector<LODLevel>& lodLevels = m_meshLodData.m_lodLevels;
        AZStd::vector<LODLevel>& copyLodLevels = copyActor->m_meshLodData.m_lodLevels;

        const LODLevel& sourceLOD = copyLodLevels[copyLODLevel];
        LODLevel& targetLOD = lodLevels[replaceLODLevel];

        const uint32 numNodes = mSkeleton->GetNumNodes();
        for (uint32 i = 0; i < numNodes; ++i)
        {
            Node* node     = mSkeleton->GetNode(i);
            Node* copyNode = copyActor->GetSkeleton()->FindNodeByID(node->GetID());

            if (copyNode == nullptr)
            {
                MCore::LogWarning("Actor::CopyLODLevel() - Failed to find node '%s' in the actor we want to copy from.", node->GetName());
            }

            const NodeLODInfo& sourceNodeInfo = sourceLOD.mNodeInfos[ copyNode->GetNodeIndex() ];
            NodeLODInfo& targetNodeInfo = targetLOD.mNodeInfos[i];

            // first get rid of existing data
            MCore::Destroy(targetNodeInfo.mMesh);
            targetNodeInfo.mMesh        = nullptr;
            MCore::Destroy(targetNodeInfo.mStack);
            targetNodeInfo.mStack       = nullptr;

            // if the node exists in both models
            if (copyNode)
            {
                // copy over the mesh and collision mesh
                if (sourceNodeInfo.mMesh)
                {
                    targetNodeInfo.mMesh = sourceNodeInfo.mMesh->Clone();
                }

                // handle the stacks
                if (sourceNodeInfo.mStack)
                {
                    targetNodeInfo.mStack = sourceNodeInfo.mStack->Clone(targetNodeInfo.mMesh);
                }

                // copy the skeletal LOD flag
                if (copySkeletalLODFlags)
                {
                    node->SetSkeletalLODStatus(replaceLODLevel, copyNode->GetSkeletalLODStatus(copyLODLevel));
                }
            }
        }

        // copy the materials
        const uint32 numMaterials = copyActor->GetNumMaterials(copyLODLevel);
        for (uint32 i = 0; i < mMaterials[replaceLODLevel].GetLength(); ++i)
        {
            mMaterials[replaceLODLevel][i]->Destroy();
        }
        mMaterials[replaceLODLevel].Clear();
        mMaterials[replaceLODLevel].Reserve(numMaterials);
        for (uint32 i = 0; i < numMaterials; ++i)
        {
            AddMaterial(replaceLODLevel, copyActor->GetMaterial(copyLODLevel, i)->Clone());
        }

        // copy the morph setup
        if (mMorphSetups[replaceLODLevel])
        {
            mMorphSetups[replaceLODLevel]->Destroy();
        }

        if (copyActor->GetMorphSetup(copyLODLevel))
        {
            mMorphSetups[replaceLODLevel] = copyActor->GetMorphSetup(copyLODLevel)->Clone();
        }
        else
        {
            mMorphSetups[replaceLODLevel] = nullptr;
        }
    }

    // preallocate memory for all LOD levels
    void Actor::SetNumLODLevels(uint32 numLODs, bool adjustMorphSetup)
    {
        m_meshLodData.m_lodLevels.resize(numLODs);

        // reserve space for the materials
        mMaterials.Resize(numLODs);
        for (uint32 i = 0; i < numLODs; ++i)
        {
            mMaterials[i].SetMemoryCategory(EMFX_MEMCATEGORY_ACTORS);
        }

        if (adjustMorphSetup)
        {
            mMorphSetups.Resize(numLODs);
            for (uint32 i = 0; i < numLODs; ++i)
            {
                mMorphSetups[i] = nullptr;
            }
        }
    }

    // removes all node meshes and stacks
    void Actor::RemoveAllNodeMeshes()
    {
        const uint32 numNodes = mSkeleton->GetNumNodes();

        AZStd::vector<LODLevel>& lodLevels = m_meshLodData.m_lodLevels;
        for (LODLevel& lodLevel : lodLevels)
        {
            for (uint32 i = 0; i < numNodes; ++i)
            {
                NodeLODInfo& info = lodLevel.mNodeInfos[i];
                MCore::Destroy(info.mMesh);
                info.mMesh = nullptr;
                MCore::Destroy(info.mStack);
                info.mStack = nullptr;
            }
        }
    }


    void Actor::CalcMeshTotals(uint32 lodLevel, uint32* outNumPolygons, uint32* outNumVertices, uint32* outNumIndices) const
    {
        uint32 totalPolys   = 0;
        uint32 totalVerts   = 0;
        uint32 totalIndices = 0;

        const uint32 numNodes = mSkeleton->GetNumNodes();
        for (uint32 i = 0; i < numNodes; ++i)
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


    void Actor::CalcStaticMeshTotals(uint32 lodLevel, uint32* outNumVertices, uint32* outNumIndices)
    {
        // the totals
        uint32 totalVerts   = 0;
        uint32 totalIndices = 0;

        // for all nodes
        const uint32 numNodes = mSkeleton->GetNumNodes();
        for (uint32 i = 0; i < numNodes; ++i)
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


    void Actor::CalcDeformableMeshTotals(uint32 lodLevel, uint32* outNumVertices, uint32* outNumIndices)
    {
        // the totals
        uint32 totalVerts   = 0;
        uint32 totalIndices = 0;

        // for all nodes
        const uint32 numNodes = mSkeleton->GetNumNodes();
        for (uint32 i = 0; i < numNodes; ++i)
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


    uint32 Actor::CalcMaxNumInfluences(uint32 lodLevel) const
    {
        uint32 maxInfluences = 0;

        const uint32 numNodes = mSkeleton->GetNumNodes();
        for (uint32 i = 0; i < numNodes; ++i)
        {
            Mesh* mesh = GetMesh(lodLevel, i);
            if (!mesh)
            {
                continue;
            }

            maxInfluences = MCore::Max<uint32>(maxInfluences, mesh->CalcMaxNumInfluences());
        }

        return maxInfluences;
    }


    // verify if the skinning will look correctly in the given geometry LOD for a given skeletal LOD level
    void Actor::VerifySkinning(MCore::Array<uint8>& conflictNodeFlags, uint32 skeletalLODLevel, uint32 geometryLODLevel)
    {
        uint32 n;

        // get the number of nodes
        const uint32 numNodes = mSkeleton->GetNumNodes();

        // check if the conflict node flag array's size is set to the number of nodes inside the actor
        if (conflictNodeFlags.GetLength() != numNodes)
        {
            conflictNodeFlags.Resize(numNodes);
        }

        // reset the conflict node array to zero which means we don't have any conflicting nodes yet
        MCore::MemSet(conflictNodeFlags.GetPtr(), 0, numNodes * sizeof(int8));

        // iterate over the all nodes in the actor
        for (n = 0; n < numNodes; ++n)
        {
            // get the current node and the pointer to the mesh for the given lod level
            Node* node = mSkeleton->GetNode(n);
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


    uint32 Actor::CalcMaxNumInfluences(uint32 lodLevel, AZStd::vector<uint32>& outVertexCounts) const
    {
        uint32 maxInfluences = 0;

        // Reset the values.
        outVertexCounts.resize(CalcMaxNumInfluences(lodLevel) + 1);
        for (size_t k = 0; k < outVertexCounts.size(); ++k)
        {
            outVertexCounts[k] = 0;
        }

        // Get the vertex counts for the influences. (e.g. 500 vertices have 1 skinning influence, 300 vertices have 2 skinning influences etc.)
        AZStd::vector<uint32> meshVertexCounts;
        const uint32 numNodes = GetNumNodes();
        for (uint32 i = 0; i < numNodes; ++i)
        {
            Mesh* mesh = GetMesh(lodLevel, i);
            if (!mesh)
            {
                continue;
            }

            const uint32 meshMaxInfluences = mesh->CalcMaxNumInfluences(meshVertexCounts);
            maxInfluences = MCore::Max<uint32>(maxInfluences, meshMaxInfluences);

            for (size_t j = 0; j < meshVertexCounts.size(); ++j)
            {
                outVertexCounts[j] += meshVertexCounts[j];
            }
        }

        return maxInfluences;
    }

    // check if there is any mesh available
    bool Actor::CheckIfHasMeshes(uint32 lodLevel) const
    {
        // check if any of the nodes has a mesh
        const uint32 numNodes = mSkeleton->GetNumNodes();
        for (uint32 i = 0; i < numNodes; ++i)
        {
            if (GetMesh(lodLevel, i))
            {
                return true;
            }
        }

        // aaaah, no meshes found
        return false;
    }


    bool Actor::CheckIfHasSkinnedMeshes(AZ::u32 lodLevel) const
    {
        const AZ::u32 numNodes = mSkeleton->GetNumNodes();
        for (AZ::u32 i = 0; i < numNodes; ++i)
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
        uint32 i;

        // get the number of lod levels
        const uint32 numLODs = GetNumLODLevels();

        // for all LODs, get rid of all the morph setups for each geometry LOD
        for (i = 0; i < mMorphSetups.GetLength(); ++i)
        {
            if (mMorphSetups[i])
            {
                mMorphSetups[i]->Destroy();
            }

            mMorphSetups[i] = nullptr;
        }

        // remove all modifiers from the stacks for each lod in all nodes
        if (deleteMeshDeformers)
        {
            // for all nodes
            const uint32 numNodes = mSkeleton->GetNumNodes();
            for (i = 0; i < numNodes; ++i)
            {
                // process all LOD levels
                for (uint32 lod = 0; lod < numLODs; ++lod)
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



    // check if the material is used by the given mesh
    bool Actor::CheckIfIsMaterialUsed(Mesh* mesh, uint32 materialIndex) const
    {
        // check if the mesh is valid
        if (mesh == nullptr)
        {
            return false;
        }

        // iterate through the submeshes
        const uint32 numSubMeshes = mesh->GetNumSubMeshes();
        for (uint32 s = 0; s < numSubMeshes; ++s)
        {
            // if the submesh material index is the same as the material index we search for, then it is being used
            if (mesh->GetSubMesh(s)->GetMaterial() == materialIndex)
            {
                return true;
            }
        }

        return false;
    }


    // check if the material is used by a mesh of this actor
    bool Actor::CheckIfIsMaterialUsed(uint32 lodLevel, uint32 index) const
    {
        // iterate through all nodes of the actor and check its meshes
        const uint32 numNodes = mSkeleton->GetNumNodes();
        for (uint32 i = 0; i < numNodes; ++i)
        {
            // if the mesh is in LOD range check if it uses the material
            if (CheckIfIsMaterialUsed(GetMesh(lodLevel, i), index))
            {
                return true;
            }

            // same for the collision mesh
            //if (CheckIfIsMaterialUsed( GetCollisionMesh(lodLevel, i), index ))
            //return true;
        }

        // return false, this means that no mesh uses the given material
        return false;
    }


    // remove the given material and reassign all material numbers of the submeshes
    void Actor::RemoveMaterial(uint32 lodLevel, uint32 index)
    {
        MCORE_ASSERT(lodLevel < mMaterials.GetLength());

        // first of all remove the given material
        mMaterials[lodLevel][index]->Destroy();
        mMaterials[lodLevel].Remove(index);
    }


    // try to find the motion extraction node automatically
    Node* Actor::FindBestMotionExtractionNode() const
    {
        Node* result = nullptr;

        // the maximum number of children of a root node, the node with the most children
        // will become our repositioning node
        uint32 maxNumChilds = 0;

        // traverse through all root nodes
        const uint32 numRootNodes = mSkeleton->GetNumRootNodes();
        for (uint32 i = 0; i < numRootNodes; ++i)
        {
            // get the given root node from the actor
            Node* rootNode = mSkeleton->GetNode(mSkeleton->GetRootNodeIndex(i));

            // get the number of child nodes recursively
            const uint32 numChildNodes = rootNode->GetNumChildNodesRecursive();

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
    void Actor::ExtractBoneList(uint32 lodLevel, MCore::Array<uint32>* outBoneList) const
    {
        // clear the existing items
        outBoneList->Clear();

        // for all nodes
        const uint32 numNodes = mSkeleton->GetNumNodes();
        for (uint32 n = 0; n < numNodes; ++n)
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
                const uint32 numInfluences = aznumeric_cast<uint32>(skinningLayer->GetNumInfluences(v));
                for (uint32 i = 0; i < numInfluences; ++i)
                {
                    // get the node number of the bone
                    uint32 nodeNr = skinningLayer->GetInfluence(v, i)->GetNodeNr();

                    // check if it is already in the bone list, if not, add it
                    if (outBoneList->Contains(nodeNr) == false)
                    {
                        outBoneList->Add(nodeNr);
                    }
                }
            }
        }
    }


    // recursively add dependencies
    void Actor::RecursiveAddDependencies(const Actor* actor)
    {
        // process all dependencies of the given actor
        const uint32 numDependencies = actor->GetNumDependencies();
        for (uint32 i = 0; i < numDependencies; ++i)
        {
            // add it to the actor instance
            mDependencies.Add(*actor->GetDependency(i));

            // recursive into the actor we are dependent on
            RecursiveAddDependencies(actor->GetDependency(i)->mActor);
        }
    }

    // update the bounding volumes
    void Actor::UpdateNodeBindPoseOBBs(uint32 lodLevel)
    {
        // for all nodes
        const uint32 numNodes = mSkeleton->GetNumNodes();
        for (uint32 i = 0; i < numNodes; ++i)
        {
            CalcOBBFromBindPose(lodLevel, i);
        }
    }


    // remove all node groups
    void Actor::RemoveAllNodeGroups()
    {
        const uint32 numGroups = mNodeGroups.GetLength();
        for (uint32 i = 0; i < numGroups; ++i)
        {
            mNodeGroups[i]->Destroy();
        }
        mNodeGroups.Clear();
    }


    // try to find a match for a given node with a given name
    // for example find "Bip01 L Hand" for node "Bip01 R Hand"
    uint16 Actor::FindBestMatchForNode(const char* nodeName, const char* subStringA, const char* subStringB, bool firstPass) const
    {
        char newString[1024];
        AZStd::string nameA;
        AZStd::string nameB;

        // search through all nodes to find the best match
        const uint32 numNodes = mSkeleton->GetNumNodes();
        for (uint32 n = 0; n < numNodes; ++n)
        {
            // get the node name
            const char* name = mSkeleton->GetNode(n)->GetName();

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
        const uint32 sourceNodeIndex = mSkeleton->FindNodeByNameNoCase(sourceNodeName)->GetNodeIndex();
        if (sourceNodeIndex == MCORE_INVALIDINDEX32)
        {
            return false;
        }

        // find the dest node index
        const uint32 destNodeIndex = mSkeleton->FindNodeByNameNoCase(destNodeName)->GetNodeIndex();
        if (destNodeIndex == MCORE_INVALIDINDEX32)
        {
            return false;
        }

        // allocate the data if we haven't already
        if (mNodeMirrorInfos.GetLength() == 0)
        {
            AllocateNodeMirrorInfos();
        }

        // apply the mapping
        mNodeMirrorInfos[ destNodeIndex   ].mSourceNode = static_cast<uint16>(sourceNodeIndex);
        mNodeMirrorInfos[ sourceNodeIndex ].mSourceNode = static_cast<uint16>(destNodeIndex);

        // we succeeded, because both source and dest have been found
        return true;
    }


    // map two nodes for mirroring
    bool Actor::MapNodeMotionSource(uint16 sourceNodeIndex, uint16 targetNodeIndex)
    {
        // allocate the data if we haven't already
        if (mNodeMirrorInfos.GetLength() == 0)
        {
            AllocateNodeMirrorInfos();
        }

        // apply the mapping
        mNodeMirrorInfos[ targetNodeIndex   ].mSourceNode   = static_cast<uint16>(sourceNodeIndex);
        mNodeMirrorInfos[ sourceNodeIndex ].mSourceNode     = static_cast<uint16>(targetNodeIndex);

        // we succeeded, because both source and dest have been found
        return true;
    }



    // match the node motion sources
    // substrings could be "Left " and "Right " to map the nodes "Left Hand" and "Right Hand" to eachother
    void Actor::MatchNodeMotionSources(const char* subStringA, const char* subStringB)
    {
        // try to map all nodes
        const uint32 numNodes = mSkeleton->GetNumNodes();
        for (uint32 i = 0; i < numNodes; ++i)
        {
            Node* node = mSkeleton->GetNode(i);

            // find the best match
            const uint16 bestIndex = FindBestMatchForNode(node->GetName(), subStringA, subStringB);

            // if a best match has been found
            if (bestIndex != MCORE_INVALIDINDEX16)
            {
                MCore::LogDetailedInfo("%s <---> %s", node->GetName(), mSkeleton->GetNode(bestIndex)->GetName());
                MapNodeMotionSource(node->GetName(), mSkeleton->GetNode(bestIndex)->GetName());
            }
        }
    }


    // set the name of the actor
    void Actor::SetName(const char* name)
    {
        mName = name;
    }


    // set the filename of the actor
    void Actor::SetFileName(const char* filename)
    {
        mFileName = filename;
    }


    // find the first active parent node in a given skeletal LOD
    uint32 Actor::FindFirstActiveParentBone(uint32 skeletalLOD, uint32 startNodeIndex) const
    {
        uint32 curNodeIndex = startNodeIndex;

        do
        {
            curNodeIndex = mSkeleton->GetNode(curNodeIndex)->GetParentIndex();
            if (curNodeIndex == MCORE_INVALIDINDEX32)
            {
                return curNodeIndex;
            }

            if (mSkeleton->GetNode(curNodeIndex)->GetSkeletalLODStatus(skeletalLOD))
            {
                return curNodeIndex;
            }
        } while (curNodeIndex != MCORE_INVALIDINDEX32);

        return MCORE_INVALIDINDEX32;
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
            const uint32 numNodes = mSkeleton->GetNumNodes();
            for (uint32 n = 0; n < numNodes; ++n)
            {
                Node* node = mSkeleton->GetNode(n);

                // check if this node has a mesh, if not we can skip it
                Mesh* mesh = GetMesh(geomLod, n);
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
                const uint32 numSubMeshes = mesh->GetNumSubMeshes();
                for (uint32 s = 0; s < numSubMeshes; ++s)
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
                            if (mSkeleton->GetNode(influence->GetNodeNr())->GetSkeletalLODStatus(geomLod) == false)
                            {
                                // find the first parent bone that is enabled in this LOD
                                const uint32 newNodeIndex = FindFirstActiveParentBone(geomLod, influence->GetNodeNr());
                                if (newNodeIndex == MCORE_INVALIDINDEX32)
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
                MeshDeformerStack* stack = GetMeshDeformerStack(geomLod, node->GetNodeIndex());
                if (stack)
                {
                    stack->ReinitializeDeformers(this, node, geomLod);
                }
            } // for all nodes
        }
    }


    // generate a path from the current node towards the root
    void Actor::GenerateUpdatePathToRoot(uint32 endNodeIndex, MCore::Array<uint32>& outPath) const
    {
        outPath.Clear(false);
        outPath.Reserve(32);

        // start at the end effector
        Node* currentNode = mSkeleton->GetNode(endNodeIndex);
        while (currentNode)
        {
            // add the current node to the update list
            outPath.Add(currentNode->GetNodeIndex());

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
            SetMotionExtractionNodeIndex(MCORE_INVALIDINDEX32);
        }
    }

    void Actor::SetMotionExtractionNodeIndex(uint32 nodeIndex)
    {
        mMotionExtractionNode = nodeIndex;
        ActorNotificationBus::Broadcast(&ActorNotificationBus::Events::OnMotionExtractionNodeChanged, this, GetMotionExtractionNode());
    }

    Node* Actor::GetMotionExtractionNode() const
    {
        if (mMotionExtractionNode != MCORE_INVALIDINDEX32 &&
            mMotionExtractionNode < mSkeleton->GetNumNodes())
        {
            return mSkeleton->GetNode(mMotionExtractionNode);
        }

        return nullptr;
    }

    void Actor::ReinitializeMeshDeformers()
    {
        const uint32 numLODLevels = GetNumLODLevels();
        const uint32 numNodes = mSkeleton->GetNumNodes();
        for (uint32 i = 0; i < numNodes; ++i)
        {
            Node* node = mSkeleton->GetNode(i);

            // iterate through all LOD levels
            for (uint32 lodLevel = 0; lodLevel < numLODLevels; ++lodLevel)
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
    void Actor::PostCreateInit(bool makeGeomLodsCompatibleWithSkeletalLODs, bool generateOBBs, bool convertUnitType)
    {
        if (mThreadIndex == MCORE_INVALIDINDEX32)
        {
            mThreadIndex = 0;
        }

        // calculate the inverse bind pose matrices
        const Pose* bindPose = GetBindPose();
        const uint32 numNodes = mSkeleton->GetNumNodes();
        mInvBindPoseTransforms.resize(numNodes);
        for (uint32 i = 0; i < numNodes; ++i)
        {
            mInvBindPoseTransforms[i] = bindPose->GetModelSpaceTransform(i).Inversed();
        }

        // make sure the skinning info doesn't use any disabled bones
        if (makeGeomLodsCompatibleWithSkeletalLODs)
        {
            MakeGeomLODsCompatibleWithSkeletalLODs();
        }

        // initialize the mesh deformers
        ReinitializeMeshDeformers();

        // make sure our world space bind pose is updated too
        if (mMorphSetups.GetLength() > 0 && mMorphSetups[0])
        {
            mSkeleton->GetBindPose()->ResizeNumMorphs(mMorphSetups[0]->GetNumMorphTargets());
        }
        mSkeleton->GetBindPose()->ForceUpdateFullModelSpacePose();
        mSkeleton->GetBindPose()->ZeroMorphWeights();

        if (generateOBBs)
        {
            UpdateNodeBindPoseOBBs(0);
        }

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

        // build the static axis aligned bounding box by creating an actor instance (needed to perform cpu skinning mesh deforms and mesh scaling etc)
        // then copy it over to the actor
        UpdateStaticAABB();

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
            ConstructMeshes();

            if (m_morphTargetMetaAsset.IsReady())
            {
                ConstructMorphTargets();
            }
            else
            {
                // Optional, not all actors have morph targets.
                const size_t numLODLevels = m_meshAsset->GetLodAssets().size();
                mMorphSetups.Resize(numLODLevels);
                for (AZ::u32 i = 0; i < numLODLevels; ++i)
                {
                    mMorphSetups[i] = nullptr;
                }
            }
        }

        m_isReady = true;
        ActorNotificationBus::Broadcast(&ActorNotificationBus::Events::OnActorReady, this);
        // Do not release the mesh assets. We need the mesh data to initialize future instances of the render actor instances.
    }

    // update the static AABB (very heavy as it has to create an actor instance, update mesh deformers, calculate the mesh based bounds etc)
    void Actor::UpdateStaticAABB()
    {
        if (!mStaticAABB.CheckIfIsValid())
        {
            ActorInstance* actorInstance = ActorInstance::Create(this, nullptr, mThreadIndex);
            //actorInstance->UpdateMeshDeformers(0.0f);
            //actorInstance->UpdateStaticBasedAABBDimensions();
            actorInstance->GetStaticBasedAABB(&mStaticAABB);
            actorInstance->Destroy();
        }
    }


    // find the mesh points most influenced by a particular node (pretty expensive function only intended for use in the editor)
    void Actor::FindMostInfluencedMeshPoints(const Node* node, AZStd::vector<AZ::Vector3>& outPoints) const
    {
        outPoints.clear();

        const uint32 geomLODLevel = 0;
        const uint32 numNodes = mSkeleton->GetNumNodes();

        for (int nodeIndex = 0; nodeIndex < numNodes; nodeIndex++)
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
            const uint32 numSubMeshes = mesh->GetNumSubMeshes();
            for (uint32 subMeshIndex = 0; subMeshIndex < numSubMeshes; ++subMeshIndex)
            {
                SubMesh* subMesh = mesh->GetSubMesh(subMeshIndex);

                // for all vertices in the submesh
                const uint32 startVertex = subMesh->GetStartVertex();
                const uint32 numVertices = subMesh->GetNumVertices();
                for (uint32 vertexIndex = 0; vertexIndex < numVertices; ++vertexIndex)
                {
                    const uint32 orgVertex = orgVertices[startVertex + vertexIndex];

                    // for all skinning influences of the vertex
                    const uint32 numInfluences = layer->GetNumInfluences(orgVertex);
                    float maxWeight = 0.0f;
                    uint32 maxWeightNodeIndex = 0;
                    for (uint32 i = 0; i < numInfluences; ++i)
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

        const uint32 numNodes = mNodeMirrorInfos.GetLength();
        for (uint32 i = 0; i < numNodes; ++i)
        {
            const uint16 motionSource = (GetHasMirrorInfo()) ? GetNodeMirrorInfo(i).mSourceNode : static_cast<uint16>(i);

            // displace the local transform a bit, and calculate its mirrored model space position
            pose.InitFromBindPose(this);
            Transform localTransform = pose.GetLocalSpaceTransform(motionSource);
            Transform orgDelta = Transform::CreateIdentity();
            orgDelta.mPosition.Set(1.1f, 2.2f, 3.3f);
            orgDelta.mRotation = MCore::AzEulerAnglesToAzQuat(0.1f, 0.2f, 0.3f);
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
                const float dist = MCore::SafeLength(modelSpaceResult.mPosition - endModelSpaceTransform.mPosition);
                if (dist <= MCore::Math::epsilon)
                {
                    //MCore::LogInfo("%s = %f (axis=%d)", mNodes[i]->GetName(), dist, a);
                    mNodeMirrorInfos[i].mAxis   = a;
                    mNodeMirrorInfos[i].mFlags  = 0;
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
                        const float dist = MCore::SafeLength(modelSpaceResult.mPosition - endModelSpaceTransform.mPosition);
                        if (dist <= MCore::Math::epsilon)
                        {
                            //MCore::LogInfo("*** %s = %f (axis=%d) (flip=%d)", mNodes[i]->GetName(), dist, a, f);
                            mNodeMirrorInfos[i].mAxis   = a;
                            mNodeMirrorInfos[i].mFlags  = flags;
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
                mNodeMirrorInfos[i].mAxis   = bestAxis;
                mNodeMirrorInfos[i].mFlags  = bestFlags;
                //MCore::LogInfo("best for %s = %f (axis=%d) (flags=%d)", mNodes[i]->GetName(), minDist, bestAxis, bestFlags);
            }
        }

        //for (uint32 i=0; i<numNodes; ++i)
        //MCore::LogInfo("%s = (axis=%d) (flags=%d)", GetNode(i)->GetName(), mNodeMirrorInfos[i].mAxis, mNodeMirrorInfos[i].mFlags);
    }


    // get the array of node mirror infos
    const MCore::Array<Actor::NodeMirrorInfo>& Actor::GetNodeMirrorInfos() const
    {
        return mNodeMirrorInfos;
    }


    // get the array of node mirror infos
    MCore::Array<Actor::NodeMirrorInfo>& Actor::GetNodeMirrorInfos()
    {
        return mNodeMirrorInfos;
    }


    // set the node mirror infos directly
    void Actor::SetNodeMirrorInfos(const MCore::Array<NodeMirrorInfo>& mirrorInfos)
    {
        mNodeMirrorInfos = mirrorInfos;
    }


    // try to geometrically match left with right nodes
    void Actor::MatchNodeMotionSourcesGeometrical()
    {
        Pose pose;
        pose.InitFromBindPose(this);

        const uint16 numNodes = static_cast<uint16>(mSkeleton->GetNumNodes());
        for (uint16 i = 0; i < numNodes; ++i)
        {
            //Node* node = mNodes[i];

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
        if (mSkeleton->GetNode(nodeIndex)->GetIsRootNode())
        {
            return MCORE_INVALIDINDEX16;
        }

        // calculate the model space transform and mirror it
        const Transform nodeTransform       = pose.GetModelSpaceTransform(nodeIndex);
        const Transform mirroredTransform   = nodeTransform.Mirrored(AZ::Vector3(1.0f, 0.0f, 0.0f));

        uint32 numMatches = 0;
        uint16 result = MCORE_INVALIDINDEX16;

        // find nodes that have the mirrored transform
        const uint32 numNodes = mSkeleton->GetNumNodes();
        for (uint32 i = 0; i < numNodes; ++i)
        {
            const Transform curNodeTransform = pose.GetModelSpaceTransform(i);
            if (i != nodeIndex)
            {
                // only check the translation for now
        #ifndef EMFX_SCALE_DISABLED
                if (MCore::Compare<AZ::Vector3>::CheckIfIsClose(
                        curNodeTransform.mPosition,
                        mirroredTransform.mPosition, MCore::Math::epsilon) &&
                    MCore::Compare<float>::CheckIfIsClose(MCore::SafeLength(curNodeTransform.mScale),
                        MCore::SafeLength(mirroredTransform.mScale), MCore::Math::epsilon))
        #else
                if (MCore::Compare<AZ::Vector3>::CheckIfIsClose(curNodeTransform.mPosition, mirroredTransform.mPosition, MCore::Math::epsilon))
        #endif
                {
                    numMatches++;
                    result = static_cast<uint16>(i);
                }
            }
        }

        if (numMatches == 1)
        {
            const uint32 hierarchyDepth         = mSkeleton->CalcHierarchyDepthForNode(nodeIndex);
            const uint32 matchingHierarchyDepth = mSkeleton->CalcHierarchyDepthForNode(result);
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
        Pose& bindPose = *mSkeleton->GetBindPose();
        bindPose.LinkToActor(this, Pose::FLAG_LOCALTRANSFORMREADY, false);

        const AZ::u32 numMorphs = bindPose.GetNumMorphWeights();
        for (AZ::u32 i = 0; i < numMorphs; ++i)
        {
            bindPose.SetMorphWeight(i, 0.0f);
        }

        mInvBindPoseTransforms.resize(mSkeleton->GetNumNodes());
    }


    // release any transform data
    void Actor::ReleaseTransformData()
    {
        mSkeleton->GetBindPose()->Clear();
        mInvBindPoseTransforms.clear();
    }


    // copy transforms from another actor
    void Actor::CopyTransformsFrom(const Actor* other)
    {
        MCORE_ASSERT(other->GetNumNodes() == mSkeleton->GetNumNodes());
        ResizeTransformData();
        mInvBindPoseTransforms = other->mInvBindPoseTransforms;
        *mSkeleton->GetBindPose() = *other->GetSkeleton()->GetBindPose();
    }

    void Actor::SetNumNodes(uint32 numNodes)
    {
        mSkeleton->SetNumNodes(numNodes);
        mNodeInfos.resize(numNodes);

        AZStd::vector<LODLevel>& lodLevels = m_meshLodData.m_lodLevels;
        for (LODLevel& lodLevel : lodLevels)
        {
            lodLevel.mNodeInfos.Resize(numNodes);
        }

        Pose* bindPose = mSkeleton->GetBindPose();
        bindPose->LinkToActor(this, Pose::FLAG_LOCALTRANSFORMREADY, false);
    }

    void Actor::AddNode(Node* node)
    {
        mSkeleton->AddNode(node);
        mSkeleton->GetBindPose()->LinkToActor(this, Pose::FLAG_LOCALTRANSFORMREADY, false);

        // initialize the LOD data
        mNodeInfos.emplace_back();
        AZStd::vector<LODLevel>& lodLevels = m_meshLodData.m_lodLevels;
        for (LODLevel& lodLevel : lodLevels)
        {
            lodLevel.mNodeInfos.AddEmpty();
        }

        mSkeleton->GetBindPose()->LinkToActor(this, Pose::FLAG_LOCALTRANSFORMREADY, false);
        mSkeleton->GetBindPose()->SetLocalSpaceTransform(mSkeleton->GetNumNodes() - 1, Transform::CreateIdentity());
    }

    Node* Actor::AddNode(uint32 nodeIndex, const char* name, uint32 parentIndex)
    {
        Node* node = Node::Create(name, GetSkeleton());
        node->SetNodeIndex(nodeIndex);
        node->SetParentIndex(parentIndex);
        AddNode(node);
        if (parentIndex == MCORE_INVALIDINDEX32)
        {
            GetSkeleton()->AddRootNode(node->GetNodeIndex());
        }
        else
        {
            node->GetParentNode()->AddChild(nodeIndex);
        }
        return node;
    }

    void Actor::RemoveNode(uint32 nr, bool delMem)
    {
        mSkeleton->RemoveNode(nr, delMem);
        mNodeInfos.erase(mNodeInfos.begin() + nr);

        AZStd::vector<LODLevel>& lodLevels = m_meshLodData.m_lodLevels;
        for (LODLevel& lodLevel : lodLevels)
        {
            lodLevel.mNodeInfos.Remove(nr);
        }
    }

    void Actor::DeleteAllNodes()
    {
        mSkeleton->RemoveAllNodes();
        mNodeInfos.clear();

        AZStd::vector<LODLevel>& lodLevels = m_meshLodData.m_lodLevels;
        for (LODLevel& lodLevel : lodLevels)
        {
            lodLevel.mNodeInfos.Clear();
        }
    }

    void Actor::ReserveMaterials(uint32 lodLevel, uint32 numMaterials)
    {
        mMaterials[lodLevel].Reserve(numMaterials);
    }

    // get a material
    Material* Actor::GetMaterial(uint32 lodLevel, uint32 nr) const
    {
        MCORE_ASSERT(lodLevel < mMaterials.GetLength());
        MCORE_ASSERT(nr < mMaterials[lodLevel].GetLength());
        return mMaterials[lodLevel][nr];
    }


    // get a material by name
    uint32 Actor::FindMaterialIndexByName(uint32 lodLevel, const char* name) const
    {
        MCORE_ASSERT(lodLevel < mMaterials.GetLength());

        // search through all materials
        const uint32 numMaterials = mMaterials[lodLevel].GetLength();
        for (uint32 i = 0; i < numMaterials; ++i)
        {
            if (mMaterials[lodLevel][i]->GetNameString() == name)
            {
                return i;
            }
        }

        // no material found
        return MCORE_INVALIDINDEX32;
    }

    // set a material
    void Actor::SetMaterial(uint32 lodLevel, uint32 nr, Material* mat)
    {
        MCORE_ASSERT(lodLevel < mMaterials.GetLength());
        MCORE_ASSERT(nr < mMaterials[lodLevel].GetLength());
        mMaterials[lodLevel][nr] = mat;
    }

    void Actor::AddMaterial(uint32 lodLevel, Material* mat)
    {
        MCORE_ASSERT(lodLevel < mMaterials.GetLength());
        mMaterials[lodLevel].Add(mat);
    }

    uint32 Actor::GetNumMaterials(uint32 lodLevel) const
    {
        MCORE_ASSERT(lodLevel < mMaterials.GetLength());
        return mMaterials[lodLevel].GetLength();
    }

    uint32 Actor::GetNumLODLevels() const
    {
        const AZStd::vector<LODLevel>& lodLevels = m_meshLodData.m_lodLevels;
        return static_cast<uint32>(lodLevels.size());
    }


    void* Actor::GetCustomData() const
    {
        return mCustomData;
    }


    void Actor::SetCustomData(void* dataPointer)
    {
        mCustomData = dataPointer;
    }


    const char* Actor::GetName() const
    {
        return mName.c_str();
    }


    const AZStd::string& Actor::GetNameString() const
    {
        return mName;
    }


    const char* Actor::GetFileName() const
    {
        return mFileName.c_str();
    }


    const AZStd::string& Actor::GetFileNameString() const
    {
        return mFileName;
    }


    void Actor::AddDependency(const Dependency& dependency)
    {
        mDependencies.Add(dependency);
    }


    void Actor::SetMorphSetup(uint32 lodLevel, MorphSetup* setup)
    {
        mMorphSetups[lodLevel] = setup;
    }


    uint32 Actor::GetNumNodeGroups() const
    {
        return mNodeGroups.GetLength();
    }


    NodeGroup* Actor::GetNodeGroup(uint32 index) const
    {
        return mNodeGroups[index];
    }


    void Actor::AddNodeGroup(NodeGroup* newGroup)
    {
        mNodeGroups.Add(newGroup);
    }


    void Actor::RemoveNodeGroup(uint32 index, bool delFromMem)
    {
        if (delFromMem)
        {
            mNodeGroups[index]->Destroy();
        }

        mNodeGroups.Remove(index);
    }


    void Actor::RemoveNodeGroup(NodeGroup* group, bool delFromMem)
    {
        mNodeGroups.RemoveByValue(group);
        if (delFromMem)
        {
            group->Destroy();
        }
    }


    // find a group index by its name
    uint32 Actor::FindNodeGroupIndexByName(const char* groupName) const
    {
        const uint32 numGroups = mNodeGroups.GetLength();
        for (uint32 i = 0; i < numGroups; ++i)
        {
            if (mNodeGroups[i]->GetNameString() == groupName)
            {
                return i;
            }
        }

        return MCORE_INVALIDINDEX32;
    }


    // find a group index by its name, but not case sensitive
    uint32 Actor::FindNodeGroupIndexByNameNoCase(const char* groupName) const
    {
        const uint32 numGroups = mNodeGroups.GetLength();
        for (uint32 i = 0; i < numGroups; ++i)
        {
            if (AzFramework::StringFunc::Equal(mNodeGroups[i]->GetNameString().c_str(), groupName, false /* no case */))
            {
                return i;
            }
        }

        return MCORE_INVALIDINDEX32;
    }


    // find a group by its name
    NodeGroup* Actor::FindNodeGroupByName(const char* groupName) const
    {
        const uint32 numGroups = mNodeGroups.GetLength();
        for (uint32 i = 0; i < numGroups; ++i)
        {
            if (mNodeGroups[i]->GetNameString() == groupName)
            {
                return mNodeGroups[i];
            }
        }
        return nullptr;
    }


    // find a group by its name, but without case sensitivity
    NodeGroup* Actor::FindNodeGroupByNameNoCase(const char* groupName) const
    {
        const uint32 numGroups = mNodeGroups.GetLength();
        for (uint32 i = 0; i < numGroups; ++i)
        {
            if (AzFramework::StringFunc::Equal(mNodeGroups[i]->GetNameString().c_str(), groupName, false /* no case */))
            {
                return mNodeGroups[i];
            }
        }
        return nullptr;
    }


    void Actor::SetDirtyFlag(bool dirty)
    {
        mDirtyFlag = dirty;
    }


    bool Actor::GetDirtyFlag() const
    {
        return mDirtyFlag;
    }


    void Actor::SetIsUsedForVisualization(bool flag)
    {
        mUsedForVisualization = flag;
    }


    bool Actor::GetIsUsedForVisualization() const
    {
        return mUsedForVisualization;
    }

    void Actor::SetIsOwnedByRuntime(bool isOwnedByRuntime)
    {
#if defined(EMFX_DEVELOPMENT_BUILD)
        mIsOwnedByRuntime = isOwnedByRuntime;
#else
        AZ_UNUSED(isOwnedByRuntime);
#endif
    }


    bool Actor::GetIsOwnedByRuntime() const
    {
#if defined(EMFX_DEVELOPMENT_BUILD)
        return mIsOwnedByRuntime;
#else
        return true;
#endif
    }

    const MCore::AABB& Actor::GetStaticAABB() const
    {
        return mStaticAABB;
    }

    void Actor::SetStaticAABB(const MCore::AABB& box)
    {
        mStaticAABB = box;
    }

    //---------------------------------

    Mesh* Actor::GetMesh(uint32 lodLevel, uint32 nodeIndex) const
    {
        const AZStd::vector<LODLevel>& lodLevels = m_meshLodData.m_lodLevels;
        return lodLevels[lodLevel].mNodeInfos[nodeIndex].mMesh;
    }

    MeshDeformerStack* Actor::GetMeshDeformerStack(uint32 lodLevel, uint32 nodeIndex) const
    {
        const AZStd::vector<LODLevel>& lodLevels = m_meshLodData.m_lodLevels;
        return lodLevels[lodLevel].mNodeInfos[nodeIndex].mStack;
    }

    // set the mesh for a given node in a given LOD
    void Actor::SetMesh(uint32 lodLevel, uint32 nodeIndex, Mesh* mesh)
    {
        AZStd::vector<LODLevel>& lodLevels = m_meshLodData.m_lodLevels;
        lodLevels[lodLevel].mNodeInfos[nodeIndex].mMesh = mesh;
    }


    // set the mesh deformer stack for a given node in a given LOD
    void Actor::SetMeshDeformerStack(uint32 lodLevel, uint32 nodeIndex, MeshDeformerStack* stack)
    {
        AZStd::vector<LODLevel>& lodLevels = m_meshLodData.m_lodLevels;
        lodLevels[lodLevel].mNodeInfos[nodeIndex].mStack = stack;
    }

    // check if the mesh has a skinning deformer (either linear or dual quat)
    bool Actor::CheckIfHasSkinningDeformer(uint32 lodLevel, uint32 nodeIndex) const
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


    // calculate the OBB for a given node
    void Actor::CalcOBBFromBindPose(uint32 lodLevel, uint32 nodeIndex)
    {
        AZStd::vector<AZ::Vector3> points;

        // if there is a mesh
        Mesh* mesh = GetMesh(lodLevel, nodeIndex);
        if (mesh)
        {
            // if the mesh is not skinned
            if (mesh->FindSharedVertexAttributeLayer(SkinningInfoVertexAttributeLayer::TYPE_ID) == nullptr)
            {
                mesh->ExtractOriginalVertexPositions(points);
            }
        }
        else // there is no mesh, so maybe this is a bone
        {
            const Transform& invBindPoseTransform = GetInverseBindPoseTransform(nodeIndex);

            // for all nodes inside the actor where this node belongs to
            const uint32 numNodes = mSkeleton->GetNumNodes();
            for (uint32 n = 0; n < numNodes; ++n)
            {
                Mesh* loopMesh = GetMesh(lodLevel, n);
                if (loopMesh == nullptr)
                {
                    continue;
                }

                // get the vertex positions in bind pose
                const uint32 numVerts = loopMesh->GetNumVertices();
                points.reserve(numVerts * 2);
                AZ::Vector3* positions = (AZ::Vector3*)loopMesh->FindOriginalVertexData(Mesh::ATTRIB_POSITIONS);

                SkinningInfoVertexAttributeLayer* skinLayer = (SkinningInfoVertexAttributeLayer*)loopMesh->FindSharedVertexAttributeLayer(SkinningInfoVertexAttributeLayer::TYPE_ID);
                if (skinLayer)
                {
                    // iterate over all skinning influences and see if this node number is used
                    // if so, add it to the list of points
                    const uint32* orgVertices = (uint32*)loopMesh->FindVertexData(Mesh::ATTRIB_ORGVTXNUMBERS);
                    for (uint32 v = 0; v < numVerts; ++v)
                    {
                        // get the original vertex number
                        const uint32 orgVtx = orgVertices[v];

                        // for all skinning influences for this vertex
                        const size_t numInfluences = skinLayer->GetNumInfluences(orgVtx);
                        for (size_t i = 0; i < numInfluences; ++i)
                        {
                            // get the node used by this influence
                            const uint32 nodeNr = skinLayer->GetInfluence(orgVtx, i)->GetNodeNr();

                            // if this is the same node as we are updating the bounds for, add the vertex position to the list
                            if (nodeNr == nodeIndex)
                            {
                                const AZ::Vector3 tempPos(positions[v]);
                                points.emplace_back(invBindPoseTransform.TransformPoint(tempPos));
                            }
                        } // for all influences
                    } // for all vertices
                } // if there is skinning info
            } // for all nodes
        }

        // init from the set of points
        if (!points.empty())
        {
            GetNodeOBB(nodeIndex).InitFromPoints(&points[0], static_cast<uint32>(points.size()));
        }
        else
        {
            GetNodeOBB(nodeIndex).Init();
        }
    }

    // remove the mesh for a given node in a given LOD
    void Actor::RemoveNodeMeshForLOD(uint32 lodLevel, uint32 nodeIndex, bool destroyMesh)
    {
        AZStd::vector<LODLevel>& lodLevels = m_meshLodData.m_lodLevels;

        LODLevel&       lod         = lodLevels[lodLevel];
        NodeLODInfo&    nodeInfo    = lod.mNodeInfos[nodeIndex];

        if (destroyMesh && nodeInfo.mMesh)
        {
            MCore::Destroy(nodeInfo.mMesh);
        }

        if (destroyMesh && nodeInfo.mStack)
        {
            MCore::Destroy(nodeInfo.mStack);
        }

        nodeInfo.mMesh  = nullptr;
        nodeInfo.mStack = nullptr;
    }

    void Actor::SetUnitType(MCore::Distance::EUnitType unitType)
    {
        mUnitType = unitType;
    }


    MCore::Distance::EUnitType Actor::GetUnitType() const
    {
        return mUnitType;
    }


    void Actor::SetFileUnitType(MCore::Distance::EUnitType unitType)
    {
        mFileUnitType = unitType;
    }


    MCore::Distance::EUnitType Actor::GetFileUnitType() const
    {
        return mFileUnitType;
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
        const uint32 numNodes = GetNumNodes();
        for (uint32 i = 0; i < numNodes; ++i)
        {
            Transform transform = bindPose->GetLocalSpaceTransform(i);
            transform.mPosition *= scaleFactor;
            bindPose->SetLocalSpaceTransform(i, transform);
        }
        bindPose->ForceUpdateFullModelSpacePose();

        // calculate the inverse bind pose matrices
        for (uint32 i = 0; i < numNodes; ++i)
        {
            mInvBindPoseTransforms[i] = bindPose->GetModelSpaceTransform(i).Inversed();
        }

        // update node obbs
        for (uint32 i = 0; i < numNodes; ++i)
        {
            MCore::OBB& box = GetNodeOBB(i);
            box.SetExtents(box.GetExtents() * scaleFactor);
            box.SetCenter(box.GetCenter() * scaleFactor);
        }

        // update static aabb
        mStaticAABB.SetMin(mStaticAABB.GetMin() * scaleFactor);
        mStaticAABB.SetMax(mStaticAABB.GetMax() * scaleFactor);

        // update mesh data for all LOD levels
        const uint32 numLODs = GetNumLODLevels();
        for (uint32 lod = 0; lod < numLODs; ++lod)
        {
            for (uint32 i = 0; i < numNodes; ++i)
            {
                Mesh* mesh = GetMesh(lod, i);
                if (mesh)
                {
                    mesh->Scale(scaleFactor);
                }
            }
        }

        // scale morph target data
        for (uint32 lod = 0; lod < numLODs; ++lod)
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
        if (mUnitType == targetUnitType)
        {
            return;
        }

        // calculate the scale factor and scale
        const float scaleFactor = static_cast<float>(MCore::Distance::GetConversionFactor(mUnitType, targetUnitType));
        Scale(scaleFactor);

        // update the unit type
        mUnitType = targetUnitType;
    }


    // Try to figure out which axis points "up" for the motion extraction node.
    Actor::EAxis Actor::FindBestMatchingMotionExtractionAxis() const
    {
        MCORE_ASSERT(mMotionExtractionNode != MCORE_INVALIDINDEX32);
        if (mMotionExtractionNode == MCORE_INVALIDINDEX32)
        {
            return AXIS_Y;
        }

        // Get the local space rotation matrix of the motion extraction node.
        const Transform& localTransform = GetBindPose()->GetLocalSpaceTransform(mMotionExtractionNode);
        const AZ::Matrix3x3 rotationMatrix = AZ::Matrix3x3::CreateFromQuaternion(localTransform.mRotation);

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


    void Actor::SetRetargetRootNodeIndex(uint32 nodeIndex)
    {
        mRetargetRootNode = nodeIndex;
    }


    void Actor::SetRetargetRootNode(Node* node)
    {
        mRetargetRootNode = node ? node->GetNodeIndex() : MCORE_INVALIDINDEX32;
    }

    void Actor::InsertJointAndParents(AZ::u32 jointIndex, AZStd::unordered_set<AZ::u32>& includedJointIndices)
    {
        // If our joint is already in, then we can skip things.
        if (includedJointIndices.find(jointIndex) != includedJointIndices.end())
        {
            return;
        }

        // Add the parent.
        const AZ::u32 parentIndex = mSkeleton->GetNode(jointIndex)->GetParentIndex();
        if (parentIndex != InvalidIndex32)
        {
            InsertJointAndParents(parentIndex, includedJointIndices);
        }

        // Add itself.
        includedJointIndices.emplace(jointIndex);
    }

    void Actor::AutoSetupSkeletalLODsBasedOnSkinningData(const AZStd::vector<AZStd::string>& alwaysIncludeJoints)
    {
        AZStd::unordered_set<AZ::u32> includedJointIndices;

        const AZ::u32 numLODs = GetNumLODLevels();
        for (AZ::u32 lod = 0; lod < numLODs; ++lod)
        {
            includedJointIndices.clear();

            // If we have no meshes, or only static meshes, we shouldn't do anything.
            if (!CheckIfHasMeshes(lod) || !CheckIfHasSkinnedMeshes(lod))
            {
                continue;
            }

            const AZ::u32 numJoints = mSkeleton->GetNumNodes();
            for (AZ::u32 jointIndex = 0; jointIndex < numJoints; ++jointIndex)
            {
                const Mesh* mesh = GetMesh(lod, jointIndex);
                if (!mesh)
                {
                    continue;
                }

                // Include the mesh, as we always want to include meshes.
                InsertJointAndParents(jointIndex, includedJointIndices);

                // Look at the joints registered in the submeshes.
                const AZ::u32 numSubMeshes = mesh->GetNumSubMeshes();
                for (AZ::u32 subMeshIndex = 0; subMeshIndex < numSubMeshes; ++subMeshIndex)
                {
                    const MCore::Array<AZ::u32>& subMeshJoints = mesh->GetSubMesh(subMeshIndex)->GetBonesArray();
                    const AZ::u32 numSubMeshJoints = subMeshJoints.GetLength();
                    for (AZ::u32 i = 0; i < numSubMeshJoints; ++i)
                    {
                        InsertJointAndParents(subMeshJoints[i], includedJointIndices);
                    }
                }
            } // for all joints

            // Now that we have the list of joints to include for this LOD, let's disable all the joints in this LOD, and enable all the ones in our list.
            if (!includedJointIndices.empty())
            {
                // Force joints in our "always include list" to be included.
                for (const AZStd::string& jointName : alwaysIncludeJoints)
                {
                    AZ::u32 jointIndex = InvalidIndex32;
                    if (!mSkeleton->FindNodeAndIndexByName(jointName, jointIndex))
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
                for (AZ::u32 jointIndex = 0; jointIndex < numJoints; ++jointIndex)
                {
                    mSkeleton->GetNode(jointIndex)->SetSkeletalLODStatus(lod, false);
                }

                // Enable all our included joints in this skeletal LOD.
                AZ_TracePrintf("EMotionFX", "[LOD %d] Enabled joints = %zd\n", lod, includedJointIndices.size());
                for (AZ::u32 jointIndex : includedJointIndices)
                {
                    mSkeleton->GetNode(jointIndex)->SetSkeletalLODStatus(lod, true);
                }
            }
            else // When we have an empty include list, enable everything.
            {
                AZ_TracePrintf("EMotionFX", "[LOD %d] Enabled joints = %zd\n", lod, mSkeleton->GetNumNodes());
                for (AZ::u32 i = 0; i < mSkeleton->GetNumNodes(); ++i)
                {
                    mSkeleton->GetNode(i)->SetSkeletalLODStatus(lod, true);
                }
            }
        } // for each LOD
    }


    void Actor::PrintSkeletonLODs()
    {
        const AZ::u32 numLODs = GetNumLODLevels();
        for (AZ::u32 lod = 0; lod < numLODs; ++lod)
        {
            AZ_TracePrintf("EMotionFX", "[LOD %d]:", lod);
            const AZ::u32 numJoints = mSkeleton->GetNumNodes();
            for (AZ::u32 jointIndex = 0; jointIndex < numJoints; ++jointIndex)
            {
                const Node* joint = mSkeleton->GetNode(jointIndex);
                if (joint->GetSkeletalLODStatus(lod))
                {
                    AZ_TracePrintf("EMotionFX", "\t%s (index=%d)", joint->GetName(), jointIndex);
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
        AZ::u32 numNodes = mSkeleton->GetNumNodes();
        AZStd::vector<bool> flags;
        AZStd::unordered_map<AZStd::string, AZStd::string> childParentMap;
        flags.resize(numNodes);
        
        AZStd::unordered_set<Node*> nodesToKeep;
        // Search the hit detection config to find and keep all the hit detection nodes.
        for (const Physics::CharacterColliderNodeConfiguration& nodeConfig : m_physicsSetup->GetHitDetectionConfig().m_nodes)
        {
            Node* node = mSkeleton->FindNodeByName(nodeConfig.m_name);
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
        for (AZ::u32 i = 0; i < numNodes; ++i)
        {
            Node* node = mSkeleton->GetNode(i);
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
        for (AZ::u32 nodeIndex = numNodes - 1; nodeIndex > 0; nodeIndex--)
        {
            if (!flags[nodeIndex])
            {
                mSkeleton->RemoveNode(nodeIndex);
            }
        }

        // Update the node index.
        mSkeleton->UpdateNodeIndexValues();

        // After the node index changed, the parent index become invalid. First, clear all information about children because
        // it's not valid anymore.
        for (AZ::u32 nodeIndex = 0; nodeIndex < mSkeleton->GetNumNodes(); ++nodeIndex)
        {
            Node* node = mSkeleton->GetNode(nodeIndex);
            node->RemoveAllChildNodes();
        }

        // Then build the child-parent relationship using the prebuild map.
        for (auto& pair : childParentMap)
        {
            Node* child = mSkeleton->FindNodeByName(pair.first);
            Node* parent = mSkeleton->FindNodeByName(pair.second);
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
        const AZStd::array_view<AZ::RPI::ModelLodAsset::Mesh>& sourceMeshes = lodModelAsset->GetMeshes();

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
        AZ_Assert(mSkeleton->GetNode(0), "Actor needs to have at least a single joint.");
        return mSkeleton->GetNode(0);
    }

    void Actor::ConstructMeshes()
    {
        AZ_Assert(m_meshAsset.IsReady(), "Mesh asset should be fully loaded and ready.");

        AZStd::vector<LODLevel>& lodLevels = m_meshLodData.m_lodLevels;
        const AZStd::array_view<AZ::Data::Asset<AZ::RPI::ModelLodAsset>>& lodAssets = m_meshAsset->GetLodAssets();
        const size_t numLODLevels = lodAssets.size();

        lodLevels.clear();
        SetNumLODLevels(numLODLevels, /*adjustMorphSetup=*/false);
        const uint32 numNodes = mSkeleton->GetNumNodes();

        // Remove all the materials and add them back based on the meshAsset. Eventually we will remove all the material from Actor and
        // GLActor.
        RemoveAllMaterials();
        mMaterials.Resize(numLODLevels);

        for (size_t lodLevel = 0; lodLevel < numLODLevels; ++lodLevel)
        {
            const AZ::Data::Asset<AZ::RPI::ModelLodAsset>& lodAsset = lodAssets[lodLevel];

            lodLevels[lodLevel].mNodeInfos.Resize(numNodes);

            // Create a single mesh for the actor.
            Mesh* mesh = Mesh::CreateFromModelLod(lodAsset, m_skinToSkeletonIndexMap);

            // Find an owning joint for the mesh.
            Node* meshJoint = FindMeshJoint(lodAsset);
            if (!meshJoint)
            {
                AZ_Error("EMotionFX", false, "Cannot find mesh joint. Skipping to add meshes to the actor.");
                continue;
            }

            const AZ::u32 jointIndex = meshJoint->GetNodeIndex();
            NodeLODInfo& jointInfo = lodLevels[lodLevel].mNodeInfos[jointIndex];

            jointInfo.mMesh = mesh;

            if (!jointInfo.mStack)
            {
                jointInfo.mStack = MeshDeformerStack::Create(mesh);
            }

            // Add the skinning deformers
            const AZ::u32 numLayers = mesh->GetNumSharedVertexAttributeLayers();
            for (AZ::u32 layerNr = 0; layerNr < numLayers; ++layerNr)
            {
                EMotionFX::VertexAttributeLayer* vertexAttributeLayer = mesh->GetSharedVertexAttributeLayer(layerNr);
                if (vertexAttributeLayer->GetType() != EMotionFX::SkinningInfoVertexAttributeLayer::TYPE_ID)
                {
                    continue;
                }

                EMotionFX::SkinningInfoVertexAttributeLayer* skinLayer =
                    static_cast<EMotionFX::SkinningInfoVertexAttributeLayer*>(vertexAttributeLayer);
                const AZ::u32 numOrgVerts = skinLayer->GetNumAttributes();
                AZStd::set<AZ::u32> localJointIndices = skinLayer->CalcLocalJointIndices(numOrgVerts);
                const AZ::u32 numLocalJoints = static_cast<AZ::u32>(localJointIndices.size());

                // The information about if we want to use dual quat skinning is baked into the mesh chunk and we don't have access to that
                // anymore. Default to dual quat skinning.
                const bool dualQuatSkinning = true;
                if (dualQuatSkinning)
                {
                    DualQuatSkinDeformer* skinDeformer = DualQuatSkinDeformer::Create(mesh);
                    jointInfo.mStack->AddDeformer(skinDeformer);
                    skinDeformer->ReserveLocalBones(numLocalJoints);
                    skinDeformer->Reinitialize(this, meshJoint, lodLevel);
                }
                else
                {
                    SoftSkinDeformer* skinDeformer = GetSoftSkinManager().CreateDeformer(mesh);
                    jointInfo.mStack->AddDeformer(skinDeformer);
                    skinDeformer->ReserveLocalBones(numLocalJoints); // pre-alloc data to prevent reallocs
                    skinDeformer->Reinitialize(this, meshJoint, lodLevel);
                }
            }

            // Add material for this mesh
            AddMaterial(lodLevel, Material::Create(GetName()));
        }
    }

    Node* Actor::FindJointByMeshName(const AZStd::string_view meshName) const
    {
        Node* joint = mSkeleton->FindNodeByName(meshName.data());
        if (!joint)
        {
            // When mesh merging in the model builder is enabled, the name of the mesh is the concatenated version
            // of all the merged meshes with a plus symbol used as delimiter. Try to find any of the merged mesh joint
            // and use the first one to add the mesh to it.
            AZStd::vector<AZStd::string> tokens;
            AZ::StringFunc::Tokenize(meshName, tokens, '+');
            for (const AZStd::string& token : tokens)
            {
                joint = mSkeleton->FindNodeByName(token);
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
            const Node* node = mSkeleton->FindNodeByName(pair.first.c_str());
            if (!node)
            {
                AZ_Assert(node, "Cannot find joint named %s in the skeleton while it is used by the skin.", pair.first.c_str());
                continue;
            }
            result.emplace(pair.second, node->GetNodeIndex());
        }

        return result;
    }

    void Actor::ConstructMorphTargets()
    {
        AZ_Assert(m_meshAsset.IsReady() && m_morphTargetMetaAsset.IsReady(),
            "Mesh as well as morph target meta asset asset should be fully loaded and ready.");
        AZStd::vector<LODLevel>& lodLevels = m_meshLodData.m_lodLevels;
        const AZStd::array_view<AZ::Data::Asset<AZ::RPI::ModelLodAsset>>& lodAssets = m_meshAsset->GetLodAssets();
        const size_t numLODLevels = lodAssets.size();

        AZ_Assert(mMorphSetups.GetLength() == numLODLevels, "There needs to be a morph setup for every single LOD level.");

        for (size_t lodLevel = 0; lodLevel < numLODLevels; ++lodLevel)
        {
            const AZ::Data::Asset<AZ::RPI::ModelLodAsset>& lodAsset = lodAssets[lodLevel];
            const AZStd::array_view<AZ::RPI::ModelLodAsset::Mesh>& sourceMeshes = lodAsset->GetMeshes();

            MorphSetup* morphSetup = mMorphSetups[lodLevel];
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

            const AZ::u32 jointIndex = meshJoint->GetNodeIndex();
            NodeLODInfo& jointInfo = lodLevels[lodLevel].mNodeInfos[jointIndex];
            Mesh* mesh = jointInfo.mMesh;

            if (!jointInfo.mStack)
            {
                jointInfo.mStack = MeshDeformerStack::Create(mesh);
            }

            // Add the morph deformer to the mesh deformer stack (in case there is none yet).
            MorphMeshDeformer* morphTargetDeformer = (MorphMeshDeformer*)jointInfo.mStack->FindDeformerByType(MorphMeshDeformer::TYPE_ID);
            if (!morphTargetDeformer)
            {
                morphTargetDeformer = MorphMeshDeformer::Create(mesh);
                // Add insert the deformer at the first position to make sure we apply morph targets before skinning.
                jointInfo.mStack->InsertDeformer(/*deformerPosition=*/0, morphTargetDeformer);
            }

            // The lod has shared buffers that combine the data from each submesh. In case any of the submeshes has a
            // morph target buffer view we can access the entire morph target buffer via the buffer asset.
            AZStd::array_view<uint8_t> morphTargetDeltaView;
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

            const AZ::u32 numMorphTargets = morphSetup->GetNumMorphTargets();
            for (AZ::u32 mtIndex = 0; mtIndex < numMorphTargets; ++mtIndex)
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
                        deformData->mMinValue = metaData.m_minPositionDelta;
                        deformData->mMaxValue = metaData.m_maxPositionDelta;

                        for (AZ::u32 deformVtx = 0; deformVtx < numDeformedVertices; ++deformVtx)
                        {
                            // Get the index into the morph delta buffer
                            const AZ::u32 vertexIndex = metaData.m_startIndex + deformVtx;

                            // Unpack the delta
                            const AZ::RPI::PackedCompressedMorphTargetDelta& packedCompressedDelta = vertexDeltas[vertexIndex];
                            AZ::RPI::CompressedMorphTargetDelta unpackedCompressedDelta = AZ::RPI::UnpackMorphTargetDelta(packedCompressedDelta);

                            // Set the EMotionFX deform data from the CmopressedMorphTargetDelta
                            deformData->mDeltas[deformVtx].mVertexNr = unpackedCompressedDelta.m_morphedVertexIndex;

                            deformData->mDeltas[deformVtx].mPosition = MCore::Compressed16BitVector3(
                                unpackedCompressedDelta.m_positionX,
                                unpackedCompressedDelta.m_positionY,
                                unpackedCompressedDelta.m_positionZ);

                            deformData->mDeltas[deformVtx].mNormal = MCore::Compressed8BitVector3(
                                unpackedCompressedDelta.m_normalX,
                                unpackedCompressedDelta.m_normalY,
                                unpackedCompressedDelta.m_normalZ);

                            deformData->mDeltas[deformVtx].mTangent = MCore::Compressed8BitVector3(
                                unpackedCompressedDelta.m_tangentX,
                                unpackedCompressedDelta.m_tangentY,
                                unpackedCompressedDelta.m_tangentZ);

                            deformData->mDeltas[deformVtx].mBitangent = MCore::Compressed8BitVector3(
                                unpackedCompressedDelta.m_bitangentX,
                                unpackedCompressedDelta.m_bitangentY,
                                unpackedCompressedDelta.m_bitangentZ);
                        }

                        morphTarget->AddDeformData(deformData);
                    }
                }
            }

            // Sync the deformer passes with the morph target deform datas.
            morphTargetDeformer->Reinitialize(this, meshJoint, lodLevel);
        }
    }
} // namespace EMotionFX
