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

#ifdef _DEBUG
#    pragma push_macro("AZ_NUMERICCAST_ENABLED")
#    undef AZ_NUMERICCAST_ENABLED
#    define AZ_NUMERICCAST_ENABLED 1
#endif // #ifdef _DEBUG

#include <EMotionFX_precompiled.h>

#include <AzCore/Casting/numeric_cast.h>
#include <AzCore/Component/EntityId.h>
#include <AzCore/Component/TickBus.h>
#include <AzCore/Component/TransformBus.h>
#include <AzCore/Debug/Profiler.h>
#include <AzCore/Jobs/LegacyJobExecutor.h>
#include <AzCore/Math/Transform.h>
#include <AzCore/base.h>

#include <LmbrCentral/Rendering/MaterialAsset.h>
#include <LmbrCentral/Rendering/MeshComponentBus.h>

#include <Integration/Rendering/Cry/CryRenderActor.h>
#include <Integration/Rendering/Cry/CryRenderActorInstance.h>
#include <Integration/Rendering/Cry/CryRenderBackendCommon.h>
#include <Integration/System/SystemCommon.h>
#include <Integration/System/CVars.h>

#include <I3DEngine.h>
#include <IRenderAuxGeom.h>
#include <IRenderMesh.h>
#include <MathConversion.h>
#include <QTangent.h>

#if defined(EMOTIONFXANIMATION_EDITOR)
#    include <Material/Material.h>
#endif

namespace EMotionFX
{
    namespace Integration
    {
        AZ_CLASS_ALLOCATOR_IMPL(CryRenderActorInstance, EMotionFXAllocator, 0)
        AZ_CLASS_ALLOCATOR_IMPL(CryRenderActorInstance::MaterialOwner, EMotionFXAllocator, 0)

        CryRenderActorInstance::CryRenderActorInstance(AZ::EntityId entityId,
            const EMotionFXPtr<EMotionFX::ActorInstance>& actorInstance,
            const AZ::Data::Asset<ActorAsset>& asset,
            const AZ::Transform& worldTransform)
            : IRenderNode()
            , RenderActorInstance(asset, actorInstance.get(), entityId)            
            , m_renderTransform(AZTransformToLYTransform(worldTransform))
            , m_worldBoundingBox(AABB::RESET)
            , m_isRegisteredWithRenderer(false)
            , m_materialReadyEventSent(false)
        {
            LmbrCentral::RenderNodeRequestBus::Handler::BusConnect(entityId);

            m_materialOwner.reset(aznew MaterialOwner(this, entityId));

            memset(m_arrSkinningRendererData, 0, sizeof(m_arrSkinningRendererData));

            QueueBuildRenderMesh();

            if (m_entityId.IsValid())
            {
                AZ::TransformNotificationBus::Handler::BusConnect(m_entityId);
                AzFramework::BoundsRequestBus::Handler::BusConnect(entityId);
                LmbrCentral::SkeletalHierarchyRequestBus::Handler::BusConnect(m_entityId);
                LmbrCentral::MeshComponentRequestBus::Handler::BusConnect(entityId);
                m_modificationHelper.Connect(m_entityId);

                AZ::Transform entityTransform = AZ::Transform::CreateIdentity();
                EBUS_EVENT_ID_RESULT(entityTransform, m_entityId, AZ::TransformBus, GetWorldTM);
                UpdateWorldTransform(entityTransform);
            }
        }

        CryRenderActorInstance::~CryRenderActorInstance()
        {
            LmbrCentral::RenderNodeRequestBus::Handler::BusDisconnect();

            if (gEnv)
            {
                int nFrameID = gEnv->pRenderer->EF_GetSkinningPoolID();
                int nList = nFrameID % 3;
                if (m_arrSkinningRendererData[nList].nFrameID == nFrameID && m_arrSkinningRendererData[nList].pSkinningData)
                {
                    AZ::LegacyJobExecutor* pAsyncDataJobExecutor = m_arrSkinningRendererData[nList].pSkinningData->pAsyncDataJobExecutor;
                    if (pAsyncDataJobExecutor)
                    {
                        pAsyncDataJobExecutor->WaitForCompletion();
                    }
                }
            }

            DeregisterWithRenderer();

            if (m_entityId.IsValid())
            {
                m_modificationHelper.Disconnect();
                CryRenderActorInstanceRequestBus::Handler::BusDisconnect();
                LmbrCentral::MeshComponentRequestBus::Handler::BusDisconnect();
                AzFramework::BoundsRequestBus::Handler::BusDisconnect(m_entityId);
                LmbrCentral::SkeletalHierarchyRequestBus::Handler::BusDisconnect(m_entityId);
                AZ::TransformNotificationBus::Handler::BusDisconnect(m_entityId);
            }
        }

        void CryRenderActorInstance::SetMaterials(const ActorAsset::MaterialList& materialPerLOD)
        {
            if (gEnv && gEnv->p3DEngine)
            {
                // Initialize materials from input paths.
                // Once materials are converted to real AZ assets, this conversion can be completely removed.
                m_materialPerLOD.clear();
                m_materialPerLOD.reserve(materialPerLOD.size());
                for (auto& materialReference : materialPerLOD)
                {
                    const AZStd::string& path = materialReference.GetAssetPath();

                    // Create render material. If it fails or isn't specified, use the material from base LOD.
                    _smart_ptr<IMaterial> material = path.empty() ? nullptr : gEnv->p3DEngine->GetMaterialManager()->LoadMaterial(path.c_str());

                    if (!material && m_materialPerLOD.size() > 0)
                    {
                        material = m_materialPerLOD.front();
                    }

                    m_materialPerLOD.emplace_back(material);
                }
            }
        }

