/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "EMotionFXConfig.h"
#include <MCore/Source/IDGenerator.h>
#include <MCore/Source/Random.h>
#include <MCore/Source/LogManager.h>

#include "Actor.h"
#include "ActorInstance.h"
#include "ActorManager.h"
#include "ActorUpdateScheduler.h"
#include "AnimGraphInstance.h"
#include "Attachment.h"
#include "AttachmentNode.h"
#include "AttachmentSkin.h"
#include "EventManager.h"
#include "Mesh.h"
#include "MeshDeformerStack.h"
#include "MorphMeshDeformer.h"
#include "MorphSetup.h"
#include "MorphSetupInstance.h"
#include "MotionLayerSystem.h"
#include "Node.h"
#include "NodeGroup.h"
#include "Recorder.h"
#include "TransformData.h"
#include <EMotionFX/Source/ActorInstanceBus.h>
#include <EMotionFX/Source/DebugDraw.h>
#include <EMotionFX/Source/RagdollInstance.h>

namespace EMotionFX
{
    AZ_CLASS_ALLOCATOR_IMPL(ActorInstance, ActorInstanceAllocator, 0)

    ActorInstance::ActorInstance(Actor* actor, AZ::Entity* entity, uint32 threadIndex)
        : BaseObject()
        , m_entity(entity)
        , m_ragdollInstance(nullptr)
    {
        MCORE_ASSERT(actor);

        m_enabledNodes.reserve(actor->GetNumNodes());

        // set the actor and create the motion system
        m_boolFlags              = 0;
        m_actor                  = actor;
        m_lodLevel               = 0;
        m_requestedLODLevel     = 0;
        m_numAttachmentRefs      = 0;
        m_threadIndex            = threadIndex;
        m_attachedTo             = nullptr;
        m_selfAttachment         = nullptr;
        m_customData             = nullptr;
        m_id                     = aznumeric_caster(MCore::GetIDGenerator().GenerateID());
        m_visualizeScale         = 1.0f;
        m_motionSamplingRate     = 0.0f;
        m_motionSamplingTimer    = 0.0f;

        m_trajectoryDelta.IdentityWithZeroScale();
        m_staticAabb = AZ::Aabb::CreateNull();

        m_animGraphInstance = nullptr;

        // set the boolean defaults
        SetFlag(BOOL_ISVISIBLE, true);
        SetFlag(BOOL_BOUNDSUPDATEENABLED, true);
        SetFlag(BOOL_NORMALIZEDMOTIONLOD, true);
        SetFlag(BOOL_RENDER, true);
        SetFlag(BOOL_USEDFORVISUALIZATION, false);
        SetFlag(BOOL_ENABLED, true);
        SetFlag(BOOL_MOTIONEXTRACTION, true);

#if defined(EMFX_DEVELOPMENT_BUILD)
        SetFlag(BOOL_OWNEDBYRUNTIME, false);
#endif // EMFX_DEVELOPMENT_BUILD

        // enable all nodes on default
        EnableAllNodes();

        // apply actor node group default states (disable groups of nodes that are disabled on default)
        const size_t numGroups = m_actor->GetNumNodeGroups();
        for (size_t i = 0; i < numGroups; ++i)
        {
            if (m_actor->GetNodeGroup(i)->GetIsEnabledOnDefault() == false) // if this group is disabled on default
            {
                m_actor->GetNodeGroup(i)->DisableNodes(this); // disable all nodes inside this group
            }
        }
        // disable nodes that are disabled in LOD 0
        Skeleton* skeleton = m_actor->GetSkeleton();
        const size_t numNodes = skeleton->GetNumNodes();
        for (size_t n = 0; n < numNodes; ++n)
        {
            if (skeleton->GetNode(n)->GetSkeletalLODStatus(0) == false)
            {
                DisableNode(static_cast<uint16>(n));
            }
        }

        // setup auto bounds update (it is enabled on default)
        m_boundsUpdateFrequency = 0.0f;
        m_boundsUpdatePassedTime = 0.0f;
        m_boundsUpdateType = BOUNDS_STATIC_BASED;
        m_boundsUpdateItemFreq = 1;

        // initialize the actor local and global transform
        m_parentWorldTransform.Identity();
        m_localTransform.Identity();
        m_worldTransform.Identity();
        m_worldTransformInv.Identity();

        // init the morph setup instance
        m_morphSetup = MorphSetupInstance::Create();
        m_morphSetup->Init(actor->GetMorphSetup(0));

        // initialize the transformation data of this instance
        m_transformData = TransformData::Create();
        m_transformData->InitForActorInstance(this);

        // create the motion system
        m_motionSystem = MotionLayerSystem::Create(this);

        // update the global and local matrices
        UpdateTransformations(0.0f);

        // update the actor dependencies
        UpdateDependencies();

        // update the static based AABB dimensions
        m_staticAabb = m_actor->GetStaticAabb();
        if (!m_staticAabb.IsValid())
        {
            UpdateMeshDeformers(0.0f, true); // TODO: not really thread safe because of shared meshes, although it probably will output correctly
            UpdateStaticBasedAabbDimensions();
        }

        // update the bounds
        UpdateBounds(/*lodLevel=*/0, m_boundsUpdateType);

        // register it
        GetActorManager().RegisterActorInstance(this);

        GetActorManager().GetScheduler()->RecursiveInsertActorInstance(this);

        ActorInstanceNotificationBus::Broadcast(&ActorInstanceNotificationBus::Events::OnActorInstanceCreated, this);
    }

    ActorInstance::~ActorInstance()
    {
        ActorInstanceNotificationBus::Broadcast(&ActorInstanceNotificationBus::Events::OnActorInstanceDestroyed, this);

        // get rid of the motion system
        if (m_motionSystem)
        {
            m_motionSystem->Destroy();
        }

        if (m_animGraphInstance)
        {
            m_animGraphInstance->Destroy();
        }

        GetDebugDraw().UnregisterActorInstance(this);

        // delete all attachments
        // actor instances that are attached will be detached, and not deleted from memory
        const size_t numAttachments = m_attachments.size();
        for (size_t i = 0; i < numAttachments; ++i)
        {
            ActorInstance* attachmentActorInstance = m_attachments[i]->GetAttachmentActorInstance();
            if (attachmentActorInstance)
            {
                attachmentActorInstance->SetAttachedTo(nullptr);
                attachmentActorInstance->SetSelfAttachment(nullptr);
                attachmentActorInstance->DecreaseNumAttachmentRefs();
                GetActorManager().UpdateActorInstanceStatus(attachmentActorInstance);
            }
            m_attachments[i]->Destroy();
        }
        m_attachments.clear();

        if (m_morphSetup)
        {
            m_morphSetup->Destroy();
        }

        if (m_transformData)
        {
            m_transformData->Destroy();
        }

        // remove the attachment from the actor instance where it is attached to
        if (GetIsAttachment())
        {
            m_attachedTo->RemoveAttachment(this /*, false*/);
        }

        // automatically unregister the actor instance
        GetActorManager().UnregisterActorInstance(this);
    }

    ActorInstance* ActorInstance::Create(Actor* actor, AZ::Entity* entity, uint32 threadIndex)
    {
        return aznew ActorInstance(actor, entity, threadIndex);
    }

