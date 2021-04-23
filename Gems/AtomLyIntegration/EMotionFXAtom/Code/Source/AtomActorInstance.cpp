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

#include <AtomActorInstance.h>
#include <AtomActor.h>
#include <ActorAsset.h>

#include <Atom/Feature/SkinnedMesh/SkinnedMeshInputBuffers.h>
#include <Integration/System/SystemCommon.h>
#include <Integration/System/SystemComponent.h>
#include <EMotionFX/Source/ActorInstance.h>
#include <EMotionFX/Source/MorphSetup.h>
#include <EMotionFX/Source/MorphSetupInstance.h>
#include <EMotionFX/Source/MorphTargetStandard.h>
#include <EMotionFX/Source/TransformData.h>
#include <EMotionFX/Source/Skeleton.h>
#include <EMotionFX/Source/Mesh.h>
#include <EMotionFX/Source/Node.h>
#include <MCore/Source/AzCoreConversions.h>

#include <Atom/RPI.Public/Scene.h>

#include <AzCore/Casting/numeric_cast.h>
#include <AzCore/Component/EntityId.h>
#include <AzCore/Component/TickBus.h>
#include <AzCore/Math/Transform.h>
#include <AzCore/base.h>

namespace AZ
{
    namespace Render
    {
        AZ_CLASS_ALLOCATOR_IMPL(AtomActorInstance, EMotionFX::Integration::EMotionFXAllocator, 0)

        AtomActorInstance::AtomActorInstance(AZ::EntityId entityId,
            const EMotionFX::Integration::EMotionFXPtr<EMotionFX::ActorInstance>& actorInstance,
            const AZ::Data::Asset<EMotionFX::Integration::ActorAsset>& asset,
            [[maybe_unused]] const AZ::Transform& worldTransform,
            EMotionFX::Integration::SkinningMethod skinningMethod)
            : RenderActorInstance(asset, actorInstance.get(), entityId)
        {
            RenderActorInstance::SetSkinningMethod(skinningMethod);
            if (m_entityId.IsValid())
            {
                Activate();
                AzFramework::BoundsRequestBus::Handler::BusConnect(m_entityId);
            }
        }

        AtomActorInstance::~AtomActorInstance()
        {
            if (m_entityId.IsValid())
            {
                AzFramework::BoundsRequestBus::Handler::BusDisconnect();
                Deactivate();
            }

            Data::AssetBus::MultiHandler::BusDisconnect();
        }

        void AtomActorInstance::OnTick([[maybe_unused]] float timeDelta)
        {
            UpdateBounds();
        }

        void AtomActorInstance::UpdateBounds()
        {
            // Update RenderActorInstance world bounding box
            // The bounding box is moving with the actor instance. It is static in the way that it does not change shape.
            // The entity and actor transforms are kept in sync already.
            m_worldAABB = AZ::Aabb::CreateFromMinMax(m_actorInstance->GetAABB().GetMin(), m_actorInstance->GetAABB().GetMax());

            // Update RenderActorInstance local bounding box
            m_localAABB = AZ::Aabb::CreateFromMinMax(m_actorInstance->GetStaticBasedAABB().GetMin(), m_actorInstance->GetStaticBasedAABB().GetMax());

            AzFramework::EntityBoundsUnionRequestBus::Broadcast(
                &AzFramework::EntityBoundsUnionRequestBus::Events::RefreshEntityLocalBoundsUnion, m_entityId);
        }

        AZ::Aabb AtomActorInstance:: GetWorldBounds()
        {
            return m_worldAABB;
        }

        AZ::Aabb AtomActorInstance::GetLocalBounds()
        {
            return m_localAABB;
        }

        void AtomActorInstance::SetSkinningMethod(EMotionFX::Integration::SkinningMethod emfxSkinningMethod)
        {
            RenderActorInstance::SetSkinningMethod(emfxSkinningMethod);

            m_boneTransforms = CreateBoneTransformBufferFromActorInstance(m_actorInstance, emfxSkinningMethod);
            // Release the Atom skinned mesh and acquire a new one to apply the new skinning method
            UnregisterActor();
            RegisterActor();
        }

        SkinningMethod AtomActorInstance::GetAtomSkinningMethod() const
        {
            switch (GetSkinningMethod())
            {
            case EMotionFX::Integration::SkinningMethod::DualQuat:
                return SkinningMethod::DualQuaternion;
            case EMotionFX::Integration::SkinningMethod::Linear:
                return SkinningMethod::LinearSkinning;
            default:
                AZ_Error("AtomActorInstance", false, "Unsupported skinning method. Defaulting to linear");
            }

            return SkinningMethod::LinearSkinning;
        }