        void CryRenderActorInstance::SetIsVisible(bool isVisible)
        {
            RenderActorInstance::SetIsVisible(isVisible);

            // Set the cry render node visibility accordingly via the MeshComponentRequestBus.
            SetVisibility(m_isVisible);
        }

        void CryRenderActorInstance::UpdateWorldBoundingBox()
        {
            const MCore::AABB& emfxAabb = m_actorInstance->GetAABB();
            m_worldBoundingBox = AABB(AZVec3ToLYVec3(emfxAabb.GetMin()), AZVec3ToLYVec3(emfxAabb.GetMax()));

            if (m_isRegisteredWithRenderer)
            {
                gEnv->p3DEngine->RegisterEntity(this);
            }
        }

        void CryRenderActorInstance::RegisterWithRenderer()
        {
            if (!m_isRegisteredWithRenderer && gEnv && gEnv->p3DEngine)
            {
                SetRndFlags(ERF_CASTSHADOWMAPS | ERF_HAS_CASTSHADOWMAPS | ERF_COMPONENT_ENTITY, true);

                UpdateWorldBoundingBox();

                gEnv->p3DEngine->RegisterEntity(this);

                m_isRegisteredWithRenderer = true;
            }
        }

        void CryRenderActorInstance::DeregisterWithRenderer()
        {
            if (m_isRegisteredWithRenderer && gEnv && gEnv->p3DEngine)
            {
                gEnv->p3DEngine->FreeRenderNodeState(this);
                m_isRegisteredWithRenderer = false;
            }
        }

        void CryRenderActorInstance::UpdateWorldTransform(const AZ::Transform& entityTransform)
        {
            m_renderTransform = AZTransformToLYTransform(entityTransform);
            UpdateWorldBoundingBox();
        }

        void CryRenderActorInstance::OnTransformChanged(const AZ::Transform& local, const AZ::Transform& world)
        {
            AZ_UNUSED(local);
            UpdateWorldTransform(world);
        }

        AZ::u32 CryRenderActorInstance::GetJointCount()
        {
            return m_actorInstance->GetActor()->GetSkeleton()->GetNumNodes();
        }

        const char* CryRenderActorInstance::GetJointNameByIndex(AZ::u32 jointIndex)
        {
            EMotionFX::Skeleton* skeleton = m_actorInstance->GetActor()->GetSkeleton();
            const AZ::u32 numNodes = skeleton->GetNumNodes();
            if (jointIndex < numNodes)
            {
                return skeleton->GetNode(jointIndex)->GetName();
            }

            return nullptr;
        }

        AZ::s32 CryRenderActorInstance::GetJointIndexByName(const char* jointName)
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

        AZ::Transform CryRenderActorInstance::GetJointTransformCharacterRelative(AZ::u32 jointIndex)
        {
            const EMotionFX::TransformData* transforms = m_actorInstance->GetTransformData();
            if (transforms && jointIndex < transforms->GetNumTransforms())
            {
                return MCore::EmfxTransformToAzTransform(transforms->GetCurrentPose()->GetModelSpaceTransform(jointIndex));
            }

            return AZ::Transform::CreateIdentity();
        }