    // update the transformation data
    void ActorInstance::UpdateTransformations(float timePassedInSeconds, bool updateJointTransforms, bool sampleMotions)
    {
        // Update the LOD level in case a change was requested.
        UpdateLODLevel();

        const Recorder& recorder = GetRecorder();
        timePassedInSeconds *= GetEMotionFX().GetGlobalSimulationSpeed();

        // if we are using the recorder to playback
        if (recorder.GetIsInPlayMode() && recorder.GetHasRecorded(this))
        {
            // output the anim graph instance, this doesn't overwrite transforms, just some things internally
            if (recorder.GetRecordSettings().m_recordAnimGraphStates && m_animGraphInstance)
            {
                m_animGraphInstance->Update(0.0f);
                m_animGraphInstance->Output(nullptr);
            }

            // apply the main transformation
            recorder.SampleAndApplyMainTransform(recorder.GetCurrentPlayTime(), this);

            // apply the node transforms
            if (recorder.GetRecordSettings().m_recordTransforms)
            {
                recorder.SampleAndApplyTransforms(recorder.GetCurrentPlayTime(), this);
            }

            // sample the morph targets
            if (recorder.GetRecordSettings().m_recordMorphs)
            {
                recorder.SampleAndApplyMorphs(recorder.GetCurrentPlayTime(), this);
            }

            // perform forward kinematics etc
            UpdateWorldTransform();
            UpdateSkinningMatrices();
            UpdateAttachments(); // update the attachment parent matrices

            // update the bounds when needed
            if (GetBoundsUpdateEnabled())
            {
                m_boundsUpdatePassedTime += timePassedInSeconds;
                if (m_boundsUpdatePassedTime >= m_boundsUpdateFrequency)
                {
                    UpdateBounds(m_lodLevel, m_boundsUpdateType, m_boundsUpdateItemFreq);
                    m_boundsUpdatePassedTime = 0.0f;
                }
            }

            return;
        } // if the recorder is in playback mode and we recorded this actor instance

        // check if we are an attachment
        Attachment* attachment = GetSelfAttachment();

        // if we are not an attachment, or if we are but not one influenced by multiple joints
        if (!attachment || !attachment->GetIsInfluencedByMultipleJoints())
        {
            // update the motion system, which performs all blending, and updates all local transforms (excluding the local matrices)
            if (m_animGraphInstance)
            {
                m_animGraphInstance->Update(timePassedInSeconds);
                UpdateWorldTransform();
                if (updateJointTransforms && sampleMotions)
                {
                    m_animGraphInstance->Output(m_transformData->GetCurrentPose());

                    if (m_ragdollInstance)
                    {
                        m_ragdollInstance->PostAnimGraphUpdate(timePassedInSeconds);
                    }
                }
            }
            else if (m_motionSystem)
            {
                m_motionSystem->Update(timePassedInSeconds, (updateJointTransforms && sampleMotions));
            }
            else
            {
                UpdateWorldTransform();
            }

            // when the actor instance isn't visible, we don't want to do more things
            if (!updateJointTransforms)
            {
                if (GetBoundsUpdateEnabled() && m_boundsUpdateType == BOUNDS_STATIC_BASED)
                {
                    UpdateBounds(m_lodLevel, m_boundsUpdateType);
                }

                return;
            }

            m_transformData->GetCurrentPose()->ApplyMorphWeightsToActorInstance();
            ApplyMorphSetup();

            UpdateSkinningMatrices();
            UpdateAttachments();
        }
        else // we are a skin attachment
        {
            m_localTransform.Identity();
            if (m_animGraphInstance)
            {
                m_animGraphInstance->Update(timePassedInSeconds);
                UpdateWorldTransform();

                if (updateJointTransforms && sampleMotions)
                {
                    m_animGraphInstance->Output(m_transformData->GetCurrentPose());
                }
            }
            else if (m_motionSystem)
            {
                m_motionSystem->Update(timePassedInSeconds, (updateJointTransforms && sampleMotions));
            }
            else
            {
                UpdateWorldTransform();
            }

            // when the actor instance isn't visible, we don't want to do more things
            if (!updateJointTransforms)
            {
                if (GetBoundsUpdateEnabled() && m_boundsUpdateType == BOUNDS_STATIC_BASED)
                {
                    UpdateBounds(m_lodLevel, m_boundsUpdateType);
                }
                return;
            }

            m_selfAttachment->UpdateJointTransforms(*m_transformData->GetCurrentPose());
            m_transformData->GetCurrentPose()->ApplyMorphWeightsToActorInstance();
            ApplyMorphSetup();
            UpdateSkinningMatrices();
            UpdateAttachments();
        }

        // update the bounds when needed
        if (GetBoundsUpdateEnabled())
        {
            m_boundsUpdatePassedTime += timePassedInSeconds;
            if (m_boundsUpdatePassedTime >= m_boundsUpdateFrequency)
            {
                UpdateBounds(m_lodLevel, m_boundsUpdateType, m_boundsUpdateItemFreq);
                m_boundsUpdatePassedTime = 0.0f;
            }
        }
    }

    // update the world transformation
    void ActorInstance::UpdateWorldTransform()
    {
        m_worldTransform = m_localTransform;
        m_worldTransform.Multiply(m_parentWorldTransform);
        m_worldTransformInv = m_worldTransform.Inversed();
    }

    // updates the skinning matrices of all nodes
    void ActorInstance::UpdateSkinningMatrices()
    {
        AZ_PROFILE_SCOPE(Animation, "ActorInstance::UpdateSkinningMatrices");

        AZ::Matrix3x4* skinningMatrices = m_transformData->GetSkinningMatrices();
        const Pose* pose = m_transformData->GetCurrentPose();

        const size_t numNodes = GetNumEnabledNodes();
        for (size_t i = 0; i < numNodes; ++i)
        {
            const size_t nodeNumber = GetEnabledNode(i);
            Transform skinningTransform = m_actor->GetInverseBindPoseTransform(nodeNumber);
            skinningTransform.Multiply(pose->GetModelSpaceTransform(nodeNumber));
            skinningMatrices[nodeNumber] = AZ::Matrix3x4::CreateFromTransform(skinningTransform.ToAZTransform());
        }
    }

    // Update the mesh deformers, which updates the vertex positions on the CPU, so performing CPU skinning and morphing etc.
    void ActorInstance::UpdateMeshDeformers(float timePassedInSeconds, bool processDisabledDeformers)
    {
        timePassedInSeconds *= GetEMotionFX().GetGlobalSimulationSpeed();

        // Update the mesh deformers.
        const Skeleton* skeleton = m_actor->GetSkeleton();
        for (uint16 nodeNr : m_enabledNodes)
        {
            Node* node = skeleton->GetNode(nodeNr);
            MeshDeformerStack* stack = m_actor->GetMeshDeformerStack(m_lodLevel, nodeNr);
            if (stack)
            {
                stack->Update(this, node, timePassedInSeconds, processDisabledDeformers);
            }
        }
    }

    // Update the mesh morph deformers, which updates the vertex positions on the CPU, so performing CPU morphing.
    void ActorInstance::UpdateMorphMeshDeformers(float timePassedInSeconds, bool processDisabledDeformers)
    {
        timePassedInSeconds *= GetEMotionFX().GetGlobalSimulationSpeed();

        // Update the mesh morph deformers.
        const Skeleton* skeleton = m_actor->GetSkeleton();
        for (uint16 nodeNr : m_enabledNodes)
        {
            Node* node = skeleton->GetNode(nodeNr);
            MeshDeformerStack* stack = m_actor->GetMeshDeformerStack(m_lodLevel, nodeNr);
            if (stack)
            {
                stack->UpdateByModifierType(this, node, timePassedInSeconds, MorphMeshDeformer::TYPE_ID, true, processDisabledDeformers);
            }
        }
    }

    void ActorInstance::PostPhysicsUpdate(float timePassedInSeconds)
    {
        if (m_ragdollInstance)
        {
            m_ragdollInstance->PostPhysicsUpdate(timePassedInSeconds);
        }
    }

