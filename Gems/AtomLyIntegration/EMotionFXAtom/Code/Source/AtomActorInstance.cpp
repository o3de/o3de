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
#include <AtomLyIntegration/CommonFeatures/SkinnedMesh/SkinnedMeshOverrideBus.h>
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

#include <numeric>

namespace AZ::Render
{
    static constexpr uint32_t s_maxActiveWrinkleMasks = 16;

    AZ_CLASS_ALLOCATOR_IMPL(AtomActorInstance, EMotionFX::Integration::EMotionFXAllocator)

    AtomActorInstance::AtomActorInstance(AZ::EntityId entityId,
        const EMotionFX::Integration::EMotionFXPtr<EMotionFX::ActorInstance>& actorInstance,
        const AZ::Data::Asset<EMotionFX::Integration::ActorAsset>& asset,
        [[maybe_unused]] const AZ::Transform& worldTransform,
        EMotionFX::Integration::SkinningMethod skinningMethod,
        bool rayTracingEnabled)
        : m_rayTracingEnabled(rayTracingEnabled), RenderActorInstance(asset, actorInstance.get(), entityId)
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
        m_atomActorDebugDraw->UpdateActorInstance(m_actorInstance, timeDelta);
    }

    void AtomActorInstance::DebugDraw(const EMotionFX::ActorRenderFlags& renderFlags)
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

    AZ::Aabb AtomActorInstance::GetWorldBounds() const
    {
        return m_worldAABB;
    }

    AZ::Aabb AtomActorInstance::GetLocalBounds() const
    {
        return m_localAABB;
    }

    void AtomActorInstance::SetSkinningMethod(EMotionFX::Integration::SkinningMethod emfxSkinningMethod)
    {
        // Check if the actor has skinning, otherwise fall back to `NoSkinning` regardless of `emfxSkinningMethod`
        if (m_actorInstance->GetActor()->GetSkinMetaAsset().Get())
        {
            RenderActorInstance::SetSkinningMethod(emfxSkinningMethod);
            m_boneTransforms = CreateBoneTransformBufferFromActorInstance(m_actorInstance, emfxSkinningMethod);
        }
        else
        {
            RenderActorInstance::SetSkinningMethod(EMotionFX::Integration::SkinningMethod::None);
        }

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
        case EMotionFX::Integration::SkinningMethod::None:
            return SkinningMethod::NoSkinning;
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
        MaterialConsumerRequestBus::Handler::BusConnect(m_entityId);
        LmbrCentral::SkeletalHierarchyRequestBus::Handler::BusConnect(m_entityId);

        Create();
    }

    void AtomActorInstance::Deactivate()
    {
        SkinnedMeshOutputStreamNotificationBus::Handler::BusDisconnect();
        LmbrCentral::SkeletalHierarchyRequestBus::Handler::BusDisconnect();
        MaterialConsumerRequestBus::Handler::BusDisconnect();
        SkinnedMeshFeatureProcessorNotificationBus::Handler::BusDisconnect();

        Destroy();

        m_meshFeatureProcessor = nullptr;
        m_skinnedMeshFeatureProcessor = nullptr;
    }

    MaterialAssignmentLabelMap AtomActorInstance::GetMaterialLabels() const
    {
        return GetMaterialSlotLabelsFromModelAsset(GetModelAsset());
    }

    MaterialAssignmentId AtomActorInstance::FindMaterialAssignmentId(
        const MaterialAssignmentLodIndex lod, const AZStd::string& label) const
    {
        return GetMaterialSlotIdFromModelAsset(GetModelAsset(), lod, label);
    }

    MaterialAssignmentMap AtomActorInstance::GetDefaultMaterialMap() const
    {
        return GetDefaultMaterialMapFromModelAsset(GetModelAsset());
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
    }

    void AtomActorInstance::OnMaterialsUpdated(const MaterialAssignmentMap& materials)
    {
        if (m_meshFeatureProcessor)
        {
            m_meshFeatureProcessor->SetCustomMaterials(*m_meshHandle, ConvertToCustomMaterialMap(materials));
        }
    }

    void AtomActorInstance::OnMaterialPropertiesUpdated([[maybe_unused]] const MaterialAssignmentMap& materials)
    {
        if (m_meshFeatureProcessor)
        {
            m_meshFeatureProcessor->SetRayTracingDirty(*m_meshHandle);
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

    void AtomActorInstance::SetRayTracingEnabled(bool enabled)
    {
        if (m_meshHandle->IsValid() && m_meshFeatureProcessor)
        {
            m_rayTracingEnabled = enabled;
            m_meshFeatureProcessor->SetRayTracingEnabled(*m_meshHandle, m_rayTracingEnabled);
        }
    }

    bool AtomActorInstance::GetRayTracingEnabled() const
    {
        if (m_meshHandle->IsValid() && m_meshFeatureProcessor)
        {
            return m_meshFeatureProcessor->GetRayTracingEnabled(*m_meshHandle);
        }
        return false;
    }

    void AtomActorInstance::SetExcludeFromReflectionCubeMaps(bool enabled)
    {
        if (m_meshHandle->IsValid() && m_meshFeatureProcessor)
        {
            m_meshFeatureProcessor->SetExcludeFromReflectionCubeMaps(*m_meshHandle, enabled);
        }
    }

    bool AtomActorInstance::GetExcludeFromReflectionCubeMaps() const
    {
        if (m_meshHandle->IsValid() && m_meshFeatureProcessor)
        {
            return m_meshFeatureProcessor->GetExcludeFromReflectionCubeMaps(*m_meshHandle);
        }

        return false;
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
            EMotionFX::Integration::SkinningMethod skinningMethod = GetSkinningMethod();
            
            // When skinning mode is none or there's no skin asset, skip creating bone transform buffer
            if (skinningMethod != EMotionFX::Integration::SkinningMethod::None && m_actorAsset->GetActor()->GetSkinMetaAsset().Get())
            {
                m_boneTransforms = CreateBoneTransformBufferFromActorInstance(m_actorInstance, skinningMethod);
                AZ_Error("AtomActorInstance", m_boneTransforms || AZ::RHI::IsNullRHI(), "Failed to create bone transform buffer.");
            }
            else if (!m_actorAsset->GetActor()->GetSkinMetaAsset().Get())
            {
                // Fallback to no skinning if skinning metaasset doesn't exist
                RenderActorInstance::SetSkinningMethod(EMotionFX::Integration::SkinningMethod::None);
            }

            // If the instance is created before the default materials on the model have finished loading, the mesh feature processor will ignore it.
            // Wait for them all to be ready before creating the instance
            uint32_t lodCount = m_skinnedMeshInputBuffers->GetLodCount();
            for (uint32_t lodIndex = 0; lodIndex < lodCount; ++lodIndex)
            {
                const SkinnedMeshInputLod& inputLod = m_skinnedMeshInputBuffers->GetLod(lodIndex);
                Data::Asset<RPI::ModelLodAsset> modelLodAsset = inputLod.GetModelLodAsset();
                for (const RPI::ModelLodAsset::Mesh& submesh : modelLodAsset->GetMeshes())
                {
                    Data::Asset<RPI::MaterialAsset> defaultSubmeshMaterial = m_skinnedMeshInputBuffers->GetModelAsset()->FindMaterialSlot(submesh.GetMaterialSlotId()).m_defaultMaterialAsset;
                    if (defaultSubmeshMaterial && !defaultSubmeshMaterial.IsReady())
                    {
                        // Start listening for the material's OnAssetReady event.
                        // AtomActorInstance::Create is called on the main thread, so there should be no need to synchronize with the OnAssetReady event handler
                        // since those events will also come from the main thread
                        m_waitForMaterialLoadIds.insert(defaultSubmeshMaterial.GetId());
                        Data::AssetBus::MultiHandler::BusConnect(defaultSubmeshMaterial.GetId());
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
            if (m_boneTransforms)
            {
                m_boneTransforms.reset();
            }
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
        if (m_skinnedMeshHandle.IsValid())
        {
            AZStd::vector<float> boneTransforms;
            GetBoneTransformsFromActorInstance(m_actorInstance, boneTransforms, GetSkinningMethod());

            m_skinnedMeshFeatureProcessor->SetSkinningMatrices(m_skinnedMeshHandle, boneTransforms);

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
                        // and impact a unique mesh and thus correspond with unique dispatches in the morph target pass
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

                    m_skinnedMeshFeatureProcessor->SetMorphTargetWeights(m_skinnedMeshHandle, lodIndex, m_morphTargetWeights);

                    // Until EMotionFX and Atom lods are synchronized [ATOM-13564] we don't know which EMotionFX lod to pull the weights from
                    // Until that is fixed, just use lod 0 [ATOM-15251]
                    if (lodIndex == 0)
                    {
                        UpdateWrinkleMasks();
                    }
                }
            }
            UpdateLightingChannelMask();
        }
    }

    void AtomActorInstance::RegisterActor()
    {
        if (!m_skinnedMeshInstance)
        {
            AZ_Error("AtomActorInstance", m_skinnedMeshInstance, "SkinnedMeshInstance must be created before register this actor.");
            return;
        }

        MaterialAssignmentMap materials;
        MaterialComponentRequestBus::EventResult(materials, m_entityId, &MaterialComponentRequests::GetMaterialMap);
        CreateRenderProxy(materials);

        InitWrinkleMasks();

        TransformNotificationBus::Handler::BusConnect(m_entityId);
        MaterialComponentNotificationBus::Handler::BusConnect(m_entityId);
        MeshComponentRequestBus::Handler::BusConnect(m_entityId);
        SkinnedMeshOverrideRequestBus::Handler::BusConnect(m_entityId);
        MeshHandleStateRequestBus::Handler::BusConnect(m_entityId);

        m_meshFeatureProcessor->SetVisible(*m_meshHandle, IsVisible());
    }

    void AtomActorInstance::UnregisterActor()
    {
        MeshComponentNotificationBus::Event(m_entityId, &MeshComponentNotificationBus::Events::OnModelPreDestroy);

        MeshHandleStateRequestBus::Handler::BusDisconnect();
        SkinnedMeshOverrideRequestBus::Handler::BusDisconnect(m_entityId);
        MeshComponentRequestBus::Handler::BusDisconnect();
        MaterialComponentNotificationBus::Handler::BusDisconnect();
        TransformNotificationBus::Handler::BusDisconnect();
        m_skinnedMeshFeatureProcessor->ReleaseSkinnedMesh(m_skinnedMeshHandle);
        if (m_meshHandle)
        {
            m_meshFeatureProcessor->ReleaseMesh(*m_meshHandle);
            MeshHandleStateNotificationBus::Event(m_entityId, &MeshHandleStateNotificationBus::Events::OnMeshHandleSet, &(*m_meshHandle));
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
            meshDescriptor.m_entityId = m_entityId;
            meshDescriptor.m_modelAsset = m_skinnedMeshInstance->m_model->GetModelAsset();
            meshDescriptor.m_customMaterials = ConvertToCustomMaterialMap(materials);
            meshDescriptor.m_isRayTracingEnabled = m_rayTracingEnabled;
            meshDescriptor.m_isAlwaysDynamic = true;
            meshDescriptor.m_excludeFromReflectionCubeMaps = true;
            meshDescriptor.m_isSkinnedMesh = true;
            meshDescriptor.m_supportRayIntersection = true; // we need to keep the buffer data in order to initialize the actor.
            meshDescriptor.m_modelChangedEventHandler = m_modelChangedEventHandler;
            meshDescriptor.m_objectSrgCreatedHandler = m_objectSrgCreatedHandler;
            m_meshHandle = AZStd::make_shared<MeshFeatureProcessorInterface::MeshHandle>(m_meshFeatureProcessor->AcquireMesh(meshDescriptor));
        }

        // If render proxies already exist, they will be auto-freed
        SkinnedMeshFeatureProcessorInterface::SkinnedMeshHandleDescriptor desc{ m_skinnedMeshInputBuffers, m_skinnedMeshInstance, m_meshHandle, m_boneTransforms, {GetAtomSkinningMethod()} };
        m_skinnedMeshHandle = m_skinnedMeshFeatureProcessor->AcquireSkinnedMesh(desc);

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
            MaterialConsumerNotificationBus::Event(m_entityId, &MaterialConsumerNotificationBus::Events::OnMaterialAssignmentSlotsChanged);

            RegisterActor();
        }
        else
        {
            AZ_Warning("AtomActorInstance", m_skinnedMeshInstance, "Failed to create target skinned model. Will automatically attempt to re-create when skinned mesh memory is freed up.");
            SkinnedMeshOutputStreamNotificationBus::Handler::BusConnect();
        }
    }

    void AtomActorInstance::EnableSkinning(uint32_t lodIndex, uint32_t meshIndex)
    {
        if (m_skinnedMeshHandle.IsValid())
        {
            m_skinnedMeshFeatureProcessor->EnableSkinning(m_skinnedMeshHandle, lodIndex, meshIndex);
        }
    }

    void AtomActorInstance::DisableSkinning(uint32_t lodIndex, uint32_t meshIndex)
    {
        if (m_skinnedMeshHandle.IsValid())
        {
            m_skinnedMeshFeatureProcessor->DisableSkinning(m_skinnedMeshHandle, lodIndex, meshIndex);
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
            const AZStd::vector<Data::Instance<RPI::ShaderResourceGroup>>& wrinkleMaskObjectSrgs = m_meshFeatureProcessor->GetObjectSrgs(*m_meshHandle);

            for (auto& wrinkleMaskObjectSrg : wrinkleMaskObjectSrgs)
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
                        wrinkleMaskObjectSrg->SetImageArray(wrinkleMasksIndex, AZStd::span<const Data::Instance<RPI::Image>>(m_wrinkleMasks.data(), m_wrinkleMasks.size()));

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

    void AtomActorInstance::HandleObjectSrgCreate(const Data::Instance<RPI::ShaderResourceGroup>& objectSrg)
    {
        MeshComponentNotificationBus::Event(m_entityId, &MeshComponentNotificationBus::Events::OnObjectSrgCreated, objectSrg);
    }

    void AtomActorInstance::HandleModelChange(const Data::Instance<RPI::Model>& model)
    {
        Data::Asset<RPI::ModelAsset> modelAsset = m_meshFeatureProcessor->GetModelAsset(*m_meshHandle);
        if (model && modelAsset.IsReady())
        {
            MeshComponentNotificationBus::Event(m_entityId, &MeshComponentNotificationBus::Events::OnModelReady, modelAsset, model);
            MaterialConsumerNotificationBus::Event(m_entityId, &MaterialConsumerNotificationBus::Events::OnMaterialAssignmentSlotsChanged);
            AZ::Interface<AzFramework::IEntityBoundsUnion>::Get()->RefreshEntityLocalBoundsUnion(m_entityId);
            MeshHandleStateNotificationBus::Event(m_entityId, &MeshHandleStateNotificationBus::Events::OnMeshHandleSet, &(*m_meshHandle));
        }
    }

    void AtomActorInstance::UpdateLightingChannelMask()
    {
        if (m_meshHandle)
        {
            const AZStd::vector<Data::Instance<RPI::ShaderResourceGroup>>& objectSrgs =
                m_meshFeatureProcessor->GetObjectSrgs(*m_meshHandle);
            for (auto& objectSrg : objectSrgs)
            {
                RHI::ShaderInputConstantIndex lightingChannelMaskIndex =
                    objectSrg->FindShaderInputConstantIndex(AZ::Name("m_lightingChannelMask"));
                if (lightingChannelMaskIndex.IsValid())
                {
                    objectSrg->SetConstant(lightingChannelMaskIndex, m_actorInstance->GetLightingChannelMask());
                }
            }
            m_meshFeatureProcessor->SetLightingChannelMask(*m_meshHandle, m_actorInstance->GetLightingChannelMask());
            m_meshFeatureProcessor->QueueObjectSrgForCompile(*m_meshHandle);
        }
    }

    const MeshFeatureProcessorInterface::MeshHandle* AtomActorInstance::GetMeshHandle() const
    {
        return m_meshHandle.get();
    }

} // namespace AZ::Render