        void CryRenderActorInstance::Render(const struct SRendParams& inRenderParams, const struct SRenderingPassInfo& passInfo)
        {
            if (!CVars::emfx_actorRenderEnabled)
            {
                return;
            }

            ActorAsset* data = m_actorAsset.Get();
            if (!data)
            {
                // Asset is not loaded.
                AZ_WarningOnce("ActorRenderNode", false, "Actor asset is not loaded. Rendering aborted.");
                return;
            }

            CryRenderActor* renderActor = GetRenderActor();
            if (!renderActor)
            {
                return;
            }

            if (!m_renderTransform.IsValid())
            {
                AZ_Warning("ActorRenderNode", false, "Render node has no valid transform.");
                return;
            }

            if (!(renderActor->ReadyForRendering()) || m_renderMeshesPerLOD.empty())
            {
                return; // not ready for rendering
            }

            AZ_PROFILE_SCOPE(AZ::Debug::ProfileCategory::Animation, "CryRenderActorInstance::Render");

            AZ::u32 useLodIndex = m_actorInstance->GetLODLevel();

            SRendParams rParams(inRenderParams);
            rParams.fAlpha = 1.f;
            IMaterial* previousMaterial = rParams.pMaterial;
            const int previousObjectFlags = rParams.dwFObjFlags;
            rParams.dwFObjFlags |= FOB_DYNAMIC_OBJECT;
            rParams.pMatrix = &m_renderTransform;
            rParams.lodValue = useLodIndex;

            CRenderObject* pObj = gEnv->pRenderer->EF_GetObject_Temp(passInfo.ThreadID());
            pObj->m_fSort = rParams.fCustomSortOffset;
            pObj->m_fAlpha = rParams.fAlpha;
            pObj->m_fDistance = rParams.fDistance;
            pObj->m_II.m_AmbColor = rParams.AmbientColor;

            SRenderObjData* pD = gEnv->pRenderer->EF_GetObjData(pObj, true, passInfo.ThreadID());
            if (rParams.pShaderParams && rParams.pShaderParams->size() > 0)
            {
                pD->SetShaderParams(rParams.pShaderParams);
            }

            pD->m_uniqueObjectId = reinterpret_cast<uintptr_t>(this);

            rParams.pMatrix = &m_renderTransform;

            pObj->m_II.m_Matrix = *rParams.pMatrix;
            pObj->m_nClipVolumeStencilRef = rParams.nClipVolumeStencilRef;
            pObj->m_nTextureID = rParams.nTextureID;
            pObj->m_ObjFlags |= rParams.dwFObjFlags;
            rParams.dwFObjFlags &= ~FOB_NEAREST;
            pObj->m_nMaterialLayers = rParams.nMaterialLayersBlend;
            pD->m_nHUDSilhouetteParams = rParams.nHUDSilhouettesParams;
            pD->m_nCustomData = rParams.nCustomData;
            pD->m_nCustomFlags = rParams.nCustomFlags;
            pObj->m_DissolveRef = rParams.nDissolveRef;
            pObj->m_nSort = fastround_positive(rParams.fDistance * 2.0f);

            if (SSkinningData* skinningData = GetSkinningData())
            {
                pD->m_pSkinningData = skinningData;
                pObj->m_ObjFlags |= FOB_SKINNED;
                pObj->m_ObjFlags |= FOB_DYNAMIC_OBJECT;
                pObj->m_ObjFlags |= FOB_MOTION_BLUR;

                // Shader code is associating this with skin offset - this parameter is currently not used by our skeleton
                pD->m_fTempVars[0] = pD->m_fTempVars[1] = pD->m_fTempVars[2] = 0;
            }

            MeshLOD* meshLOD = renderActor->GetMeshLOD(useLodIndex);
            if (meshLOD)
            {
                if (meshLOD->m_hasDynamicMeshes)
                {
                    m_actorInstance->UpdateMorphMeshDeformers(0.0f);
                }

                IMaterial* pMaterial = rParams.pMaterial;

                // Grab material for this LOD.
                if (!pMaterial && !m_materialPerLOD.empty())
                {
                    const AZ::u32 materialIndex = AZ::GetClamp<AZ::u32>(useLodIndex, 0, m_materialPerLOD.size() - 1);
                    pMaterial = m_materialPerLOD[materialIndex];
                }

                // Otherwise, fall back to default material.
                if (!pMaterial)
                {
                    pMaterial = gEnv->p3DEngine->GetMaterialManager()->GetDefaultMaterial();
                }

                // Send render meshes for editing by other components if required.
                if (!m_modificationHelper.GetMeshModified())
                {
                    for (const LmbrCentral::MeshModificationRequestHelper::MeshLODPrimIndex& meshIndices : m_modificationHelper.MeshesToEdit())
                    {
                        if (meshIndices.lodIndex >= m_renderMeshesPerLOD.size() ||
                            meshIndices.primitiveIndex >= m_renderMeshesPerLOD[meshIndices.lodIndex].size())
                        {
                            AZ_Warning("ActorRenderNode", false, "Mesh indices out of range");
                            continue;
                        }

                        IRenderMesh* renderMesh = m_renderMeshesPerLOD[meshIndices.lodIndex][meshIndices.primitiveIndex];
                        LmbrCentral::MeshModificationNotificationBus::Event(
                            m_entityId,
                            &LmbrCentral::MeshModificationNotificationBus::Events::ModifyMesh,
                            meshIndices.lodIndex,
                            meshIndices.primitiveIndex,
                            renderMesh);
                    }
                    m_modificationHelper.SetMeshModified(true);
                }

                const bool morphsUpdated = MorphTargetWeightsWereUpdated(useLodIndex);
                const size_t numPrimitives = meshLOD->m_primitives.size();
                for (size_t prim = 0; prim < numPrimitives; ++prim)
                {
                    const Primitive& primitive = meshLOD->m_primitives[prim];
                    if (primitive.m_isDynamic && morphsUpdated)
                    {
                        UpdateDynamicSkin(useLodIndex, prim);
                    }

                    if (useLodIndex < m_renderMeshesPerLOD.size() && prim < m_renderMeshesPerLOD[useLodIndex].size())
                    {
                        IRenderMesh* renderMesh = m_renderMeshesPerLOD[useLodIndex][prim];
                        if (renderMesh)
                        {
                            renderMesh->Render(rParams, pObj, pMaterial, passInfo);
                        }
                    }
                }
            }

            // Restore previous state.
            rParams.pMaterial = previousMaterial;
            rParams.dwFObjFlags = previousObjectFlags;
        }