    // add an attachment
    void ActorInstance::AddAttachment(Attachment* attachment)
    {
        AZ_Assert(attachment, "Attachment cannot be a nullptr");
        AZ_Assert(attachment->GetAttachmentActorInstance() != this, "Cannot attach to itself.");

        // first remove the current attachment tree from the scheduler
        ActorInstance* root = FindAttachmentRoot();
        GetActorManager().GetScheduler()->RecursiveRemoveActorInstance(root);

        // add the attachment
        m_attachments.emplace_back(attachment);
        ActorInstance* attachmentActorInstance = attachment->GetAttachmentActorInstance();
        if (attachmentActorInstance)
        {
            attachmentActorInstance->IncreaseNumAttachmentRefs();
            attachmentActorInstance->SetAttachedTo(this);
            GetActorManager().UpdateActorInstanceStatus(attachmentActorInstance);
        }

        // and re-add the root to the scheduler
        //GetActorManager().GetScheduler()->RecursiveInsertActorInstance(root, 0);
        // re-add the root if it was visible already
        //if (root->GetIsVisible())
        GetActorManager().GetScheduler()->RecursiveInsertActorInstance(root);
    }

    // try to find the attachment number for a given actor instance
    size_t ActorInstance::FindAttachmentNr(ActorInstance* actorInstance)
    {
        // for all attachments
        const auto foundAttachment = AZStd::find_if(m_attachments.begin(), m_attachments.end(), [actorInstance](const Attachment* attachment)
        {
            return attachment->GetAttachmentActorInstance() == actorInstance;
        });

        return foundAttachment != m_attachments.end() ? AZStd::distance(m_attachments.begin(), foundAttachment) : InvalidIndex;
    }

    // remove an attachment by actor instance pointer
    bool ActorInstance::RemoveAttachment(ActorInstance* actorInstance, bool delFromMem)
    {
        // try to find the attachment
        const size_t attachmentNr = FindAttachmentNr(actorInstance);
        if (attachmentNr == InvalidIndex)
        {
            return false;
        }

        // remove the actual attachment
        RemoveAttachment(attachmentNr, delFromMem);
        return true;
    }

    // remove an attachment
    void ActorInstance::RemoveAttachment(size_t nr, bool delFromMem)
    {
        MCORE_ASSERT(nr < m_attachments.size());

        // first remove the current attachment tree from the scheduler
        ActorInstance* root = FindAttachmentRoot();
        GetActorManager().GetScheduler()->RecursiveRemoveActorInstance(root);

        // get the attachment
        Attachment* attachment = m_attachments[nr];

        // its not an attachment anymore
        ActorInstance* attachmentInstance = attachment->GetAttachmentActorInstance();
        if (attachmentInstance)
        {
            attachmentInstance->SetSelfAttachment(nullptr);

            // unregister the attachment inside the actor instance that represents the attachment
            attachmentInstance->DecreaseNumAttachmentRefs();
            attachmentInstance->SetAttachedTo(nullptr);
            GetActorManager().UpdateActorInstanceStatus(attachmentInstance);

            attachmentInstance->SetParentWorldSpaceTransform(Transform::CreateIdentity());
        }

        // delete it from memory when desired
        if (delFromMem)
        {
            attachment->Destroy();
        }

        // remove it from the attachment list
        m_attachments.erase(AZStd::next(begin(m_attachments), nr));

        // and re-add the root to the scheduler
        GetActorManager().GetScheduler()->RecursiveInsertActorInstance(root, 0);

        // re-add the attachment
        if (attachmentInstance)
        {
            GetActorManager().GetScheduler()->RecursiveInsertActorInstance(attachmentInstance, 0);
        }
    }

    // remove all attachments
    void ActorInstance::RemoveAllAttachments(bool delFromMem)
    {
        // keep removing the last attachment until there are none left
        while (m_attachments.size())
        {
            RemoveAttachment(m_attachments.size() - 1, delFromMem);
        }
    }

    // update the dependencies
    void ActorInstance::UpdateDependencies()
    {
        // get rid of existing dependencies
        m_dependencies.clear();

        // add the main dependency
        Actor::Dependency mainDependency;
        mainDependency.m_actor = m_actor;
        mainDependency.m_animGraph = (m_animGraphInstance) ? m_animGraphInstance->GetAnimGraph() : nullptr;
        m_dependencies.emplace_back(mainDependency);

        // add all dependencies stored inside the actor
        const size_t numDependencies = m_actor->GetNumDependencies();
        for (size_t i = 0; i < numDependencies; ++i)
        {
            m_dependencies.emplace_back(*m_actor->GetDependency(i));
        }
    }

    // set the attachment matrices
    void ActorInstance::UpdateAttachments()
    {
        for (Attachment* attachment : m_attachments)
        {
            attachment->Update();
        }
    }

    // find the root attachment actor instance, for example if you have a
    // knight with knife attached to his hands, where the knight is attached to a horse, the horse will be the
    // attachment root
    ActorInstance* ActorInstance::FindAttachmentRoot() const
    {
        if (m_attachedTo)
        {
            return m_attachedTo->FindAttachmentRoot();
        }

        return const_cast<ActorInstance*>(this);
    }

    // change visibility state
    void ActorInstance::SetIsVisible(bool isVisible)
    {
        // if the state doesn't change, do nothing
        if (isVisible == GetIsVisible())
        {
            return;
        }

        // if we change visibility state
        SetFlag(BOOL_ISVISIBLE, isVisible);
    }

    // update the bounding volume
    void ActorInstance::UpdateBounds(size_t geomLODLevel, EBoundsType boundsType, uint32 itemFrequency)
    {
        AZ_PROFILE_SCOPE(Animation, "ActorInstance::UpdateBounds");

        // depending on the bounding volume update type
        switch (boundsType)
        {
            // calculate the static based AABB
            case BOUNDS_STATIC_BASED:
                CalcStaticBasedAabb(&m_aabb);
                break;

            // based on the world space positions of the nodes (least accurate, but fastest)
            case BOUNDS_NODE_BASED:
                CalcNodeBasedAabb(&m_aabb, itemFrequency);
                break;

            // based on the world space positions of the vertices of the meshes (most accurate)
            case BOUNDS_MESH_BASED:
                UpdateMeshDeformers(0.0f);
                CalcMeshBasedAabb(geomLODLevel, &m_aabb, itemFrequency);
                break;

            // when we're dealing with an unspecified bounding volume update method
            default:
                MCore::LogInfo("*** EMotionFX::ActorInstance::UpdateBounds() - Unknown boundsType specified! (%d) ***", (uint32)boundsType);
        }

        // Expand the bounding volume by a tolerance area in case set.
        if (!AZ::IsClose(m_boundsExpandBy, 0.0f) && m_aabb.IsValid())
        {
            const AZ::Vector3 center = m_aabb.GetCenter();
            const AZ::Vector3 halfExtents = m_aabb.GetExtents() * 0.5f;
            const AZ::Vector3 scaledHalfExtents = halfExtents * (1.0f + m_boundsExpandBy);
            m_aabb.SetMin(center - scaledHalfExtents);
            m_aabb.SetMax(center + scaledHalfExtents);
        }
    }

    // calculate the axis aligned bounding box based on the world space positions of the nodes
    void ActorInstance::CalcNodeBasedAabb(AZ::Aabb* outResult, uint32 nodeFrequency)
    {
        *outResult = AZ::Aabb::CreateNull();

        const Pose* pose = m_transformData->GetCurrentPose();
        const Skeleton* skeleton = m_actor->GetSkeleton();

        // for all nodes, encapsulate the world space positions
        const size_t numNodes = GetNumEnabledNodes();
        for (size_t i = 0; i < numNodes; i += nodeFrequency)
        {
            const uint16 nodeNr = GetEnabledNode(i);
            if (skeleton->GetNode(nodeNr)->GetIncludeInBoundsCalc())
            {
                outResult->AddPoint(pose->GetWorldSpaceTransform(nodeNr).m_position);
            }
        }
    }