        AtomActor* AtomActorInstance::GetRenderActor() const
        {
            EMotionFX::Integration::ActorAsset* actorAsset = m_actorAsset.Get();
            if (!actorAsset)
            {
                AZ_Assert(false, "Actor asset is not loaded.");
                return nullptr;
            }

            AtomActor* renderActor = azdynamic_cast<AtomActor*>(actorAsset->GetRenderActor());
            if (!renderActor)
            {
                AZ_Assert(false, "Expecting a Atom render backend actor.");
                return nullptr;
            }

            return renderActor;
        }

        void AtomActorInstance::Activate()
        {
            m_skinnedMeshFeatureProcessor = RPI::Scene::GetFeatureProcessorForEntity<SkinnedMeshFeatureProcessorInterface>(m_entityId);
            AZ_Assert(m_skinnedMeshFeatureProcessor, "AtomActorInstance was unable to find a SkinnedMeshFeatureProcessor on the EntityContext provided.");

            m_meshFeatureProcessor = RPI::Scene::GetFeatureProcessorForEntity<MeshFeatureProcessorInterface>(m_entityId);
            AZ_Assert(m_meshFeatureProcessor, "AtomActorInstance was unable to find a MeshFeatureProcessor on the EntityContext provided.");

            m_transformInterface = TransformBus::FindFirstHandler(m_entityId);
            AZ_Warning("AtomActorInstance", m_transformInterface, "Unable to attach to a TransformBus handler. This skinned mesh will always be rendered at the origin.");

            SkinnedMeshFeatureProcessorNotificationBus::Handler::BusConnect();
            MaterialReceiverRequestBus::Handler::BusConnect(m_entityId);
            LmbrCentral::SkeletalHierarchyRequestBus::Handler::BusConnect(m_entityId);

            Create();
        }

        void AtomActorInstance::Deactivate()
        {
            SkinnedMeshOutputStreamNotificationBus::Handler::BusDisconnect();
            LmbrCentral::SkeletalHierarchyRequestBus::Handler::BusDisconnect();
            MaterialReceiverRequestBus::Handler::BusDisconnect();
            SkinnedMeshFeatureProcessorNotificationBus::Handler::BusDisconnect();

            Destroy();

            m_meshFeatureProcessor = nullptr;
            m_skinnedMeshFeatureProcessor = nullptr;
        }

        MaterialAssignmentMap AtomActorInstance::GetMaterialAssignments() const
        {
            if (m_skinnedMeshInstance && m_skinnedMeshInstance->m_model)
            {
                return GetMaterialAssignmentsFromModel(m_skinnedMeshInstance->m_model);
            }

            return MaterialAssignmentMap{};
        }

        AZStd::unordered_set<AZ::Name> AtomActorInstance::GetModelUvNames() const
        {
            if (m_skinnedMeshInstance && m_skinnedMeshInstance->m_model)
            {
                return m_skinnedMeshInstance->m_model->GetUvNames();
            }
            return AZStd::unordered_set<AZ::Name>();
        }

        void AtomActorInstance::OnTransformChanged(const AZ::Transform& /*local*/, const AZ::Transform& world)
        {
            // The mesh transform is used to determine where the actor instance is actually rendered
            m_meshFeatureProcessor->SetTransform(*m_meshHandle, world); // handle validity is checked internally.

            if (m_skinnedMeshRenderProxy.IsValid())
            {
                // The skinned mesh transform is used to determine which Lod needs to be skinned
                m_skinnedMeshRenderProxy->SetTransform(world);
            }
        }

        void AtomActorInstance::OnMaterialsUpdated(const MaterialAssignmentMap& materials)
        {
            if (m_meshFeatureProcessor)
            {
                m_meshFeatureProcessor->SetMaterialAssignmentMap(*m_meshHandle, materials);
            }
        }

        void AtomActorInstance::SetModelAsset([[maybe_unused]] Data::Asset<RPI::ModelAsset> modelAsset)
        {
            // Changing model asset is not supported by Atom Actor Instance.
            // The model asset is obtained from the Actor inside the ActorAsset,
            // which is passed to the constructor. To set a different model asset
            // this instance should use a different Actor.
            AZ_Assert(false, "AtomActorInstance::SetModelAsset not supported");
        }