        SSkinningData* CryRenderActorInstance::GetSkinningData()
        {
            AZ_PROFILE_SCOPE(AZ::Debug::ProfileCategory::Animation, "CryRenderActorInstance::GetSkinningData");
            // Get data to fill.
            const int nFrameID = gEnv->pRenderer->EF_GetSkinningPoolID();
            const int nList = nFrameID % 3;
            const int nPrevList = (nFrameID - 1) % 3;

            // Before allocating new skinning date, check if we already have for this frame.
            if (m_arrSkinningRendererData[nList].nFrameID == nFrameID && m_arrSkinningRendererData[nList].pSkinningData)
            {
                return m_arrSkinningRendererData[nList].pSkinningData;
            }

            const EMotionFX::TransformData* transforms = m_actorInstance->GetTransformData();
            const AZ::Matrix3x4* skinningMatrices = transforms->GetSkinningMatrices();
            const AZ::u32 transformCount = transforms->GetNumTransforms();

            SSkinningData* renderSkinningData = gEnv->pRenderer->EF_CreateSkinningData(transformCount, false, m_skinningMethod == SkinningMethod::Linear);

            if (m_skinningMethod == SkinningMethod::Linear)
            {
                Matrix34* renderTransforms = renderSkinningData->pBoneMatrices;

                for (AZ::u32 transformIndex = 0; transformIndex < transformCount; ++transformIndex)
                {
                    renderTransforms[transformIndex] = AZMatrix3x4ToLYMatrix3x4(skinningMatrices[transformIndex]);
                }
            }
            else if (m_skinningMethod == SkinningMethod::DualQuat)
            {
                DualQuat* renderTransforms = renderSkinningData->pBoneQuatsS;

                for (AZ::u32 transformIndex = 0; transformIndex < transformCount; ++transformIndex)
                {
                    renderTransforms[transformIndex] = AZMatrix3x4ToLYDualQuat(skinningMatrices[transformIndex]);
                }
            }

            // Set data for motion blur.
            if (m_arrSkinningRendererData[nPrevList].nFrameID == (nFrameID - 1) && m_arrSkinningRendererData[nPrevList].pSkinningData)
            {
                renderSkinningData->nHWSkinningFlags |= eHWS_MotionBlured;
                renderSkinningData->pPreviousSkinningRenderData = m_arrSkinningRendererData[nPrevList].pSkinningData;
                AZ::LegacyJobExecutor* pAsyncDataJobExecutor = renderSkinningData->pPreviousSkinningRenderData->pAsyncDataJobExecutor;
                if (pAsyncDataJobExecutor)
                {
                    pAsyncDataJobExecutor->WaitForCompletion();
                }
            }
            else
            {
                // If we don't have motion blur data, use the some as for the current frame.
                renderSkinningData->pPreviousSkinningRenderData = renderSkinningData;
            }

            m_arrSkinningRendererData[nList].nFrameID = nFrameID;
            m_arrSkinningRendererData[nList].pSkinningData = renderSkinningData;

            return renderSkinningData;
        }

        bool CryRenderActorInstance::GetLodDistances([[maybe_unused]] const SFrameLodInfo& frameLodInfo, float* distances) const
        {
            for (int lodIndex = 0; lodIndex < SMeshLodInfo::s_nMaxLodCount; ++lodIndex)
            {
                distances[lodIndex] = FLT_MAX;
            }

            return true;
        }

        EERType CryRenderActorInstance::GetRenderNodeType()
        {
            return eERType_RenderComponent;
        }

        const char* CryRenderActorInstance::GetName() const
        {
            return "ActorRenderNode";
        }

        const char* CryRenderActorInstance::GetEntityClassName() const
        {
            return "ActorRenderNode";
        }

        Vec3 CryRenderActorInstance::GetPos([[maybe_unused]] bool bWorldOnly /* = true */) const
        {
            return m_renderTransform.GetTranslation();
        }

        const AABB CryRenderActorInstance::GetBBox() const
        {
            return m_worldBoundingBox;
        }

        void CryRenderActorInstance::GetLocalBounds(AABB& bbox)
        {
            const MCore::AABB& emfxAabb = m_actorInstance->GetStaticBasedAABB();
            bbox = AABB(AZVec3ToLYVec3(emfxAabb.GetMin()), AZVec3ToLYVec3(emfxAabb.GetMax()));
        }

        void CryRenderActorInstance::SetBBox(const AABB& WSBBox)
        {
            m_worldBoundingBox = WSBBox;
        }

        void CryRenderActorInstance::OffsetPosition(const Vec3& delta)
        {
            // Recalculate local transform
            AZ::Transform localTransform = AZ::Transform::CreateIdentity();
            EBUS_EVENT_ID_RESULT(localTransform, m_entityId, AZ::TransformBus, GetLocalTM);

            localTransform.SetTranslation(localTransform.GetTranslation() + LYVec3ToAZVec3(delta));
            EBUS_EVENT_ID(m_entityId, AZ::TransformBus, SetLocalTM, localTransform);
        }

        void CryRenderActorInstance::SetMaterial(_smart_ptr<IMaterial> pMat)
        {
            AZ_Assert(m_materialPerLOD.size() < 2, "Attempting to override actor's multiple LOD materials with a single material");
            m_materialPerLOD.clear();
            m_materialPerLOD.emplace_back(pMat);
        }

        _smart_ptr<IMaterial> CryRenderActorInstance::GetMaterial(Vec3* pHitPos /* = nullptr */)
        {
            AZ_UNUSED(pHitPos);

            if (!m_materialPerLOD.empty())
            {
                return m_materialPerLOD.front();
            }

            return nullptr;
        }

        _smart_ptr<IMaterial> CryRenderActorInstance::GetMaterialOverride()
        {
            return nullptr;
        }

        IStatObj* CryRenderActorInstance::GetEntityStatObj(unsigned int /*nPartId*/, unsigned int /*nSubPartId*/, Matrix34A* /*pMatrix*/, bool /*bReturnOnlyVisible*/)
        {
            return nullptr;
        }

        _smart_ptr<IMaterial> CryRenderActorInstance::GetEntitySlotMaterial(unsigned int /*nPartId*/, bool /*bReturnOnlyVisible*/, bool* /*pbDrawNear */)
        {
            return GetMaterial(nullptr);
        }

        float CryRenderActorInstance::GetMaxViewDist()
        {
            return (100.f * GetViewDistanceMultiplier()); // \todo
        }

        void CryRenderActorInstance::GetMemoryUsage(class ICrySizer* /*pSizer*/) const
        {
        }