    // calculate the AABB that contains all world space vertices of all meshes
    void ActorInstance::CalcMeshBasedAabb(size_t geomLODLevel, AZ::Aabb* outResult, uint32 vertexFrequency)
    {
        *outResult = AZ::Aabb::CreateNull();

        const Pose* pose = m_transformData->GetCurrentPose();
        const Skeleton* skeleton = m_actor->GetSkeleton();

        // for all nodes, encapsulate the world space positions
        const size_t numNodes = GetNumEnabledNodes();
        for (size_t i = 0; i < numNodes; ++i)
        {
            const uint16 nodeNr = GetEnabledNode(i);
            Node* node = skeleton->GetNode(nodeNr);

            // skip nodes without meshes
            Mesh* mesh = m_actor->GetMesh(geomLODLevel, nodeNr);
            if (mesh == nullptr)
            {
                continue;
            }

            // if this node should be excluded
            if (node->GetIncludeInBoundsCalc() == false)
            {
                continue;
            }

            const Transform worldTransform = pose->GetMeshNodeWorldSpaceTransform(geomLODLevel, nodeNr);

            // calculate and encapsulate the mesh bounds inside the total mesh box
            AZ::Aabb meshBox;
            mesh->CalcAabb(&meshBox, worldTransform, vertexFrequency);
            outResult->AddAabb(meshBox);
        }
    }

    // setup bounding volume auto update settings
    void ActorInstance::SetupAutoBoundsUpdate(float updateFrequencyInSeconds, EBoundsType boundsType, uint32 itemFrequency)
    {
        MCORE_ASSERT(itemFrequency > 0); // zero would cause an infinite loop
        m_boundsUpdateFrequency = updateFrequencyInSeconds;
        m_boundsUpdateType = boundsType;
        m_boundsUpdateItemFreq = itemFrequency;
        SetBoundsUpdateEnabled(true);
    }

    // apply the morph setup
    void ActorInstance::ApplyMorphSetup()
    {
        // get the morph setup instance and original setup
        MorphSetupInstance* morphSetupInstance = GetMorphSetupInstance();
        if (morphSetupInstance == nullptr)
        {
            return;
        }

        // if there is no morph setup, we have nothing to do
        MorphSetup* morphSetup = m_actor->GetMorphSetup(m_lodLevel);
        if (morphSetup == nullptr)
        {
            return;
        }

        // apply all morph targets
        //bool allZero = true;
        const size_t numTargets = morphSetup->GetNumMorphTargets();
        for (size_t i = 0; i < numTargets; ++i)
        {
            // get the morph target
            MorphTarget* morphTarget = morphSetup->GetMorphTarget(i);
            MorphSetupInstance::MorphTarget* morphTargetInstance = morphSetupInstance->FindMorphTargetByID(morphTarget->GetID());
            if (morphTargetInstance == nullptr)
            {
                continue;
            }

            // get the weight for this morph target
            const float weight = morphTargetInstance->GetWeight();

            // apply the morph target if its weight is non-zero
            if (MCore::Math::Abs(weight) > 0.0001f)
            {
                //allZero = false;
                morphTarget->Apply(this, weight);
            }
        }
    }

    //---------------------

    // check intersection with a ray, but don't get the intersection point or closest intersecting node
    Node* ActorInstance::IntersectsCollisionMesh(size_t lodLevel, const MCore::Ray& ray) const
    {
        const Skeleton* skeleton = m_actor->GetSkeleton();
        const Pose* pose = m_transformData->GetCurrentPose();

        // for all nodes
        const size_t numNodes = GetNumEnabledNodes();
        for (size_t i = 0; i < numNodes; ++i)
        {
            const uint16 nodeNr = GetEnabledNode(i);

            // check if there is a mesh for this node
            Mesh* mesh = m_actor->GetMesh(lodLevel, nodeNr);
            if (mesh == nullptr)
            {
                continue;
            }

            if (mesh->GetIsCollisionMesh() == false)
            {
                continue;
            }

            const Transform worldTransform = pose->GetMeshNodeWorldSpaceTransform(lodLevel, nodeNr);

            // return when there is an intersection
            if (mesh->Intersects(worldTransform, ray))
            {
                return skeleton->GetNode(nodeNr);
            }
        }

        return nullptr;
    }

    Node* ActorInstance::IntersectsCollisionMesh(size_t lodLevel, const MCore::Ray& ray, AZ::Vector3* outIntersect, AZ::Vector3* outNormal, AZ::Vector2* outUV, float* outBaryU, float* outBaryV, uint32* outIndices) const
    {
        Node* closestNode = nullptr;
        AZ::Vector3 point;
        AZ::Vector3 closestPoint(0.0f, 0.0f, 0.0f);
        float dist, baryU, baryV, closestBaryU = 0, closestBaryV = 0;
        float closestDist = FLT_MAX;
        Transform closestTransform = Transform::CreateIdentity();
        uint32 closestIndices[3];
        uint32 triIndices[3];

        const Skeleton* skeleton = m_actor->GetSkeleton();
        const Pose* pose = m_transformData->GetCurrentPose();

        // check all nodes
        const size_t numNodes = GetNumEnabledNodes();
        for (size_t i = 0; i < numNodes; i++)
        {
            const uint16 nodeNr = GetEnabledNode(i);
            Node* curNode = skeleton->GetNode(nodeNr);
            Mesh* mesh = m_actor->GetMesh(lodLevel, nodeNr);
            if (mesh == nullptr)
            {
                continue;
            }

            if (mesh->GetIsCollisionMesh() == false)
            {
                continue;
            }

            const Transform worldTransform = pose->GetMeshNodeWorldSpaceTransform(lodLevel, nodeNr);

            // if there is an intersection between the ray and the mesh
            if (mesh->Intersects(worldTransform, ray, &point, &baryU, &baryV, triIndices))
            {
                // calculate the squared distance between the intersection point and the ray origin
                dist = (point - ray.GetOrigin()).GetLengthSq();

                // if it is the closest point till now, record it as closest
                if (dist < closestDist)
                {
                    closestTransform = worldTransform;
                    closestPoint = point;
                    closestDist = dist;
                    closestNode = curNode;
                    closestBaryU = baryU;
                    closestBaryV = baryV;
                    closestIndices[0] = triIndices[0];
                    closestIndices[1] = triIndices[1];
                    closestIndices[2] = triIndices[2];
                }
            }
        }

        // output the closest values
        if (closestNode)
        {
            if (outIntersect)
            {
                *outIntersect = closestPoint;
            }

            if (outBaryU)
            {
                *outBaryU = closestBaryU;
            }

            if (outBaryV)
            {
                *outBaryV = closestBaryV;
            }

            if (outIndices)
            {
                outIndices[0] = closestIndices[0];
                outIndices[1] = closestIndices[1];
                outIndices[2] = closestIndices[2];
            }

            // calculate the interpolated normal
            if (outNormal || outUV)
            {
                Mesh* mesh = m_actor->GetMesh(lodLevel, closestNode->GetNodeIndex());

                // calculate the normal at the intersection point
                if (outNormal)
                {
                    AZ::Vector3* normals = (AZ::Vector3*)mesh->FindVertexData(Mesh::ATTRIB_NORMALS);
                    AZ::Vector3  norm = MCore::BarycentricInterpolate<AZ::Vector3>(closestBaryU, closestBaryV, normals[closestIndices[0]], normals[closestIndices[1]], normals[closestIndices[2]]);
                    norm = closestTransform.TransformVector(norm);
                    norm.Normalize();
                    *outNormal = norm;
                }

                // calculate the UV coordinate at the intersection point
                if (outUV)
                {
                    // get the UV coordinates of the first layer
                    AZ::Vector2* uvData = static_cast<AZ::Vector2*>(mesh->FindVertexData(Mesh::ATTRIB_UVCOORDS, 0));
                    if (uvData)
                    {
                        // calculate the interpolated texture coordinate
                        *outUV = MCore::BarycentricInterpolate<AZ::Vector2>(closestBaryU, closestBaryV, uvData[closestIndices[0]], uvData[closestIndices[1]], uvData[closestIndices[2]]);
                    }
                }
            }
        }

        // return the result
        return closestNode;
    }