        Data::Asset<const RPI::ModelAsset> AtomActorInstance::GetModelAsset() const
        {
            AZ_Assert(GetActor(), "Expecting a Atom Actor Instance having a valid Actor.");
            return GetActor()->GetMeshAsset();
        }

        void AtomActorInstance::SetModelAssetId([[maybe_unused]] Data::AssetId modelAssetId)
        {
            // Changing model asset is not supported by Atom Actor Instance.
            // The model asset is obtained from the Actor inside the ActorAsset,
            // which is passed to the constructor. To set a different model asset
            // this instance should use a different Actor.
            AZ_Assert(false, "AtomActorInstance::SetModelAssetId not supported");
        }

        Data::AssetId AtomActorInstance::GetModelAssetId() const
        {
            return GetModelAsset().GetId();
        }

        void AtomActorInstance::SetModelAssetPath([[maybe_unused]] const AZStd::string& modelAssetPath)
        {
            // Changing model asset is not supported by Atom Actor Instance.
            // The model asset is obtained from the Actor inside the ActorAsset,
            // which is passed to the constructor. To set a different model asset
            // this instance should use a different Actor.
            AZ_Assert(false, "AtomActorInstance::SetModelAssetPath not supported");
        }

        AZStd::string AtomActorInstance::GetModelAssetPath() const
        {
            return GetModelAsset().GetHint();
        }

        AZ::Data::Instance<RPI::Model> AtomActorInstance::GetModel() const
        {
            return m_skinnedMeshInstance->m_model;
        }

        void AtomActorInstance::SetSortKey(RHI::DrawItemSortKey sortKey)
        {
            m_meshFeatureProcessor->SetSortKey(*m_meshHandle, sortKey);
        }

        RHI::DrawItemSortKey AtomActorInstance::GetSortKey() const
        {
            return m_meshFeatureProcessor->GetSortKey(*m_meshHandle);
        }
        
        void AtomActorInstance::SetLodOverride(RPI::Cullable::LodOverride lodOverride)
        {
            m_meshFeatureProcessor->SetLodOverride(*m_meshHandle, lodOverride);
        }

        RPI::Cullable::LodOverride AtomActorInstance::GetLodOverride() const
        {
            return m_meshFeatureProcessor->GetLodOverride(*m_meshHandle);
        }

        void AtomActorInstance::SetVisibility(bool visible)
        {
            SetIsVisible(visible);
        }

        bool AtomActorInstance::GetVisibility() const
        {
            return IsVisible();
        }

        AZ::u32 AtomActorInstance::GetJointCount()
        {
            return m_actorInstance->GetActor()->GetSkeleton()->GetNumNodes();
        }

        const char* AtomActorInstance::GetJointNameByIndex(AZ::u32 jointIndex)
        {
            EMotionFX::Skeleton* skeleton = m_actorInstance->GetActor()->GetSkeleton();
            const AZ::u32 numNodes = skeleton->GetNumNodes();
            if (jointIndex < numNodes)
            {
                return skeleton->GetNode(jointIndex)->GetName();
            }

            return nullptr;
        }

        AZ::s32 AtomActorInstance::GetJointIndexByName(const char* jointName)
        {
            if (jointName)
            {
                EMotionFX::Skeleton* skeleton = m_actorInstance->GetActor()->GetSkeleton();
                const AZ::u32 numNodes = skeleton->GetNumNodes();
                for (AZ::u32 nodeIndex = 0; nodeIndex < numNodes; ++nodeIndex)
                {
                    if (0 == azstricmp(jointName, skeleton->GetNode(nodeIndex)->GetName()))
                    {
                        return nodeIndex;
                    }
                }
            }

            return -1;
        }

        AZ::Transform AtomActorInstance::GetJointTransformCharacterRelative(AZ::u32 jointIndex)
        {
            const EMotionFX::TransformData* transforms = m_actorInstance->GetTransformData();
            if (transforms && jointIndex < transforms->GetNumTransforms())
            {
                return MCore::EmfxTransformToAzTransform(transforms->GetCurrentPose()->GetModelSpaceTransform(jointIndex));
            }

            return AZ::Transform::CreateIdentity();
        }