        bool CryRenderActorInstance::MorphTargetWeightsWereUpdated(uint32 lodLevel)
        {
            bool differentMorpthTargets = false;

            MorphSetupInstance* morphSetupInstance = m_actorInstance->GetMorphSetupInstance();
            if (morphSetupInstance)
            {
                // if there is no morph setup, we have nothing to do
                MorphSetup* morphSetup = m_actorAsset.Get()->GetActor()->GetMorphSetup(lodLevel);
                if (morphSetup)
                {
                    const uint32 numTargets = morphSetup->GetNumMorphTargets();

                    if (numTargets != m_lastMorphTargetWeights.size())
                    {
                        differentMorpthTargets = true;
                        m_lastMorphTargetWeights.resize(numTargets);
                    }

                    for (uint32 i = 0; i < numTargets; ++i)
                    {
                        // get the morph target
                        MorphTarget* morphTarget = morphSetup->GetMorphTarget(i);
                        MorphSetupInstance::MorphTarget* morphTargetInstance = morphSetupInstance->FindMorphTargetByID(morphTarget->GetID());
                        if (morphTargetInstance)
                        {
                            const float currentWeight = morphTargetInstance->GetWeight();
                            if (!AZ::IsClose(currentWeight, m_lastMorphTargetWeights[i], MCore::Math::epsilon))
                            {
                                m_lastMorphTargetWeights[i] = currentWeight;
                                differentMorpthTargets = true;
                            }
                        }
                    }
                }
                else if (!m_lastMorphTargetWeights.empty())
                {
                    differentMorpthTargets = true;
                    m_lastMorphTargetWeights.clear();
                }
            }
            else if (!m_lastMorphTargetWeights.empty())
            {
                differentMorpthTargets = true;
                m_lastMorphTargetWeights.clear();
            }
            return differentMorpthTargets;
        }

        void CryRenderActorInstance::UpdateDynamicSkin(size_t lodIndex, size_t primitiveIndex)
        {
            ActorAsset* actorAsset = m_actorAsset.Get();
            if (!actorAsset)
            {
                // Asset is not loaded.
                AZ_WarningOnce("ActorRenderNode", false, "Actor asset is not loaded. Rendering aborted.");
                return;
            }

            CryRenderActor* renderActor = GetRenderActor();
            if (!renderActor)
            {
                return;
            }

            MeshLOD* meshLOD = renderActor->GetMeshLOD(lodIndex);
            if (!meshLOD)
            {
                return;
            }

            const Primitive& primitive = meshLOD->m_primitives[primitiveIndex];
            _smart_ptr<IRenderMesh>& renderMesh = m_renderMeshesPerLOD[lodIndex][primitiveIndex];

            IRenderMesh::ThreadAccessLock lockRenderMesh(renderMesh);

            strided_pointer<Vec3> destVertices;
            strided_pointer<Vec3> destNormals;
            strided_pointer<SPipQTangents> destTangents;

            destVertices.data = reinterpret_cast<Vec3*>(renderMesh->GetPosPtr(destVertices.iStride, FSL_SYSTEM_UPDATE));
            destNormals.data = reinterpret_cast<Vec3*>(renderMesh->GetNormPtr(destNormals.iStride, FSL_SYSTEM_UPDATE));

            AZ_Assert(destVertices, "Unexpected null pointer for vertices");
            AZ_Assert(destNormals, "Unexpected null pointer for normals");

            ActorAsset* assetData = m_actorAsset.Get();
            AZ_Assert(assetData, "Invalid asset data");

            const EMotionFX::SubMesh* subMesh = primitive.m_subMesh;
            const EMotionFX::Mesh* mesh = subMesh->GetParentMesh();
            const AZ::Vector3* sourcePositions = static_cast<AZ::Vector3*>(mesh->FindVertexData(EMotionFX::Mesh::ATTRIB_POSITIONS));
            const AZ::Vector3* sourceNormals = static_cast<AZ::Vector3*>(mesh->FindOriginalVertexData(EMotionFX::Mesh::ATTRIB_NORMALS)); // TODO: this shouldn't use the original data, this is a bug, but left here on purpose to hide an issue.
            const AZ::Vector3* sourceBitangents = static_cast<AZ::Vector3*>(mesh->FindOriginalVertexData(EMotionFX::Mesh::ATTRIB_BITANGENTS)); // Due to time constraints we will fix this later.
            const AZ::Vector4* sourceTangents = static_cast<AZ::Vector4*>(mesh->FindOriginalVertexData(EMotionFX::Mesh::ATTRIB_TANGENTS));

            if (!destTangents)
            {
                destTangents.data = reinterpret_cast<SPipQTangents*>(renderMesh->GetQTangentPtr(destTangents.iStride, FSL_SYSTEM_UPDATE));
            }
            AZ_Assert(static_cast<bool>(destTangents), "Expected a destination tangent buffer");

            const AZ::u32 startVertex = subMesh->GetStartVertex();
            const size_t numSubMeshVertices = subMesh->GetNumVertices();
            for (size_t i = 0; i < numSubMeshVertices; ++i)
            {
                const AZ::u32 vertexIndex = startVertex + i;

                const AZ::Vector3& sourcePosition = sourcePositions[vertexIndex];
                destVertices[i] = Vec3(sourcePosition.GetX(), sourcePosition.GetY(), sourcePosition.GetZ());

                const AZ::Vector3& sourceNormal = sourceNormals[vertexIndex];
                destNormals[i] = Vec3(sourceNormal.GetX(), sourceNormal.GetY(), sourceNormal.GetZ());

                if (sourceTangents)
                {
                    // We only need to update the tangents if they are in the mesh, otherwise they will be 0 or not
                    // be present at the destination
                    const AZ::Vector4& sourceTangent = sourceTangents[vertexIndex];
                    const AZ::Vector3 sourceNormalV3(sourceNormals[vertexIndex]);

                    AZ::Vector3 bitangent;
                    if (sourceBitangents)
                    {
                        bitangent = sourceBitangents[vertexIndex];
                    }
                    else
                    {
                        bitangent = sourceNormalV3.Cross(sourceTangent.GetAsVector3()) * sourceTangent.GetW();
                    }

                    const SMeshTangents meshTangent(
                        Vec3(sourceTangent.GetX(), sourceTangent.GetY(), sourceTangent.GetZ()),
                        Vec3(bitangent.GetX(), bitangent.GetY(), bitangent.GetZ()),
                        Vec3(sourceNormalV3.GetX(), sourceNormalV3.GetY(), sourceNormalV3.GetZ()));

                    const Quat q = MeshTangentFrameToQTangent(meshTangent);
                    destTangents[i] = SPipQTangents(
                        Vec4sf(
                            PackingSNorm::tPackF2B(q.v.x),
                            PackingSNorm::tPackF2B(q.v.y),
                            PackingSNorm::tPackF2B(q.v.z),
                            PackingSNorm::tPackF2B(q.w)));
                } // if (sourceTangents)
            } // for all vertices

            renderMesh->UnlockStream(VSF_GENERAL);
            if (destTangents)
            {
                renderMesh->UnlockStream(VSF_QTANGENTS);
            }
        }