    // check intersection with a ray, but don't get the intersection point or closest intersecting node
    Node* ActorInstance::IntersectsMesh(size_t lodLevel, const MCore::Ray& ray) const
    {
        const Pose* pose = m_transformData->GetCurrentPose();
        const Skeleton* skeleton = m_actor->GetSkeleton();

        // for all nodes
        const size_t numNodes = GetNumEnabledNodes();
        for (size_t i = 0; i < numNodes; ++i)
        {
            const uint16 nodeNr = GetEnabledNode(i);
            Node* node = skeleton->GetNode(nodeNr);

            // check if there is a mesh for this node
            Mesh* mesh = m_actor->GetMesh(lodLevel, nodeNr);
            if (mesh == nullptr)
            {
                continue;
            }

            const Transform worldTransform = pose->GetMeshNodeWorldSpaceTransform(lodLevel, nodeNr);

            // return when there is an intersection
            if (mesh->Intersects(worldTransform, ray))
            {
                return node;
            }
        }

        // no intersection found
        return nullptr;
    }

    void ActorInstance::SetRagdoll(Physics::Ragdoll* ragdoll)
    {
        if (ragdoll && ragdoll->GetNumNodes() > 0)
        {
            m_ragdollInstance = AZStd::make_unique<RagdollInstance>(ragdoll, this);
        }
        else
        {
            m_ragdollInstance.reset();
        }
    }

    RagdollInstance* ActorInstance::GetRagdollInstance() const
    {
        return m_ragdollInstance.get();
    }

    // intersection test that returns the closest intersection
    Node* ActorInstance::IntersectsMesh(size_t lodLevel, const MCore::Ray& ray, AZ::Vector3* outIntersect, AZ::Vector3* outNormal, AZ::Vector2* outUV, float* outBaryU, float* outBaryV, uint32* outIndices) const
    {
        Node* closestNode = nullptr;
        AZ::Vector3 point;
        AZ::Vector3 closestPoint(0.0f, 0.0f, 0.0f);
        Transform closestTransform = Transform::CreateIdentity();
        float dist, baryU, baryV, closestBaryU = 0, closestBaryV = 0;
        float closestDist = FLT_MAX;
        uint32 closestIndices[3];
        uint32 triIndices[3];

        const Pose* pose = m_transformData->GetCurrentPose();
        const Skeleton* skeleton = m_actor->GetSkeleton();

        // check all nodes
        const size_t numNodes = GetNumEnabledNodes();
        for (size_t i = 0; i < numNodes; i++)
        {
            const uint16 nodeNr = GetEnabledNode(i);
            Node* curNode = skeleton->GetNode(nodeNr);
            Mesh* mesh = m_actor->GetMesh(lodLevel, nodeNr);
            if (mesh == nullptr)
            {
                continue;
            }

            const Transform worldTransform = pose->GetMeshNodeWorldSpaceTransform(lodLevel, nodeNr);

            // if there is an intersection between the ray and the mesh
            if (mesh->Intersects(worldTransform, ray, &point, &baryU, &baryV, triIndices))
            {
                // calculate the squared distance between the intersection point and the ray origin
                dist = (point - ray.GetOrigin()).GetLengthSq();

                // if it is the closest point till now, record it as closest
                if (dist < closestDist)
                {
                    closestTransform = worldTransform;
                    closestPoint = point;
                    closestDist = dist;
                    closestNode = curNode;
                    closestBaryU = baryU;
                    closestBaryV = baryV;
                    closestIndices[0] = triIndices[0];
                    closestIndices[1] = triIndices[1];
                    closestIndices[2] = triIndices[2];
                }
            }
        }

        // output the closest values
        if (closestNode)
        {
            if (outIntersect)
            {
                *outIntersect = closestPoint;
            }

            if (outBaryU)
            {
                *outBaryU = closestBaryU;
            }

            if (outBaryV)
            {
                *outBaryV = closestBaryV;
            }

            if (outIndices)
            {
                outIndices[0] = closestIndices[0];
                outIndices[1] = closestIndices[1];
                outIndices[2] = closestIndices[2];
            }

            // calculate the interpolated normal
            if (outNormal || outUV)
            {
                Mesh* mesh = m_actor->GetMesh(lodLevel, closestNode->GetNodeIndex());

                // calculate the normal at the intersection point
                if (outNormal)
                {
                    AZ::Vector3* normals = (AZ::Vector3*)mesh->FindVertexData(Mesh::ATTRIB_NORMALS);
                    AZ::Vector3  norm = MCore::BarycentricInterpolate<AZ::Vector3>(
                            closestBaryU, closestBaryV,
                            normals[closestIndices[0]], normals[closestIndices[1]], normals[closestIndices[2]]);                   
                    norm = closestTransform.TransformVector(norm);
                    norm.Normalize();
                    *outNormal = norm;
                }

                // calculate the UV coordinate at the intersection point
                if (outUV)
                {
                    // get the UV coordinates of the first layer
                    AZ::Vector2* uvData = static_cast<AZ::Vector2*>(mesh->FindVertexData(Mesh::ATTRIB_UVCOORDS, 0));
                    if (uvData)
                    {
                        // calculate the interpolated texture coordinate
                        *outUV = MCore::BarycentricInterpolate<AZ::Vector2>(closestBaryU, closestBaryV, uvData[closestIndices[0]], uvData[closestIndices[1]], uvData[closestIndices[2]]);
                    }
                }
            }
        }

        // return the result
        return closestNode;
    }

    //---------------------

    //
    void ActorInstance::EnableNode(uint16 nodeIndex)
    {
        // if this node already is at an enabled state, do nothing
        if (AZStd::find(begin(m_enabledNodes), end(m_enabledNodes), nodeIndex) != end(m_enabledNodes))
        {
            return;
        }

        Skeleton* skeleton = m_actor->GetSkeleton();

        // find the location where to insert (as the flattened hierarchy needs to be preserved in the array)
        bool found = false;
        size_t curNode = nodeIndex;
        do
        {
            // get the parent of the current node
            size_t parentIndex = skeleton->GetNode(curNode)->GetParentIndex();
            if (parentIndex != InvalidIndex)
            {
                const auto parentArrayIter = AZStd::find(begin(m_enabledNodes), end(m_enabledNodes), static_cast<uint16>(parentIndex));
                if (parentArrayIter != end(m_enabledNodes))
                {
                    if (parentArrayIter + 1 == end(m_enabledNodes))
                    {
                        m_enabledNodes.emplace_back(nodeIndex);
                    }
                    else
                    {
                        m_enabledNodes.emplace(parentArrayIter + 1, nodeIndex);
                    }
                    found = true;
                }
                else
                {
                    curNode = parentIndex;
                }
            }
            else // if we're dealing with a root node, insert it in the front of the array
            {
                m_enabledNodes.emplace(AZStd::next(begin(m_enabledNodes), 0), nodeIndex);
                found = true;
            }
        } while (found == false);
    }

