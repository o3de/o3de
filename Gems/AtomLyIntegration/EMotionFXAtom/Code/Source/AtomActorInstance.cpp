/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AtomActorInstance.h>
#include <AtomActor.h>
#include <AtomActorDebugDraw.h>
#include <ActorAsset.h>

#include <Atom/Feature/SkinnedMesh/SkinnedMeshInputBuffers.h>
#include <Integration/System/SystemCommon.h>
#include <Integration/System/SystemComponent.h>
#include <EMotionFX/Source/ActorInstance.h>
#include <EMotionFX/Source/DebugDraw.h>
#include <EMotionFX/Source/MorphSetup.h>
#include <EMotionFX/Source/MorphSetupInstance.h>
#include <EMotionFX/Source/MorphTargetStandard.h>
#include <EMotionFX/Source/TransformData.h>
#include <EMotionFX/Source/Skeleton.h>
#include <EMotionFX/Source/Mesh.h>
#include <EMotionFX/Source/Node.h>
#include <MCore/Source/AzCoreConversions.h>

#include <Atom/RHI/RHIUtils.h>

#include <Atom/RPI.Public/AuxGeom/AuxGeomDraw.h>
#include <Atom/RPI.Public/AuxGeom/AuxGeomFeatureProcessorInterface.h>
#include <Atom/RPI.Public/Scene.h>
#include <Atom/RPI.Public/Image/StreamingImage.h>

#include <AzCore/Casting/numeric_cast.h>
#include <AzCore/Component/EntityId.h>
#include <AzCore/Component/TickBus.h>
#include <AzCore/Math/Transform.h>
#include <AzCore/base.h>

namespace AZ
{
    namespace Render
    {
        static constexpr uint32_t s_maxActiveWrinkleMasks = 16;

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

            m_atomActorDebugDraw = AZStd::make_unique<AtomActorDebugDraw>(entityId);
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

        void AtomActorInstance::DebugDraw(const EMotionFX::ActorRenderFlagBitset& renderFlags)
        {
            m_atomActorDebugDraw->DebugDraw(renderFlags, m_actorInstance);
        }

        void AtomActorInstance::UpdateBounds()
        {
            // Update RenderActorInstance world bounding box
            // The bounding box is moving with the actor instance.
            // The entity and actor transforms are kept in sync already.
            m_worldAABB = m_actorInstance->GetAabb();

            // Update RenderActorInstance local bounding box
            // NB: computing the local bbox from the world bbox makes the local bbox artificially larger than it should be
            // instead EMFX should support getting the local bbox from the actor instance directly
            m_localAABB = m_worldAABB.GetTransformedAabb(m_transformInterface->GetWorldTM().GetInverse());

            // Update bbox on mesh instance if it exists
            if (m_meshFeatureProcessor && m_meshHandle && m_meshHandle->IsValid() && m_skinnedMeshInstance)
            {
                m_meshFeatureProcessor->SetLocalAabb(*m_meshHandle, m_localAABB);
            }

            AZ::Interface<AzFramework::IEntityBoundsUnion>::Get()->RefreshEntityLocalBoundsUnion(m_entityId);
        }

        AZ::Aabb AtomActorInstance::GetWorldBounds()
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