        void CryRenderActorInstance::QueueBuildRenderMesh()
        {
            m_shouldBuildRenderMesh = true;
            auto entityId = m_entityId;

            // Start listening for the queued event.
            CryRenderActorInstanceRequestBus::Handler::BusConnect(entityId);

            AZStd::function<void()> finalizeOnMainThread = [entityId]()
            {
                // RenderMesh creation must be performed on the main thread, as required by the renderer.
                // As this function got queued onto the system tick bus and its execution is delayed until
                // the queue is being executed, the actor asset as well as the corresponding cry render actor
                // might have already been destructed. Rather than directly calling Finalize() on a possibly
                // dangling cry render actor, we request a finalize call for it which will only be handled in
                // case the render actor still exists.
                CryRenderActorInstanceRequestBus::Event(entityId, &CryRenderActorInstanceRequests::BuildRenderMeshPerLOD);
            };

            AZ::SystemTickBus::QueueFunction(finalizeOnMainThread);
        }

        void CryRenderActorInstance::BuildRenderMeshPerLOD()
        {
            // Make sure that the queued request is intended for this actor instance.
            if (!m_shouldBuildRenderMesh)
            {
                return;
            }

            // Stop listening for queued requests.
            CryRenderActorInstanceRequestBus::Handler::BusDisconnect();

            m_shouldBuildRenderMesh = false;

            // RenderMesh creation must be performed on the main thread,
            // as required by the renderer.
            m_renderMeshesPerLOD.clear(); // Release smart pointers

            CryRenderActor* renderActor = GetRenderActor();
            if (!renderActor)
            {
                return;
            }

            // Make sure the CryRenderActor data has been finalized.  We finalize the data lazily on instance creation to help ensure
            // that it happens in the correct order.
            renderActor->Finalize();

            // Populate m_renderMeshesPerLOD. If the mesh doesn't require to be unique, we reuse the render mesh from the actor. If the
            // mesh requires to be unique, we create a copy of the actor's render mesh since this actor instance will be modifying it.

            const size_t lodCount = renderActor->GetNumLODs();
            m_renderMeshesPerLOD.resize(lodCount);
            for (size_t i = 0; i < lodCount; ++i)
            {
                MeshLOD* meshLOD = renderActor->GetMeshLOD(i);
                AZ_Assert(meshLOD, "Render Actor's Meshes for LOD %d are not loaded.", i);

                const size_t numPrims = meshLOD->m_primitives.size();
                m_renderMeshesPerLOD[i].resize(numPrims);
                for (size_t primIndex = 0; primIndex < numPrims; ++primIndex)
                {
                    Primitive& primitive = meshLOD->m_primitives[primIndex];

                    _smart_ptr<IRenderMesh> renderMesh;
                    if (primitive.m_useUniqueMesh)
                    {
                        // Create a copy since each actor instance can be deforming differently and we need to send different meshes to render
                        renderMesh = gEnv->pRenderer->CreateRenderMesh("EMotion FX Actor", primitive.m_renderMesh->GetSourceName(), nullptr, eRMT_Dynamic);
                        const AZ::u32 renderMeshFlags = FSM_ENABLE_NORMALSTREAM | FSM_VERTEX_VELOCITY;
                        renderMesh->SetMesh(*primitive.m_mesh, 0, renderMeshFlags, false);
                    }
                    else
                    {
                        // Reuse the same render mesh
                        renderMesh = primitive.m_renderMesh;
                    }

                    m_renderMeshesPerLOD[i][primIndex] = renderMesh;
                }
            }

            // Make sure the material flags have been appropriately updated.
            // Make sure the 3D Engine has refreshed any materials related to this mesh.
            for (auto material : m_materialPerLOD)
            {
                material->UpdateShaderItems();
            }
        }

        void CryRenderActorInstance::OnTick([[maybe_unused]] float timeDelta)
        {
            UpdateBounds();

            if (!m_materialReadyEventSent && m_materialOwner->IsMaterialOwnerReady())
            {
                LmbrCentral::MaterialOwnerNotificationBus::Event(GetEntityId(), &LmbrCentral::MaterialOwnerNotifications::OnMaterialOwnerReady);
                m_materialReadyEventSent = true;
            }
        }