        void AtomActorInstance::Create()
        {
            Destroy();

            m_skinnedMeshInputBuffers = GetRenderActor()->FindOrCreateSkinnedMeshInputBuffers();
            AZ_Error("AtomActorInstance", m_skinnedMeshInputBuffers, "Failed to get SkinnedMeshInputBuffers from Actor.");
            if (m_skinnedMeshInputBuffers)
            {
                m_boneTransforms = CreateBoneTransformBufferFromActorInstance(m_actorInstance, GetSkinningMethod());
                AZ_Error("AtomActorInstance", m_boneTransforms, "Failed to create bone transform buffer.");

                // If the instance is created before the default materials on the model have finished loading, the mesh feature processor will ignore it.
                // Wait for them all to be ready before creating the instance
                size_t lodCount = m_skinnedMeshInputBuffers->GetLodCount();
                for (size_t lodIndex = 0; lodIndex < lodCount; ++lodIndex)
                {
                    const SkinnedMeshInputLod& inputLod = m_skinnedMeshInputBuffers->GetLod(lodIndex);
                    const AZStd::vector< SkinnedSubMeshProperties>& subMeshProperties = inputLod.GetSubMeshProperties();
                    for (const SkinnedSubMeshProperties& submesh : subMeshProperties)
                    {
                        AZ_Error("AtomActorInstance", submesh.m_material, "Actor does not have a valid default material in lod %d", lodIndex);
                        if (submesh.m_material)
                        {
                            if (!submesh.m_material->IsReady())
                            {
                                // Start listening for the material's OnAssetReady event.
                                // AtomActorInstance::Create is called on the main thread, so there should be no need to synchronize with the OnAssetReady event handler
                                // since those events will also come from the main thread
                                m_waitForMaterialLoadIds.insert(submesh.m_material->GetId());
                                Data::AssetBus::MultiHandler::BusConnect(submesh.m_material->GetId());
                            }
                        }
                    }
                }
                // If all the default materials are ready, create the skinned mesh instance
                if (m_waitForMaterialLoadIds.empty())
                {
                    CreateSkinnedMeshInstance();
                }
            }
        }

        void AtomActorInstance::OnAssetReady(AZ::Data::Asset<AZ::Data::AssetData> asset)
        {
            Data::AssetBus::MultiHandler::BusDisconnect(asset->GetId());
            m_waitForMaterialLoadIds.erase(asset->GetId());
            // If all the default materials are ready, create the skinned mesh instance
            if (m_waitForMaterialLoadIds.empty())
            {
                CreateSkinnedMeshInstance();
            }
        }

        void AtomActorInstance::Destroy()
        {
            if (m_skinnedMeshInstance)
            {
                UnregisterActor();
                m_skinnedMeshInputBuffers.reset();
                m_skinnedMeshInstance.reset();
                m_boneTransforms.reset();
            }
        }

        void AtomActorInstance::OnUpdateSkinningMatrices()
        {
            if (m_skinnedMeshRenderProxy.IsValid())
            {
                AZStd::vector<float> boneTransforms;
                GetBoneTransformsFromActorInstance(m_actorInstance, boneTransforms, GetSkinningMethod());

                m_skinnedMeshRenderProxy->SetSkinningMatrices(boneTransforms);

                // Update the morph weights for every lod. This does not mean they will all be dispatched, but they will all have up to date weights
                // TODO: once culling is hooked up such that EMotionFX and Atom are always in sync about which lod to update, only update the currently visible lods [ATOM-13564]
                for (uint32_t lodIndex = 0; lodIndex < m_actorInstance->GetActor()->GetNumLODLevels(); ++lodIndex)
                {
                    EMotionFX::MorphSetup* morphSetup = m_actorInstance->GetActor()->GetMorphSetup(lodIndex);
                    if (morphSetup)
                    {
                        uint32_t morphTargetCount = morphSetup->GetNumMorphTargets();
                        m_morphTargetWeights.clear();
                        for (uint32_t morphTargetIndex = 0; morphTargetIndex < morphTargetCount; ++morphTargetIndex)
                        {
                            EMotionFX::MorphTarget* morphTarget = morphSetup->GetMorphTarget(morphTargetIndex);
                            // check if we are dealing with a standard morph target
                            if (morphTarget->GetType() != EMotionFX::MorphTargetStandard::TYPE_ID)
                            {
                                continue;
                            }

                            // down cast the morph target
                            EMotionFX::MorphTargetStandard* morphTargetStandard = static_cast<EMotionFX::MorphTargetStandard*>(morphTarget);

                            EMotionFX::MorphSetupInstance::MorphTarget* morphTargetSetupInstance = m_actorInstance->GetMorphSetupInstance()->FindMorphTargetByID(morphTargetStandard->GetID());

                            // Each morph target is split into several deform datas, all of which share the same weight but have unique min/max delta values
                            // and thus correspond with unique dispatches in the morph target pass
                            for (uint32_t deformDataIndex = 0; deformDataIndex < morphTargetStandard->GetNumDeformDatas(); ++deformDataIndex)
                            {
                                // Morph targets that don't deform any vertices (e.g. joint-based morph targets) are not registered in the render proxy. Skip adding their weights.
                                const EMotionFX::MorphTargetStandard::DeformData* deformData = morphTargetStandard->GetDeformData(deformDataIndex);
                                if (deformData->mNumVerts > 0)
                                {
                                    m_morphTargetWeights.push_back(morphTargetSetupInstance->GetWeight());
                                }
                            }
                        }
                        m_skinnedMeshRenderProxy->SetMorphTargetWeights(lodIndex, m_morphTargetWeights);
                    }
                }
            }
        }