    // disable a given node
    void ActorInstance::DisableNode(uint16 nodeIndex)
    {
        // try to remove the node from the array
        const auto it = AZStd::find(begin(m_enabledNodes), end(m_enabledNodes), nodeIndex);
        if (it != end(m_enabledNodes))
        {
            m_enabledNodes.erase(it);
        }
    }

    // enable all nodes
    void ActorInstance::EnableAllNodes()
    {
        m_enabledNodes.resize(m_actor->GetNumNodes());
        std::iota(m_enabledNodes.begin(), m_enabledNodes.end(), uint16(0));
    }

    // disable all nodes
    void ActorInstance::DisableAllNodes()
    {
        m_enabledNodes.clear();
    }

    // change the skeletal LOD level
    void ActorInstance::SetSkeletalLODLevelNodeFlags(size_t level)
    {
        // make sure the lod level is in range of 0..63
        const size_t newLevel = MCore::Clamp<size_t>(level, 0, 63);

        // if the lod level is the same as it currently is, do nothing
        if (newLevel == m_lodLevel)
        {
            return;
        }

        Skeleton* skeleton = m_actor->GetSkeleton();

        // change the state of all nodes that need state changes
        const size_t numNodes = GetNumNodes();
        for (size_t i = 0; i < numNodes; ++i)
        {
            Node* node = skeleton->GetNode(i);

            // check the curent and the new enabled state
            const bool curEnabled = node->GetSkeletalLODStatus(m_lodLevel);
            const bool newEnabled = node->GetSkeletalLODStatus(newLevel);

            // if the state changed, enable or disable it
            if (curEnabled != newEnabled)
            {
                if (newEnabled) // if the new LOD says that this node should be enabled, enable it
                {
                    EnableNode(static_cast<uint16>(i));
                }
                else // otherwise disable it
                {
                    DisableNode(static_cast<uint16>(i));
                }
            }
        }
    }

    void ActorInstance::SetLODLevel(size_t level)
    {
        m_requestedLODLevel = level;
    }

    void ActorInstance::UpdateLODLevel()
    {
        // Switch LOD level in case a change was requested.
        if (m_lodLevel != m_requestedLODLevel)
        {
            // Enable and disable all nodes accordingly (do not call this after setting the new m_lodLevel)
            SetSkeletalLODLevelNodeFlags(m_requestedLODLevel);

            // Make sure the LOD level is valid and update it.
            m_lodLevel = MCore::Clamp<size_t>(m_requestedLODLevel, 0, m_actor->GetNumLODLevels() - 1);
        }
    }

    // enable or disable nodes based on the skeletal LOD flags
    void ActorInstance::UpdateSkeletalLODFlags()
    {
        // change the state of all nodes that need state changes
        Skeleton* skeleton = m_actor->GetSkeleton();
        const size_t numNodes = skeleton->GetNumNodes();
        for (size_t i = 0; i < numNodes; ++i)
        {
            Node* node = skeleton->GetNode(i);

            // if the new LOD says that this node should be enabled, enable it
            if (node->GetSkeletalLODStatus(m_lodLevel))
            {
                EnableNode(static_cast<uint16>(i));
            }
            else // otherwise disable it
            {
                DisableNode(static_cast<uint16>(i));
            }
        }
    }

    // calculate the number of disabled nodes for a given skeletal lod level
    size_t ActorInstance::CalcNumDisabledNodes(size_t skeletalLODLevel) const
    {
        uint32 numDisabledNodes = 0;

        const Skeleton* skeleton = m_actor->GetSkeleton();

        // get the number of nodes and iterate through them
        const size_t numNodes = GetNumNodes();
        for (size_t i = 0; i < numNodes; ++i)
        {
            // get the current node
            Node* node = skeleton->GetNode(i);

            // check if the current node is disabled in the given skeletal lod level, if yes increase the resulting number
            if (node->GetSkeletalLODStatus(skeletalLODLevel) == false)
            {
                numDisabledNodes++;
            }
        }

        return numDisabledNodes;
    }

    // calculate the number of skeletal LOD levels
    size_t ActorInstance::CalcNumSkeletalLODLevels() const
    {
        size_t numSkeletalLODLevels = 0;

        // iterate over all skeletal LOD levels
        size_t currentNumDisabledNodes = 0;
        size_t previousNumDisabledNodes = InvalidIndex;
        for (size_t i = 0; i < sizeof(size_t) * 8; ++i)
        {
            // get the number of disabled nodes in the current skeletal LOD level
            currentNumDisabledNodes = CalcNumDisabledNodes(i);

            // compare the number of disabled nodes from the current skeletal LOD level with the previous one and increase the number of
            // skeletal LOD levels in case they are not equal, if they are equal we have reached the last skeletal LOD level and we can skip the further calculations
            if (previousNumDisabledNodes != currentNumDisabledNodes)
            {
                numSkeletalLODLevels++;
                previousNumDisabledNodes = currentNumDisabledNodes;
            }
            else
            {
                break;
            }
        }

        return numSkeletalLODLevels;
    }

    // change the current motion system
    void ActorInstance::SetMotionSystem(MotionSystem* newSystem, bool delCurrentFromMem)
    {
        if (delCurrentFromMem && m_motionSystem)
        {
            m_motionSystem->Destroy();
        }

        m_motionSystem = newSystem;
    }

    // check if this actor instance is a skin attachment
    bool ActorInstance::GetIsSkinAttachment() const
    {
        if (m_selfAttachment == nullptr)
        {
            return false;
        }

        return m_selfAttachment->GetIsInfluencedByMultipleJoints();
    }

    // draw a skeleton using lines, calling the drawline callbacks in the event handlers
    void ActorInstance::DrawSkeleton(const Pose& pose, const AZ::Color& color)
    {
        DebugDraw& debugDraw = GetDebugDraw();
        DebugDraw::ActorInstanceData* drawData = debugDraw.GetActorInstanceData(this);
        drawData->Lock();
        drawData->DrawPose(pose, color);
        drawData->Unlock();
    }

    // Remove the trajectory transform from the input transformation.
    void ActorInstance::MotionExtractionCompensate(Transform& inOutMotionExtractionNodeTransform, const Transform& localSpaceBindPoseTransform, EMotionExtractionFlags motionExtractionFlags)
    {
        Transform trajectoryTransform = inOutMotionExtractionNodeTransform;

        // Make sure the z axis is really pointing up and project it onto the ground plane.
        const AZ::Vector3 forwardAxis = MCore::CalcForwardAxis(trajectoryTransform.m_rotation);
        if (forwardAxis.GetZ() > 0.0f) // Pick the closest, so if we point more upwards already, we take 1.0, otherwise take -1.0. Sometimes Y would point up, sometimes down.
        {
            MCore::RotateFromTo(trajectoryTransform.m_rotation, forwardAxis, AZ::Vector3(0.0f, 0.0f, 1.0f));
        }
        else
        {
            MCore::RotateFromTo(trajectoryTransform.m_rotation, forwardAxis, AZ::Vector3(0.0f, 0.0f, -1.0f));
        }

        trajectoryTransform.ApplyMotionExtractionFlags(motionExtractionFlags);

        // Get the projected bind pose transform.
        Transform bindTransformProjected = localSpaceBindPoseTransform;
        bindTransformProjected.ApplyMotionExtractionFlags(motionExtractionFlags);

        // Remove the projected rotation and translation from the transform to prevent the double transform.
        inOutMotionExtractionNodeTransform.m_rotation = (bindTransformProjected.m_rotation.GetConjugate() * trajectoryTransform.m_rotation).GetConjugate() * inOutMotionExtractionNodeTransform.m_rotation;
        inOutMotionExtractionNodeTransform.m_position = inOutMotionExtractionNodeTransform.m_position - (trajectoryTransform.m_position - bindTransformProjected.m_position);
        inOutMotionExtractionNodeTransform.m_rotation.Normalize();
    }