        void CryRenderActorInstance::UpdateBounds()
        {
            UpdateWorldBoundingBox();

            // Update RenderActorInstance world bounding box
#if defined(EMOTIONFXANIMATION_EDITOR)
            const AABB renderNodeWorldBox = GetBBox();
            m_worldAABB = AZ::Aabb::CreateFromMinMax(AZ::Vector3(renderNodeWorldBox.min.x, renderNodeWorldBox.min.y, renderNodeWorldBox.min.z), AZ::Vector3(renderNodeWorldBox.max.x, renderNodeWorldBox.max.y, renderNodeWorldBox.max.z));
#else
            // The bounding box is moving with the actor instance. It is static in the way that it does not change shape.
            // The entity and actor transforms are kept in sync already.
            m_worldAABB = AZ::Aabb::CreateFromMinMax(m_actorInstance->GetAABB().GetMin(), m_actorInstance->GetAABB().GetMax());
#endif

            // Update RenderActorInstance local bounding box
#if defined(EMOTIONFXANIMATION_EDITOR)
            AABB renderNodeLocalBox;
            IRenderNode::GetLocalBounds(renderNodeLocalBox);
            m_localAABB = AZ::Aabb::CreateFromMinMax(AZ::Vector3(renderNodeLocalBox.min.x, renderNodeLocalBox.min.y, renderNodeLocalBox.min.z), AZ::Vector3(renderNodeLocalBox.max.x, renderNodeLocalBox.max.y, renderNodeLocalBox.max.z));
#else
            m_localAABB = AZ::Aabb::CreateFromMinMax(m_actorInstance->GetStaticBasedAABB().GetMin(), m_actorInstance->GetStaticBasedAABB().GetMax());
#endif
        }

        bool CryRenderActorInstance::IsInCameraFrustum() const
        {
            if (!gEnv || !gEnv->pSystem)
            {
                return false;
            }

            const CCamera& camera = gEnv->pSystem->GetViewCamera();
            return camera.IsAABBVisible_F(m_worldBoundingBox);
        }

        void CryRenderActorInstance::DebugDraw(const DebugOptions& debugOptions)
        {
            if (!gEnv || !gEnv->pRenderer)
            {
                return;
            }

            if (debugOptions.m_drawSkeleton)
            {
                DrawSkeleton();
            }

            if (debugOptions.m_drawAABB)
            {
                DrawAABB();
            }

            if (debugOptions.m_drawRootTransform)
            {
                DrawRootTransform(debugOptions.m_rootWorldTransform);
            }

            if (debugOptions.m_emfxDebugDraw)
            {
                EmfxDebugDraw();
            }
        }

        void CryRenderActorInstance::DrawAABB()
        {
            gEnv->pRenderer->GetIRenderAuxGeom()->DrawAABB(GetBBox(), false, Col_Cyan, eBBD_Faceted);
        }

        void CryRenderActorInstance::DrawSkeleton()
        {
            if (!m_actorInstance)
            {
                return;
            }

            const EMotionFX::TransformData* transformData = m_actorInstance->GetTransformData();
            const EMotionFX::Skeleton* skeleton = m_actorInstance->GetActor()->GetSkeleton();
            const EMotionFX::Pose* pose = transformData->GetCurrentPose();

            const AZ::u32 transformCount = transformData->GetNumTransforms();
            const AZ::u32 lodLevel = m_actorInstance->GetLODLevel();

            for (AZ::u32 index = 0; index < skeleton->GetNumNodes(); ++index)
            {
                const EMotionFX::Node* node = skeleton->GetNode(index);
                const AZ::u32 parentIndex = node->GetParentIndex();
                if (parentIndex == MCORE_INVALIDINDEX32)
                {
                    continue;
                }

                if (!node->GetSkeletalLODStatus(lodLevel))
                {
                    continue;
                }

                const AZ::Vector3 bonePos = pose->GetWorldSpaceTransform(index).mPosition;
                const AZ::Vector3 parentPos = pose->GetWorldSpaceTransform(parentIndex).mPosition;
                gEnv->pRenderer->GetIRenderAuxGeom()->DrawBone(AZVec3ToLYVec3(parentPos), AZVec3ToLYVec3(bonePos), Col_YellowGreen);
            }
        }

        void CryRenderActorInstance::DrawRootTransform(const AZ::Transform& worldTransform)
        {
            gEnv->pRenderer->GetIRenderAuxGeom()->DrawCone(AZVec3ToLYVec3(worldTransform.GetTranslation() + AZ::Vector3(0.0f, 0.0f, 0.1f)), AZVec3ToLYVec3(worldTransform.GetBasisY()), 0.05f, 0.5f, Col_Green);
        }

        void CryRenderActorInstance::EmfxDebugDraw()
        {
            IRenderAuxGeom* geomRenderer = gEnv->pRenderer->GetIRenderAuxGeom();
            EMotionFX::DebugDraw& debugDraw = EMotionFX::GetDebugDraw();
            debugDraw.Lock();
            DebugDraw::ActorInstanceData* actorInstanceData = debugDraw.GetActorInstanceData(m_actorInstance);
            actorInstanceData->Lock();
            for (const DebugDraw::Line& line : actorInstanceData->GetLines())
            {
                const ColorF startColor(line.m_startColor.GetR(), line.m_startColor.GetG(), line.m_startColor.GetB(), line.m_startColor.GetA());
                const ColorF endColor(line.m_endColor.GetR(), line.m_endColor.GetG(), line.m_endColor.GetB(), line.m_endColor.GetA());
                geomRenderer->DrawLine(Vec3(line.m_start), startColor, Vec3(line.m_end), endColor, 1.0f);
            }
            actorInstanceData->Unlock();
            debugDraw.Unlock();
        }