        void AtomActorInstance::SetIsVisible(bool isVisible)
        {
            if (IsVisible() != isVisible)
            {
                RenderActorInstance::SetIsVisible(isVisible);
                if (m_meshFeatureProcessor && m_meshHandle)
                {
                    m_meshFeatureProcessor->SetVisible(*m_meshHandle, isVisible);
                }
            }
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
        
        RPI::ModelMaterialSlotMap AtomActorInstance::GetModelMaterialSlots() const
        {
            Data::Asset<const RPI::ModelAsset> modelAsset = GetModelAsset();
            if (modelAsset.IsReady())
            {
                return modelAsset->GetMaterialSlots();
            }
            else
            {
                return {};
            }
        }

        MaterialAssignmentId AtomActorInstance::FindMaterialAssignmentId(
            const MaterialAssignmentLodIndex lod, const AZStd::string& label) const
        {
            if (m_skinnedMeshInstance && m_skinnedMeshInstance->m_model)
            {
                return FindMaterialAssignmentIdInModel(m_skinnedMeshInstance->m_model, lod, label);
            }

            return MaterialAssignmentId();
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

        void AtomActorInstance::SetLodType(RPI::Cullable::LodType lodType)
        {
            RPI::Cullable::LodConfiguration config = m_meshFeatureProcessor->GetMeshLodConfiguration(*m_meshHandle);
            config.m_lodType = lodType;
            m_meshFeatureProcessor->SetMeshLodConfiguration(*m_meshHandle, config);
        }

        RPI::Cullable::LodType AtomActorInstance::GetLodType() const
        {
            return m_meshFeatureProcessor->GetMeshLodConfiguration(*m_meshHandle).m_lodType;
        }
        
        void AtomActorInstance::SetLodOverride(RPI::Cullable::LodOverride lodOverride)
        {
            RPI::Cullable::LodConfiguration config = m_meshFeatureProcessor->GetMeshLodConfiguration(*m_meshHandle);
            config.m_lodOverride = lodOverride;
            m_meshFeatureProcessor->SetMeshLodConfiguration(*m_meshHandle, config);
        }

        RPI::Cullable::LodOverride AtomActorInstance::GetLodOverride() const
        {
            return m_meshFeatureProcessor->GetMeshLodConfiguration(*m_meshHandle).m_lodOverride;
        }

        void AtomActorInstance::SetMinimumScreenCoverage(float minimumScreenCoverage)
        {
            RPI::Cullable::LodConfiguration config = m_meshFeatureProcessor->GetMeshLodConfiguration(*m_meshHandle);
            config.m_minimumScreenCoverage = minimumScreenCoverage;
            m_meshFeatureProcessor->SetMeshLodConfiguration(*m_meshHandle, config);
        }

        float AtomActorInstance::GetMinimumScreenCoverage() const
        {
            return m_meshFeatureProcessor->GetMeshLodConfiguration(*m_meshHandle).m_minimumScreenCoverage;
        }

        void AtomActorInstance::SetQualityDecayRate(float qualityDecayRate)
        {
            RPI::Cullable::LodConfiguration config = m_meshFeatureProcessor->GetMeshLodConfiguration(*m_meshHandle);
            config.m_qualityDecayRate = qualityDecayRate;
            m_meshFeatureProcessor->SetMeshLodConfiguration(*m_meshHandle, config);
        }

        float AtomActorInstance::GetQualityDecayRate() const
        {
            return m_meshFeatureProcessor->GetMeshLodConfiguration(*m_meshHandle).m_qualityDecayRate;
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
            return aznumeric_caster(m_actorInstance->GetActor()->GetSkeleton()->GetNumNodes());
        }

        const char* AtomActorInstance::GetJointNameByIndex(AZ::u32 jointIndex)
        {
            EMotionFX::Skeleton* skeleton = m_actorInstance->GetActor()->GetSkeleton();
            const size_t numNodes = skeleton->GetNumNodes();
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
                const size_t numNodes = skeleton->GetNumNodes();
                for (size_t nodeIndex = 0; nodeIndex < numNodes; ++nodeIndex)
                {
                    if (0 == azstricmp(jointName, skeleton->GetNode(nodeIndex)->GetName()))
                    {
                        return aznumeric_caster(nodeIndex);
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
            AZ_Warning("AtomActorInstance", m_skinnedMeshInputBuffers, "Failed to create SkinnedMeshInputBuffers from Actor. It is likely that this actor doesn't have any meshes");
            if (m_skinnedMeshInputBuffers)
            {
                m_boneTransforms = CreateBoneTransformBufferFromActorInstance(m_actorInstance, GetSkinningMethod());
                AZ_Error("AtomActorInstance", m_boneTransforms || AZ::RHI::IsNullRenderer(), "Failed to create bone transform buffer.");

                // If the instance is created before the default materials on the model have finished loading, the mesh feature processor will ignore it.
                // Wait for them all to be ready before creating the instance
                size_t lodCount = m_skinnedMeshInputBuffers->GetLodCount();
                for (size_t lodIndex = 0; lodIndex < lodCount; ++lodIndex)
                {
                    const SkinnedMeshInputLod& inputLod = m_skinnedMeshInputBuffers->GetLod(lodIndex);
                    const AZStd::vector< SkinnedSubMeshProperties>& subMeshProperties = inputLod.GetSubMeshProperties();
                    for (const SkinnedSubMeshProperties& submesh : subMeshProperties)
                    {
                        Data::Asset<RPI::MaterialAsset> materialAsset = submesh.m_materialSlot.m_defaultMaterialAsset;
                        AZ_Error("AtomActorInstance", materialAsset, "Actor does not have a valid default material in lod %d", lodIndex);

                        if (materialAsset)
                        {
                            if (!materialAsset->IsReady())
                            {
                                // Start listening for the material's OnAssetReady event.
                                // AtomActorInstance::Create is called on the main thread, so there should be no need to synchronize with the OnAssetReady event handler
                                // since those events will also come from the main thread
                                m_waitForMaterialLoadIds.insert(materialAsset->GetId());
                                Data::AssetBus::MultiHandler::BusConnect(materialAsset->GetId());
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

        template<class X>
        void swizzle_unique(AZStd::vector<X>& values, const AZStd::vector<size_t>& indices)
        {
            AZStd::vector<X> out;
            out.reserve(indices.size());

            for (size_t i : indices)
            {
                out.push_back(AZStd::move(values[i]));
            }

            values = AZStd::move(out);
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
                const auto lodCount = aznumeric_cast<uint32_t>(m_actorInstance->GetActor()->GetNumLODLevels());
                for (uint32_t lodIndex = 0; lodIndex < lodCount; ++lodIndex)
                {
                    EMotionFX::MorphSetup* morphSetup = m_actorInstance->GetActor()->GetMorphSetup(lodIndex);
                    if (morphSetup)
                    {
                        // Track all the masks/weights that are currently active
                        m_wrinkleMasks.clear();
                        m_wrinkleMaskWeights.clear();

                        size_t morphTargetCount = morphSetup->GetNumMorphTargets();
                        m_morphTargetWeights.clear();
                        for (size_t morphTargetIndex = 0; morphTargetIndex < morphTargetCount; ++morphTargetIndex)
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
                            for (size_t deformDataIndex = 0; deformDataIndex < morphTargetStandard->GetNumDeformDatas(); ++deformDataIndex)
                            {
                                // Morph targets that don't deform any vertices (e.g. joint-based morph targets) are not registered in the render proxy. Skip adding their weights.
                                const EMotionFX::MorphTargetStandard::DeformData* deformData = morphTargetStandard->GetDeformData(deformDataIndex);
                                if (deformData->m_numVerts > 0)
                                {
                                    float weight = morphTargetSetupInstance->GetWeight();
                                    m_morphTargetWeights.push_back(weight);

                                    // If the morph target is active and it has a wrinkle mask
                                    auto wrinkleMaskIter = m_morphTargetWrinkleMaskMapsByLod[lodIndex].find(morphTargetStandard);
                                    if (weight > 0 && wrinkleMaskIter != m_morphTargetWrinkleMaskMapsByLod[lodIndex].end())
                                    {
                                        // Add the wrinkle mask and weight, to be set on the material
                                        m_wrinkleMasks.push_back(wrinkleMaskIter->second);
                                        m_wrinkleMaskWeights.push_back(weight);
                                    }
                                }
                            }
                        }

                        AZ_Assert(m_wrinkleMasks.size() == m_wrinkleMaskWeights.size(), "Must have equal # of masks and weights");

                        // If there's too many masks, truncate
                        if (m_wrinkleMasks.size() > s_maxActiveWrinkleMasks)
                        {
                            // Build a remapping of indices (because we want to sort two vectors)
                            AZStd::vector<size_t> remapped;
                            remapped.resize_no_construct(m_wrinkleMasks.size());
                            std::iota(remapped.begin(), remapped.end(), 0);

                            // Sort index remapping by weight (highest first)
                            std::sort(remapped.begin(), remapped.end(), [&](size_t ia, size_t ib) {
                                return m_wrinkleMaskWeights[ia] > m_wrinkleMaskWeights[ib];
                            });

                            // Truncate indices list
                            remapped.resize(s_maxActiveWrinkleMasks);

                            // Remap wrinkle masks list and weights list
                            swizzle_unique(m_wrinkleMasks, remapped);
                            swizzle_unique(m_wrinkleMaskWeights, remapped);
                        }

                        m_skinnedMeshRenderProxy->SetMorphTargetWeights(lodIndex, m_morphTargetWeights);

                        // Until EMotionFX and Atom lods are synchronized [ATOM-13564] we don't know which EMotionFX lod to pull the weights from
                        // Until that is fixed, just use lod 0 [ATOM-15251]
                        if (lodIndex == 0)
                        {
                            UpdateWrinkleMasks();
                        }
                    }
                }
            }
        }

        void AtomActorInstance::RegisterActor()
        {
            MaterialAssignmentMap materials;
            MaterialComponentRequestBus::EventResult(materials, m_entityId, &MaterialComponentRequests::GetMaterialOverrides);
            CreateRenderProxy(materials);

            InitWrinkleMasks();

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
                MeshHandleDescriptor meshDescriptor;
                meshDescriptor.m_modelAsset = m_skinnedMeshInstance->m_model->GetModelAsset();

                // [GFX TODO][ATOM-13067] Enable raytracing on skinned meshes
                meshDescriptor.m_isRayTracingEnabled = false;

                m_meshHandle = AZStd::make_shared<MeshFeatureProcessorInterface::MeshHandle>(
                    m_meshFeatureProcessor->AcquireMesh(meshDescriptor, materials));
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

                // [TODO ATOM-15288]
                // Temporary workaround for cloth to make sure the output skinned buffers are filled at least once.
                // When meshes with cloth data are not dispatched for skinning FillSkinnedMeshInstanceBuffers can be removed.
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
                    [[maybe_unused]] const uint64_t outputByteCount = aznumeric_cast<uint64_t>(lodVertexCount) * aznumeric_cast<uint64_t>(outputElementSize);
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

        void AtomActorInstance::InitWrinkleMasks()
        {
            EMotionFX::Actor* actor = m_actorAsset->GetActor();
            m_morphTargetWrinkleMaskMapsByLod.resize(m_skinnedMeshInputBuffers->GetLodCount());
            m_wrinkleMasks.reserve(s_maxActiveWrinkleMasks);
            m_wrinkleMaskWeights.reserve(s_maxActiveWrinkleMasks);

            for (size_t lodIndex = 0; lodIndex < m_skinnedMeshInputBuffers->GetLodCount(); ++lodIndex)
            {
                EMotionFX::MorphSetup* morphSetup = actor->GetMorphSetup(static_cast<uint32>(lodIndex));
                if (morphSetup)
                {
                    const AZStd::vector<AZ::RPI::MorphTargetMetaAsset::MorphTarget>& metaDatas = actor->GetMorphTargetMetaAsset()->GetMorphTargets();
                    // Loop over all the EMotionFX morph targets
                    size_t numMorphTargets = morphSetup->GetNumMorphTargets();
                    for (size_t morphTargetIndex = 0; morphTargetIndex < numMorphTargets; ++morphTargetIndex)
                    {
                        EMotionFX::MorphTargetStandard* morphTarget = static_cast<EMotionFX::MorphTargetStandard*>(morphSetup->GetMorphTarget(morphTargetIndex));
                        for (const RPI::MorphTargetMetaAsset::MorphTarget& metaData : metaDatas)
                        {
                            // Find the metaData associated with this morph target
                            if (metaData.m_morphTargetName == morphTarget->GetNameString() && metaData.m_wrinkleMask && metaData.m_numVertices > 0)
                            {
                                // If the metaData has a wrinkle mask, add it to the map
                                Data::Instance<RPI::StreamingImage> streamingImage = RPI::StreamingImage::FindOrCreate(metaData.m_wrinkleMask);
                                if (streamingImage)
                                {
                                    m_morphTargetWrinkleMaskMapsByLod[lodIndex][morphTarget] = streamingImage;
                                }
                            }
                        }
                    }
                }
            }
        }

        void AtomActorInstance::UpdateWrinkleMasks()
        {
            if (m_meshHandle)
            {
                Data::Instance<RPI::ShaderResourceGroup> wrinkleMaskObjectSrg = m_meshFeatureProcessor->GetObjectSrg(*m_meshHandle);
                if (wrinkleMaskObjectSrg)
                {
                    RHI::ShaderInputImageIndex wrinkleMasksIndex = wrinkleMaskObjectSrg->FindShaderInputImageIndex(Name{ "m_wrinkle_masks" });
                    RHI::ShaderInputConstantIndex wrinkleMaskWeightsIndex = wrinkleMaskObjectSrg->FindShaderInputConstantIndex(Name{ "m_wrinkle_mask_weights" });
                    RHI::ShaderInputConstantIndex wrinkleMaskCountIndex = wrinkleMaskObjectSrg->FindShaderInputConstantIndex(Name{ "m_wrinkle_mask_count" });
                    if (wrinkleMasksIndex.IsValid() || wrinkleMaskWeightsIndex.IsValid() || wrinkleMaskCountIndex.IsValid())
                    {
                        AZ_Error("AtomActorInstance", wrinkleMasksIndex.IsValid(), "m_wrinkle_masks not found on the ObjectSrg, but m_wrinkle_mask_weights and/or m_wrinkle_mask_count are being used.");
                        AZ_Error("AtomActorInstance", wrinkleMaskWeightsIndex.IsValid(), "m_wrinkle_mask_weights not found on the ObjectSrg, but m_wrinkle_masks and/or m_wrinkle_mask_count are being used.");
                        AZ_Error("AtomActorInstance", wrinkleMaskCountIndex.IsValid(), "m_wrinkle_mask_count not found on the ObjectSrg, but m_wrinkle_mask_weights and/or m_wrinkle_masks are being used.");

                        if (m_wrinkleMasks.size())
                        {
                            wrinkleMaskObjectSrg->SetImageArray(wrinkleMasksIndex, AZStd::array_view<Data::Instance<RPI::Image>>(m_wrinkleMasks.data(), m_wrinkleMasks.size()));

                            // Set the weights for any active masks
                            for (size_t i = 0; i < m_wrinkleMaskWeights.size(); ++i)
                            {
                                wrinkleMaskObjectSrg->SetConstant(wrinkleMaskWeightsIndex, m_wrinkleMaskWeights[i], static_cast<uint32_t>(i));
                            }
                            AZ_Error("AtomActorInstance", m_wrinkleMaskWeights.size() <= s_maxActiveWrinkleMasks, "The skinning shader supports no more than %d active morph targets with wrinkle masks.", s_maxActiveWrinkleMasks);
                        }

                        wrinkleMaskObjectSrg->SetConstant(wrinkleMaskCountIndex, aznumeric_cast<uint32_t>(m_wrinkleMasks.size()));
                        m_meshFeatureProcessor->QueueObjectSrgForCompile(*m_meshHandle);
                    }
                }
            }
        }

    } //namespace Render
} // namespace AZ