    void ActorInstance::MotionExtractionCompensate(Transform& inOutMotionExtractionNodeTransform, EMotionExtractionFlags motionExtractionFlags) const
    {
        MCORE_ASSERT(m_actor->GetMotionExtractionNodeIndex() != InvalidIndex);
        Transform bindPoseTransform = m_transformData->GetBindPose()->GetLocalSpaceTransform(m_actor->GetMotionExtractionNodeIndex());

        MotionExtractionCompensate(inOutMotionExtractionNodeTransform, bindPoseTransform, motionExtractionFlags);
    }

    // Remove the trajectory transform from the motion extraction node to prevent double transformation.
    void ActorInstance::MotionExtractionCompensate(EMotionExtractionFlags motionExtractionFlags)
    {
        const size_t motionExtractIndex = m_actor->GetMotionExtractionNodeIndex();
        if (motionExtractIndex == InvalidIndex)
        {
            return;
        }

        Pose* currentPose = m_transformData->GetCurrentPose();
        Transform transform = currentPose->GetLocalSpaceTransform(motionExtractIndex);
        MotionExtractionCompensate(transform, motionExtractionFlags);

        currentPose->SetLocalSpaceTransform(motionExtractIndex, transform);
    }

    void ActorInstance::ApplyMotionExtractionDelta(Transform& inOutTransform, const Transform& trajectoryDelta)
    {
        Transform curTransform = inOutTransform;
#ifndef EMFX_SCALE_DISABLED
        curTransform.m_position += trajectoryDelta.m_position * curTransform.m_scale;
#else
        curTransform.m_position += trajectoryDelta.m_position;
#endif

        curTransform.m_rotation *= trajectoryDelta.m_rotation;
        curTransform.m_rotation.Normalize();

        inOutTransform = curTransform;
    }

    // Apply the motion extraction delta transform to the actor instance.
    void ActorInstance::ApplyMotionExtractionDelta(const Transform& trajectoryDelta)
    {
        if (m_actor->GetMotionExtractionNodeIndex() == InvalidIndex)
        {
            return;
        }

        ApplyMotionExtractionDelta(m_localTransform, trajectoryDelta);
    }

    // apply the currently set motion extraction delta transform to the actor instance
    void ActorInstance::ApplyMotionExtractionDelta()
    {
        ApplyMotionExtractionDelta(m_trajectoryDelta);
    }

    void ActorInstance::SetMotionExtractionEnabled(bool enabled)
    {
        SetFlag(BOOL_MOTIONEXTRACTION, enabled);
    }

    bool ActorInstance::GetMotionExtractionEnabled() const
    {
        return (m_boolFlags & BOOL_MOTIONEXTRACTION) != 0;
    }

    // update the static based aabb dimensions
    void ActorInstance::UpdateStaticBasedAabbDimensions()
    {
        Transform orgTransform = GetLocalSpaceTransform();
        SetLocalSpacePosition(AZ::Vector3::CreateZero());
        EMFX_SCALECODE(SetLocalSpaceScale(AZ::Vector3(1.0f, 1.0f, 1.0f));)

        UpdateTransformations(0.0f, true);
        UpdateMeshDeformers(0.0f);

        // calculate the aabb of this
        if (m_actor->CheckIfHasMeshes(0))
        {
            CalcMeshBasedAabb(0, &m_staticAabb);
        }
        else
        {
            CalcNodeBasedAabb(&m_staticAabb);
        }

        m_localTransform = orgTransform;
    }

    // calculate the moved static based aabb
    void ActorInstance::CalcStaticBasedAabb(AZ::Aabb* outResult)
    {
        if (GetIsSkinAttachment())
        {
            m_selfAttachment->GetAttachToActorInstance()->CalcStaticBasedAabb(outResult);
            return;
        }

        *outResult = m_staticAabb;
        EMFX_SCALECODE(
            if (m_staticAabb.IsValid())
            {
                outResult->SetMin(m_staticAabb.GetMin() * m_worldTransform.m_scale);
                outResult->SetMax(m_staticAabb.GetMax() * m_worldTransform.m_scale);
            }
        )
        outResult->Translate(m_worldTransform.m_position);
    }

    // adjust the anim graph instance
    void ActorInstance::SetAnimGraphInstance(AnimGraphInstance* instance)
    {
        m_animGraphInstance = instance;
        UpdateDependencies();
    }

    Actor* ActorInstance::GetActor() const
    {
        return m_actor;
    }

    void ActorInstance::SetID(uint32 id)
    {
        m_id = id;
    }

    MotionSystem* ActorInstance::GetMotionSystem() const
    {
        return m_motionSystem;
    }

    size_t ActorInstance::GetLODLevel() const
    {
        return m_lodLevel;
    }

    void ActorInstance::SetCustomData(void* customData)
    {
        m_customData = customData;
    }

    void* ActorInstance::GetCustomData() const
    {
        return m_customData;
    }

    AZ::Entity* ActorInstance::GetEntity() const
    {
        return m_entity;
    }

    AZ::EntityId ActorInstance::GetEntityId() const
    {
        if (m_entity)
        {
            return m_entity->GetId();
        }

        return AZ::EntityId();
    }

    bool ActorInstance::GetBoundsUpdateEnabled() const
    {
        return (m_boolFlags & BOOL_BOUNDSUPDATEENABLED);
    }

    float ActorInstance::GetBoundsUpdateFrequency() const
    {
        return m_boundsUpdateFrequency;
    }

    float ActorInstance::GetBoundsUpdatePassedTime() const
    {
        return m_boundsUpdatePassedTime;
    }

    ActorInstance::EBoundsType ActorInstance::GetBoundsUpdateType() const
    {
        return m_boundsUpdateType;
    }

    uint32 ActorInstance::GetBoundsUpdateItemFrequency() const
    {
        return m_boundsUpdateItemFreq;
    }

    void ActorInstance::SetBoundsUpdateFrequency(float seconds)
    {
        m_boundsUpdateFrequency = seconds;
    }

    void ActorInstance::SetBoundsUpdatePassedTime(float seconds)
    {
        m_boundsUpdatePassedTime = seconds;
    }

    void ActorInstance::SetBoundsUpdateType(EBoundsType bType)
    {
        m_boundsUpdateType = bType;
    }

    void ActorInstance::SetBoundsUpdateItemFrequency(uint32 freq)
    {
        MCORE_ASSERT(freq >= 1);
        m_boundsUpdateItemFreq = freq;
    }

    void ActorInstance::SetBoundsUpdateEnabled(bool enable)
    {
        SetFlag(BOOL_BOUNDSUPDATEENABLED, enable);
    }

    void ActorInstance::SetStaticBasedAabb(const AZ::Aabb& aabb)
    {
        m_staticAabb = aabb;
    }

    void ActorInstance::GetStaticBasedAabb(AZ::Aabb* outAabb)
    {
        *outAabb = m_staticAabb;
    }

    const AZ::Aabb& ActorInstance::GetStaticBasedAabb() const
    {
        return m_staticAabb;
    }

    const AZ::Aabb& ActorInstance::GetAabb() const
    {
        return m_aabb;
    }

    void ActorInstance::SetAabb(const AZ::Aabb& aabb)
    {
        m_aabb = aabb;
    }

    size_t ActorInstance::GetNumAttachments() const
    {
        return m_attachments.size();
    }

    Attachment* ActorInstance::GetAttachment(size_t nr) const
    {
        return m_attachments[nr];
    }

    bool ActorInstance::GetIsAttachment() const
    {
        return (m_attachedTo != nullptr);
    }