        //////////////////////////////////////////////////////////////////////////
        // RenderNodeRequestBus::Handler
        IRenderNode* CryRenderActorInstance::GetRenderNode()
        {
            return this;
        }

        const float CryRenderActorInstance::s_renderNodeRequestBusOrder = 100.f;
        float CryRenderActorInstance::GetRenderNodeRequestBusOrder() const
        {
            return s_renderNodeRequestBusOrder;
        }
        // RenderNodeRequestBus::Handler
        //////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        // MeshComponentRequestBus::Handler
        bool CryRenderActorInstance::GetVisibility()
        {
            return !IsHidden();
        }

        void CryRenderActorInstance::SetVisibility(bool isVisible)
        {
            Hide(!isVisible);
        }

        // MeshComponentRequestBus::Handler
        //////////////////////////////////////////////////////////////////////////

        void CryRenderActorInstance::SetMeshAsset(const AZ::Data::AssetId& id)
        {
            AZ::Data::Asset<ActorAsset> asset = AZ::Data::AssetManager::Instance().GetAsset<ActorAsset>(id, m_actorAsset.GetAutoLoadBehavior());
            if (asset)
            {
                m_actorAsset = asset;
                QueueBuildRenderMesh();
            }
        }

        //////////////////////////////////////////////////////////////////////////
        // MaterialOwnerRequestBus::Handler

        CryRenderActorInstance::MaterialOwner::MaterialOwner(CryRenderActorInstance* renderActorInstance, AZ::EntityId entityId)
            : m_renderActorInstance(renderActorInstance)
        {
            const bool registerBus = true;
            Activate(renderActorInstance, entityId, registerBus);
        }

        CryRenderActorInstance::MaterialOwner::~MaterialOwner()
        {
            Deactivate();
        }

        //////////////////////////////////////////////////////////////////////////
#if defined(EMOTIONFXANIMATION_EDITOR)
        void CryRenderActorInstance::MaterialOwner::SetMaterial(_smart_ptr<IMaterial> material)
        {
            // Set m_materialPerActor and m_materialPerLOD, which contains the material asset references
            if (material)
            {
                if (material->IsSubMaterial())
                {
                    // Attempt to apply the parent material if material is a sub-material
                    CMaterial* editorMaterial = static_cast<CMaterial*>(material->GetUserData());
                    if (editorMaterial && editorMaterial->GetParent() && editorMaterial->GetParent()->GetMatInfo())
                    {
                        material = editorMaterial->GetParent()->GetMatInfo();
                        AZ_Warning("EMotionFX", false, "Cannot apply a sub-material directly to an actor. Applying the parent material group '%s' instead.", material->GetName());
                    }
                    else
                    {
                        AZ_Error("EMotionFX", false, "Cannot apply sub-material '%s' directly to an actor. Try applying the parent material group instead.", material->GetName());
                        return;
                    }
                }

                // Apply the material to the actor
                m_renderActorInstance->m_onMaterialChangedCallback(material->GetName());
            }
            else
            {
                // If material is nullptr, re-set m_materialPerLOD to the default for this actor
                m_renderActorInstance->m_onMaterialChangedCallback("");
            }
        }
#else
        void CryRenderActorInstance::MaterialOwner::SetMaterial(_smart_ptr<IMaterial> material)
        {
            if (material && material->IsSubMaterial())
            {
                AZ_Error("MaterialOwnerRequestBus", false, "Material Owner cannot be given a Sub-Material.");
            }
            else
            {
                m_renderActorInstance->SetMaterial(material);
            }
        }
#endif
        
        _smart_ptr<IMaterial> CryRenderActorInstance::MaterialOwner::GetMaterial()
        {
            _smart_ptr<IMaterial> material = m_renderActorInstance->GetMaterial();

            if (!m_renderActorInstance->IsReady())
            {
                if (material)
                {
                    AZ_Warning("MaterialOwnerRequestBus", false, "A Material was found, but Material Owner is not ready. May have unexpected results. (Try using MaterialOwnerNotificationBus.OnMaterialOwnerReady or MaterialOwnerRequestBus.IsMaterialOwnerReady)");
                }
                else
                {
                    AZ_Error("MaterialOwnerRequestBus", false, "Material Owner is not ready and no Material was found. Assets probably have not finished loading yet. (Try using MaterialOwnerNotificationBus.OnMaterialOwnerReady or MaterialOwnerRequestBus.IsMaterialOwnerReady)");
                }
            }

            return material;
        }
        // MaterialOwnerRequestBus::Handler
        //////////////////////////////////////////////////////////////////////////

        CryRenderActor* CryRenderActorInstance::GetRenderActor() const
        {
            ActorAsset* actorAsset = m_actorAsset.Get();
            if (!actorAsset)
            {
                AZ_Assert(false, "Actor asset is not loaded.");
                return nullptr;
            }

            CryRenderActor* renderActor = azdynamic_cast<CryRenderActor*>(actorAsset->GetRenderActor());
            if (!renderActor)
            {
                AZ_Assert(false, "Expecting a Cry render backend actor.");
                return nullptr;
            }

            return renderActor;
        }
    } //namespace Integration
} // namespace EMotionFX

#ifdef _DEBUG
#    pragma pop_macro("AZ_NUMERICCAST_ENABLED")
#endif // #ifdef _DEBUG