        void AtomActorInstance::RegisterActor()
        {
            MaterialAssignmentMap materials;
            MaterialComponentRequestBus::EventResult(materials, m_entityId, &MaterialComponentRequests::GetMaterialOverrides);
            CreateRenderProxy(materials);

            TransformNotificationBus::Handler::BusConnect(m_entityId);
            MaterialComponentNotificationBus::Handler::BusConnect(m_entityId);
            MeshComponentRequestBus::Handler::BusConnect(m_entityId);

            const Data::Instance<RPI::Model> model = m_meshFeatureProcessor->GetModel(*m_meshHandle);
            MeshComponentNotificationBus::Event(m_entityId, &MeshComponentNotificationBus::Events::OnModelReady, GetModelAsset(), model);
        }

        void AtomActorInstance::UnregisterActor()
        {
            MeshComponentNotificationBus::Event(m_entityId, &MeshComponentNotificationBus::Events::OnModelPreDestroy);

            MeshComponentRequestBus::Handler::BusDisconnect();
            MaterialComponentNotificationBus::Handler::BusDisconnect();
            TransformNotificationBus::Handler::BusDisconnect();
            m_skinnedMeshFeatureProcessor->ReleaseRenderProxyInterface(m_skinnedMeshRenderProxy);
            if (m_meshHandle)
            {
                m_meshFeatureProcessor->ReleaseMesh(*m_meshHandle);
                m_meshHandle = nullptr;
            }
        }

        void AtomActorInstance::CreateRenderProxy(const MaterialAssignmentMap& materials)
        {
            auto meshFeatureProcessor = RPI::Scene::GetFeatureProcessorForEntity<MeshFeatureProcessorInterface>(m_entityId);
            AZ_Error("ActorComponentController", meshFeatureProcessor, "Unable to find a MeshFeatureProcessorInterface on the entityId.");
            if (meshFeatureProcessor)
            {
                // Last boolean parameter indicates if motion vector is enabled
                m_meshHandle = AZStd::make_shared<MeshFeatureProcessorInterface::MeshHandle>(
                    m_meshFeatureProcessor->AcquireMesh(m_skinnedMeshInstance->m_model->GetModelAsset(), materials, /*skinnedMeshWithMotion=*/true));
            }

            // If render proxies already exist, they will be auto-freed
            SkinnedMeshFeatureProcessorInterface::SkinnedMeshRenderProxyDesc desc{ m_skinnedMeshInputBuffers, m_skinnedMeshInstance, m_meshHandle, m_boneTransforms, {GetAtomSkinningMethod()} };
            m_skinnedMeshRenderProxy = m_skinnedMeshFeatureProcessor->AcquireRenderProxyInterface(desc);

            if (m_transformInterface)
            {
                OnTransformChanged(Transform::Identity(), m_transformInterface->GetWorldTM());
            }
            else
            {
                OnTransformChanged(Transform::Identity(), Transform::Identity());
            }
        }


        void AtomActorInstance::CreateSkinnedMeshInstance()
        {
            SkinnedMeshOutputStreamNotificationBus::Handler::BusDisconnect();
            m_skinnedMeshInstance = m_skinnedMeshInputBuffers->CreateSkinnedMeshInstance();
            if (m_skinnedMeshInstance && m_skinnedMeshInstance->m_model)
            {
                MaterialReceiverNotificationBus::Event(m_entityId, &MaterialReceiverNotificationBus::Events::OnMaterialAssignmentsChanged);
                RegisterActor();

                // [TODO ATOM-14478, LYN-1890]
                // Temporary workaround for cloth to make sure the output skinned buffers are filled at least once.
                // When the blend weights buffer can be unique per instance and updated by cloth component,
                // FillSkinnedMeshInstanceBuffers can be removed.
                FillSkinnedMeshInstanceBuffers();
            }
            else
            {
                AZ_Warning("AtomActorInstance", m_skinnedMeshInstance, "Failed to create target skinned model. Will automatically attempt to re-create when skinned mesh memory is freed up.");
                SkinnedMeshOutputStreamNotificationBus::Handler::BusConnect();
            }
        }