    ActorInstance* ActorInstance::GetAttachedTo() const
    {
        return m_attachedTo;
    }

    Attachment* ActorInstance::GetSelfAttachment() const
    {
        return m_selfAttachment;
    }

    size_t ActorInstance::GetNumDependencies() const
    {
        return m_dependencies.size();
    }

    Actor::Dependency* ActorInstance::GetDependency(size_t nr)
    {
        return &m_dependencies[nr];
    }

    MorphSetupInstance* ActorInstance::GetMorphSetupInstance() const
    {
        return m_morphSetup;
    }

    void ActorInstance::SetParentWorldSpaceTransform(const Transform& transform)
    {
        m_parentWorldTransform = transform;
    }

    const Transform& ActorInstance::GetParentWorldSpaceTransform() const
    {
        return m_parentWorldTransform;
    }

    void ActorInstance::SetRender(bool enabled)
    {
        SetFlag(BOOL_RENDER, enabled);
    }

    bool ActorInstance::GetRender() const
    {
        return (m_boolFlags & BOOL_RENDER) != 0;
    }

    void ActorInstance::SetIsUsedForVisualization(bool enabled)
    {
        SetFlag(BOOL_USEDFORVISUALIZATION, enabled);
    }

    bool ActorInstance::GetIsUsedForVisualization() const
    {
        return (m_boolFlags & BOOL_USEDFORVISUALIZATION) != 0;
    }

    void ActorInstance::SetIsOwnedByRuntime(bool isOwnedByRuntime)
    {
#if defined(EMFX_DEVELOPMENT_BUILD)
        SetFlag(BOOL_OWNEDBYRUNTIME, isOwnedByRuntime);
#else
        AZ_UNUSED(isOwnedByRuntime);
#endif
    }

    bool ActorInstance::GetIsOwnedByRuntime() const
    {
#if defined(EMFX_DEVELOPMENT_BUILD)
        return (m_boolFlags & BOOL_OWNEDBYRUNTIME) != 0;
#else
        return true;
#endif
    }

    uint32 ActorInstance::GetThreadIndex() const
    {
        return m_threadIndex;
    }

    void ActorInstance::SetThreadIndex(uint32 index)
    {
        m_threadIndex = index;
    }

    void ActorInstance::SetTrajectoryDeltaTransform(const Transform& transform)
    {
        m_trajectoryDelta = transform;
    }

    const Transform& ActorInstance::GetTrajectoryDeltaTransform() const
    {
        return m_trajectoryDelta;
    }

    AnimGraphPose* ActorInstance::RequestPose(uint32 threadIndex)
    {
        return GetEMotionFX().GetThreadData(threadIndex)->GetPosePool().RequestPose(this);
    }

    void ActorInstance::FreePose(uint32 threadIndex, AnimGraphPose* pose)
    {
        GetEMotionFX().GetThreadData(threadIndex)->GetPosePool().FreePose(pose);
    }

    void ActorInstance::SetMotionSamplingTimer(float timeInSeconds)
    {
        m_motionSamplingTimer = timeInSeconds;
    }

    void ActorInstance::SetMotionSamplingRate(float updateRateInSeconds)
    {
        m_motionSamplingRate = updateRateInSeconds;
    }

    float ActorInstance::GetMotionSamplingTimer() const
    {
        return m_motionSamplingTimer;
    }

    float ActorInstance::GetMotionSamplingRate() const
    {
        return m_motionSamplingRate;
    }

    void ActorInstance::IncreaseNumAttachmentRefs(uint8 numToIncreaseWith)
    {
        m_numAttachmentRefs += numToIncreaseWith;
        MCORE_ASSERT(m_numAttachmentRefs == 0 || m_numAttachmentRefs == 1);
    }

    void ActorInstance::DecreaseNumAttachmentRefs(uint8 numToDecreaseWith)
    {
        m_numAttachmentRefs -= numToDecreaseWith;
        MCORE_ASSERT(m_numAttachmentRefs == 0 || m_numAttachmentRefs == 1);
    }

    uint8 ActorInstance::GetNumAttachmentRefs() const
    {
        return m_numAttachmentRefs;
    }

    void ActorInstance::SetAttachedTo(ActorInstance* actorInstance)
    {
        m_attachedTo = actorInstance;
    }

    void ActorInstance::SetSelfAttachment(Attachment* selfAttachment)
    {
        m_selfAttachment = selfAttachment;
    }

    void ActorInstance::EnableFlag(uint8 flag)
    {
        m_boolFlags |= flag;
    }

    void ActorInstance::DisableFlag(uint8 flag)
    {
        m_boolFlags &= ~flag;
    }

    void ActorInstance::SetFlag(uint8 flag, bool enabled)
    {
        if (enabled)
        {
            m_boolFlags |= flag;
        }
        else
        {
            m_boolFlags &= ~flag;
        }
    }

    void ActorInstance::RecursiveSetIsVisible(bool isVisible)
    {
        SetIsVisible(isVisible);

        // recurse to all child attachments
        for (Attachment* attachment : m_attachments)
        {
            attachment->GetAttachmentActorInstance()->RecursiveSetIsVisible(isVisible);
        }
    }

    void ActorInstance::RecursiveSetIsVisibleTowardsRoot(bool isVisible)
    {
        SetIsVisible(isVisible);
        if (m_selfAttachment)
        {
            m_selfAttachment->GetAttachToActorInstance()->RecursiveSetIsVisibleTowardsRoot(isVisible);
        }
    }

    void ActorInstance::SetIsEnabled(bool enabled)
    {
        SetFlag(BOOL_ENABLED, enabled);
    }

    // update the normal scale factor based on the bounds
    void ActorInstance::UpdateVisualizeScale()
    {
        m_visualizeScale = 0.0f;
        UpdateMeshDeformers(0.0f);

        AZ::Aabb box = AZ::Aabb::CreateNull();

        CalcNodeBasedAabb(&box);
        if (box.IsValid())
        {
            const float boxRadius = AZ::Vector3(box.GetMax() - box.GetMin()).GetLength() * 0.5f;
            m_visualizeScale = MCore::Max<float>(m_visualizeScale, boxRadius);
        }

        CalcMeshBasedAabb(0, &box);
        if (box.IsValid())
        {
            const float boxRadius = AZ::Vector3(box.GetMax() - box.GetMin()).GetLength() * 0.5f;
            m_visualizeScale = MCore::Max<float>(m_visualizeScale, boxRadius);
        }

        m_visualizeScale *= 0.01f;
    }

    // get the normal scale factor
    float ActorInstance::GetVisualizeScale() const
    {
        return m_visualizeScale;
    }

    // manually set the visualize scale factor
    void ActorInstance::SetVisualizeScale(float factor)
    {
        m_visualizeScale = factor;
    }

    // Recursively check if we have a given attachment in the hierarchy going downwards.
    bool ActorInstance::RecursiveHasAttachment(const ActorInstance* attachmentInstance) const
    {
        if (attachmentInstance == this)
        {
            return true;
        }

        // Iterate down the chain of attachments.
        const size_t numAttachments = GetNumAttachments();
        for (size_t i = 0; i < numAttachments; ++i)
        {
            if (GetAttachment(i)->GetAttachmentActorInstance()->RecursiveHasAttachment(attachmentInstance))
            {
                return true;
            }
        }

        return false;
    }

    // Check if we can safely attach an attachment that uses the specified actor instance.
    // This will check for infinite recursion/circular chains.
    bool ActorInstance::CheckIfCanHandleAttachment(const ActorInstance* attachmentInstance) const
    {
        if (RecursiveHasAttachment(attachmentInstance) || attachmentInstance->RecursiveHasAttachment(this))
        {
            return false;
        }

        return true;
    }
} // namespace EMotionFX