        void AtomActorInstance::FillSkinnedMeshInstanceBuffers()
        {
            AZ_Assert( m_skinnedMeshInputBuffers->GetLodCount() == m_skinnedMeshInstance->m_outputStreamOffsetsInBytes.size(),
                "Number of lods in Skinned Mesh Input Buffers (%d) does not match with Skinned Mesh Instance (%d)",
                m_skinnedMeshInputBuffers->GetLodCount(), m_skinnedMeshInstance->m_outputStreamOffsetsInBytes.size());

            for (size_t lodIndex = 0; lodIndex < m_skinnedMeshInputBuffers->GetLodCount(); ++lodIndex)
            {
                const SkinnedMeshInputLod& inputSkinnedMeshLod = m_skinnedMeshInputBuffers->GetLod(lodIndex);
                const AZStd::vector<uint32_t>& outputBufferOffsetsInBytes = m_skinnedMeshInstance->m_outputStreamOffsetsInBytes[lodIndex];
                uint32_t lodVertexCount = inputSkinnedMeshLod.GetVertexCount();

                auto updateSkinnedMeshInstance =
                    [&inputSkinnedMeshLod, &outputBufferOffsetsInBytes, &lodVertexCount](SkinnedMeshInputVertexStreams inputStream, SkinnedMeshOutputVertexStreams outputStream)
                {
                    const Data::Asset<RPI::BufferAsset>& inputBufferAsset = inputSkinnedMeshLod.GetSkinningInputBufferAsset(inputStream);
                    const RHI::BufferViewDescriptor& inputBufferViewDescriptor = inputBufferAsset->GetBufferViewDescriptor();

                    const uint64_t inputByteCount = aznumeric_cast<uint64_t>(inputBufferViewDescriptor.m_elementCount) * aznumeric_cast<uint64_t>(inputBufferViewDescriptor.m_elementSize);
                    const uint64_t inputByteOffset = aznumeric_cast<uint64_t>(inputBufferViewDescriptor.m_elementOffset) * aznumeric_cast<uint64_t>(inputBufferViewDescriptor.m_elementSize);

                    const uint32_t outputElementSize = SkinnedMeshVertexStreamPropertyInterface::Get()->GetOutputStreamInfo(outputStream).m_elementSize;
                    const uint64_t outputByteCount = aznumeric_cast<uint64_t>(lodVertexCount) * aznumeric_cast<uint64_t>(outputElementSize);
                    const uint64_t outputByteOffset = aznumeric_cast<uint64_t>(outputBufferOffsetsInBytes[static_cast<uint8_t>(outputStream)]);

                    // The byte count from input and output buffers doesn't have to match necessarily.
                    // For example the output positions buffer has double the amount of elements because it has
                    // another set of positions from the previous frame.
                    AZ_Assert(inputByteCount <= outputByteCount, "Trying to write too many bytes to output buffer.");

                    // The shared buffer that all skinning output lives in
                    AZ::Data::Instance<AZ::RPI::Buffer> rpiBuffer = SkinnedMeshOutputStreamManagerInterface::Get()->GetBuffer();

                    rpiBuffer->UpdateData(
                        inputBufferAsset->GetBuffer().data() + inputByteOffset,
                        inputByteCount,
                        outputByteOffset);
                };

                updateSkinnedMeshInstance(SkinnedMeshInputVertexStreams::Position, SkinnedMeshOutputVertexStreams::Position);
                updateSkinnedMeshInstance(SkinnedMeshInputVertexStreams::Normal, SkinnedMeshOutputVertexStreams::Normal);
                updateSkinnedMeshInstance(SkinnedMeshInputVertexStreams::Tangent, SkinnedMeshOutputVertexStreams::Tangent);
                updateSkinnedMeshInstance(SkinnedMeshInputVertexStreams::BiTangent, SkinnedMeshOutputVertexStreams::BiTangent);
            }
        }

        void AtomActorInstance::OnSkinnedMeshOutputStreamMemoryAvailable()
        {
            CreateSkinnedMeshInstance();
        }
    } //namespace Render
} // namespace AZ
