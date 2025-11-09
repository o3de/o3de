/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <OpenParticleSystem/ParticleSystem.h>

#include <OpenParticleSystem/ParticleEditorRequestBus.h>
#include <OpenParticleSystem/ParticleFeatureProcessor.h>

#include <Atom/Feature/RenderCommon.h>
#include <AtomCore/Instance/InstanceDatabase.h>
#include <Atom/RHI/DrawPacketBuilder.h>
#include <Atom/RPI.Public/Scene.h>
#include <Atom/RPI.Public/View.h>

#include <particle/core/ParticleHelper.h>

#include <Atom/RPI.Reflect/Model/ModelLodAsset.h>
#include <AzFramework/Components/CameraBus.h>
#include <EMotionFX/Source/ActorManager.h>
#include <EMotionFX/Source/Node.h>
#include <EMotionFX/Source/TransformData.h>
#include <Integration/ActorComponentBus.h>

#include "EMotionFX/Source/AnimGraphInstance.h"
#include "EMotionFX/Source/MotionSystem.h"

namespace
{
    constexpr int VERTEX_DIMENSION{3};
    constexpr int FACE_DIMENSION{3};

    struct SimpleVec3
    {
        float x;
        float y;
        float z;
    };

    double CalTriArea(const AZ::Vector3& v1, const AZ::Vector3& v2, const AZ::Vector3& v3)
    {
        AZ::Vector3 e1;
        AZ::Vector3 e2;
        e1 = v2 - v1;
        e2 = v3 - v2;
        return 0.5 * e1.Cross(e2).GetLength();
    }

    void ResetMeshBuffers(const AZStd::span<const AZ::RPI::ModelLodAsset::Mesh>& subMeshes, AZStd::vector<AZ::u32>& allMeshIndices,
        AZStd::vector<AZ::Vector3>& allMeshVertexes, AZStd::vector<double>& allFaceAreas)
    {
        size_t totalIndiceCnt{0};
        size_t totalVetexCnt{0};

        for (size_t j = 0; j < subMeshes.size(); j++)
        {
            totalIndiceCnt += subMeshes[j].GetIndexCount();
            totalVetexCnt += subMeshes[j].GetVertexCount();
        }
        allMeshIndices.resize(totalIndiceCnt);
        allMeshVertexes.resize(totalVetexCnt);
        allFaceAreas.resize(totalIndiceCnt / FACE_DIMENSION);
    }

}

namespace OpenParticle
{
    ParticleSystem::~ParticleSystem()
    {
        ClearAllLightEffects();
    }

    AZ::Data::Instance<ParticleSystem> ParticleSystem::Create(const AZ::Data::Asset<ParticleAsset>& asset, const ParticleComponentConfig& rtConfig)
    {
        return AZ::Data::InstanceDatabase<ParticleSystem>::Instance().Create(asset, reinterpret_cast<const AZStd::any*>(&rtConfig));
    }

    AZ::Data::Instance<ParticleSystem> ParticleSystem::CreateInternal(ParticleAsset& asset, const ParticleComponentConfig& rtConfig)
    {
        AZ::Data::Instance<ParticleSystem> particle = new ParticleSystem(rtConfig);
        auto result = particle->Init(asset);
        if (result)
        {
            return particle;
        }

        return nullptr;
    }

    void ParticleSystem::SetFeatureProcessor(ParticleFeatureProcessor* fp)
    {
        m_particleFp = fp;
    }

    void ParticleSystem::SetScene(AZ::RPI::Scene* scene)
    {
        m_scene = scene;
    }

    void ParticleSystem::SetEntityId(const AZ::EntityId& id)
    {
        m_entityId = id;
    }

    void ParticleSystem::SetObjectId(const AZ::Render::TransformServiceFeatureProcessorInterface::ObjectId& id)
    {
        m_objectId = id;
    }

    void ParticleSystem::SetTransform(AZ::Transform transform)
    {
        AZ::Vector3 lastPos = m_transform.GetTranslation();
        m_transform = transform;
        AZ::Vector3 currentPos = m_transform.GetTranslation();
        float distance = lastPos.GetDistance(currentPos);
        if (m_particleSystem != nullptr)
        {
            auto emitters = m_particleSystem->GetAllEmitters();
            for (auto emitter : emitters)
            {
                emitter.second->SetMoveDistance(distance);
                emitter.second->SetEmitterTransform(m_transform);
            }
        }
    }

    void ParticleSystem::PostLoad()
    {
        m_particleSystem = AZStd::make_unique<SimuCore::ParticleCore::ParticleSystem>();
        m_asset->GetParticleArchive() << *m_particleSystem;
        m_particleSystem->Play();

        m_changedDiffuseMap = false;

        Rebuild();
        if (m_rtConfig.m_followActiveCamera)
        {
            m_particleSystem->UseGlobalSpace();
        }
        m_particleSystem->WarmUp();
        m_rtSysTransform = m_transform;
    }

    bool ParticleSystem::Init(ParticleAsset& asset)
    {
        m_asset = { &asset, AZ::Data::AssetLoadBehavior::PreLoad };
        return true;
    }

    void ParticleSystem::Tick(float delta)
    {
        if (!m_particleSystem || m_particleSystem->GetAllEmitters().size() == 0)
        {
            AZ_Error("ParticleSystem", false, "particleSystem is unready!");
            return;
        }

        OpenParticleSystem::CameraTransform cameraTransform;
        if (!m_rtConfig.m_inParticleEditor)
        {
            Camera::ActiveCameraRequestBus::BroadcastResult(
                    cameraTransform.m_transform, &Camera::ActiveCameraRequestBus::Events::GetActiveCameraTransform);

            if (m_rtConfig.m_followActiveCamera)
            {
                AZ::Vector3 newEmitterPosition{cameraTransform.m_transform.GetTranslation().GetX(),
                                               cameraTransform.m_transform.GetTranslation().GetY(),
                                               cameraTransform.m_transform.GetTranslation().GetZ() + m_transform.GetTranslation().GetZ()};
                m_rtSysTransform.SetTranslation(newEmitterPosition);
            }
        }
        else
        {
            OpenParticleSystem::ParticleEditorRequestBus::BroadcastResult(
                    cameraTransform, &OpenParticleSystem::ParticleEditorRequestBus::Events::GetParticleEditorCameraTransform);
        }
        m_particleSystem->UpdateWorldInfo(cameraTransform.m_transform, 
                                          m_rtConfig.m_followActiveCamera ? m_rtSysTransform : m_transform,
                                          -AZ::Vector3::CreateAxisY());

        m_particleSystem->Simulate(delta);

        if (m_changedDiffuseMap)
        {
            for (auto iter = m_diffuseMapInstances.begin(); iter != m_diffuseMapInstances.end();)
            {
                if (iter->first >= m_particleSystem->GetAllEmitters().size())
                {
                    AZ_Error("ParticleSystem", false,
                        "The emitter index is out of range, it starts from 0, please check it again.");
                    m_diffuseMapInstances.erase(iter++);
                    continue;
                }
                auto& instance = m_emitterInstances[m_particleSystem->GetAllEmitters().at(iter->first)];
                if (instance.m_material->CanCompile())
                {
                    AZ::RPI::MaterialPropertyIndex materialPropertyIndex =
                        instance.m_material->FindPropertyIndex(AZ::Name("general.DiffuseMap"));
                    if (instance.m_material->SetPropertyValue(materialPropertyIndex, iter->second))
                    {
                        if (!instance.m_material->Compile())
                        {
                            AZ_Error("ParticleSystem", false,
                                "Failed to compile the material after changed the diffuse map.");
                        }
                    }
                    else
                    {
                        AZ_Error("ParticleSystem", false, "Failed to change the diffuse map.");
                    }
                    m_diffuseMapInstances.erase(iter++);
                }
                else
                {
                    iter++;
                }
            }
            if (m_emitterInstances.size() == 0)
            {
                m_changedDiffuseMap = false;
            }
        }
    }

    static AZ::Name GetFromSpriteKey(const SimuCore::ParticleCore::DrawItem& item)
    {
        auto facing = SimuCore::ParticleCore::SpriteVariantKeyGetFacing(item.variantKey);
        switch (facing)
        {
        case SimuCore::ParticleCore::Facing::CAMERA_POS:
            return AZ::Name("FacingMode::CameraPos");
        case SimuCore::ParticleCore::Facing::CAMERA_SQUARE:
            return AZ::Name("FacingMode::CameraSquare");
        case SimuCore::ParticleCore::Facing::CAMERA_RECTANGLE:
            return AZ::Name("FacingMode::CameraRectangle");
        case SimuCore::ParticleCore::Facing::VELOCITY:
            return AZ::Name("FacingMode::Velocity");
        case SimuCore::ParticleCore::Facing::CUSTOM:
            return AZ::Name("FacingMode::Custom");
        default:
            return AZ::Name("");
        }
    }

    static void SetOption(AZ::RPI::ShaderOptionGroup& shaderOptions, const SimuCore::ParticleCore::DrawItem& item)
    {
        switch (item.type)
        {
        case SimuCore::ParticleCore::RenderType::SPRITE:
            shaderOptions.SetValue(AZ::Name("o_facingMode"), GetFromSpriteKey(item));
            break;
        default:
            shaderOptions.SetUnspecifiedToDefaultValues();
            break;
        }
    }

    static bool MaterialRequiresForwardPassIblSpecular(AZ::Data::Instance<AZ::RPI::Material> material)
    {
        for (auto& shaderItem : material->GetGeneralShaderCollection())
        {
            if (shaderItem.IsEnabled())
            {
                AZ::RPI::ShaderOptionIndex index = shaderItem.GetShaderOptionGroup().GetShaderOptionLayout()->
                    FindShaderOptionIndex(AZ::Name{ "o_materialUseForwardPassIBLSpecular" });
                if (index.IsValid())
                {
                    AZ::RPI::ShaderOptionValue value =
                        shaderItem.GetShaderOptionGroup().GetValue(AZ::Name{ "o_materialUseForwardPassIBLSpecular" });
                    if (value.GetIndex() == 1)
                    {
                        return true;
                    }
                }
            }
        }
        return false;
    }

    void ParticleSystem::Render(DriverWrap& driverWrap, const AZ::RPI::FeatureProcessor::RenderPacket& packet)
    {
        if (!m_particleSystem)
        {
            return;
        }

        m_drawPacket.clear();

        for (auto& view : packet.m_views)
        {
            auto transform = view->GetCameraTransform();

            SimuCore::ParticleCore::WorldInfo world = {};
            world.cameraPosition = transform.GetTranslation();
            world.cameraUp = transform.TransformVector(AZ::Vector3::CreateAxisZ());
            world.cameraRight = transform.TransformVector(AZ::Vector3::CreateAxisX());
            world.axisZ = -AZ::Vector3::CreateAxisY();
            world.viewKey.p = (intptr_t)view.get();
            world.emitterTransform = m_transform;

            auto emitters = m_particleSystem->GetVisibleEmitters();
            for (auto emitter : emitters) {
                if (emitter == nullptr) {
                    continue;
                }
                auto& instance = m_emitterInstances[emitter];
                for (auto& shader : instance.m_shaders)
                {
                    auto drawListTag = shader.first;
                    if (!view->HasDrawListTag(drawListTag))
                    {
                        continue;
                    }

                    auto& efd = instance.m_emitterForDrawPair[shader.second.get()];
                    DrawParam drawParam;
                    SimuCore::ParticleCore::DrawItem& item = drawParam.item;
                    emitter->Render(reinterpret_cast<AZ::u8*>(&driverWrap), world, item);
                    auto pipeline = m_particleFp->Fetch(GetPipelineKey(item.type));
                    if (pipeline == nullptr || item.drawArgs.Empty())
                    {
                        ClearLightEffects(emitter->GetEmitterId());
                        continue;
                    }

                    // Get light data
                    if (emitter->HasLightModule())
                    {
                        SetParticleLight(*emitter, item);
                    }

                    if (emitter->HasSkeletonModule())
                    {
                        SetBoneAndVertexBuffer(*emitter);
                    }

                    {
                        AZ_PROFILE_SCOPE(AzCore, "ParticleSystem::shader compile");
                        AZ::RPI::ShaderOptionGroup& shaderOptions = efd.optionGroup;
                        SetOption(shaderOptions, item);
                        auto variant = shader.second->GetVariant(shaderOptions.GetShaderVariantId());

                        if (efd.variantKey.value != item.variantKey.value && efd.m_drawSrg &&
                            efd.m_drawSrg->GetLayout()->HasShaderVariantKeyFallbackEntry())
                        {
                            if (!variant.IsFullyBaked())
                            {
                                efd.m_drawSrg->SetShaderVariantKeyFallbackValue(
                                        shaderOptions.GetShaderVariantKeyFallbackValue());
                            }
                            efd.m_drawSrg->Compile();
                            efd.variantKey.value = item.variantKey.value;
                        }
                        drawParam.shader = shader.second;
                        drawParam.variant = variant;
                    }
                    if (emitter->GetRenderType() == SimuCore::ParticleCore::RenderType::MESH)
                    {
                        const auto meshCount = instance.m_model.GetMeshCount();
                        if (!instance.m_model.BuildInputStreamLayouts(shader.second->GetInputContract()))
                        {
                            continue;
                        }

                        for (auto it = 0; it < meshCount; ++it)
                        {
                            RenderParticle(drawListTag, drawParam, instance, *view, it);
                        }
                    }
                    else
                    {
                        // sprite & ribbon
                        RenderParticle(drawListTag, drawParam, instance, *view);
                    }
                }
            }
        }
    }

    void ParticleSystem::SetMaterialDiffuseMap(AZ::u32 emitterIndex, AZStd::string mapPath)
    {
        AZStd::string path = mapPath + ".streamingimage";
        AZ::Data::AssetId mapAssetId;
        AZ::Data::AssetCatalogRequestBus::BroadcastResult(mapAssetId, &AZ::Data::AssetCatalogRequestBus::
            Events::GetAssetIdByPath, path.c_str(), AZ::RPI::MaterialAsset::TYPEINFO_Uuid(), false);

        if (!mapAssetId.IsValid())
        {
            AZ_Error("ParticleSystem", false, "Failed to get the diffuse map's asset id.");
            return;
        }

        AZ::Data::Asset<AZ::RPI::ImageAsset>  imageAsset = AZ::Data::Asset<AZ::RPI::ImageAsset>(
            mapAssetId, azrtti_typeid<AZ::RPI::StreamingImageAsset>());

        if (!imageAsset.IsReady())
        {
            imageAsset = AZ::Data::AssetManager::Instance().GetAsset<AZ::RPI::StreamingImageAsset>(
                imageAsset.GetId(), AZ::Data::AssetLoadBehavior::PreLoad);
            imageAsset.BlockUntilLoadComplete();
            if (!imageAsset.IsReady())
            {
                AZ_Error("ParticleSystem", false, "Failed to load the diffuse map.");
                return;
            }
        }
        AZStd::any any;
        any = imageAsset;
        AZ::RPI::MaterialPropertyValue value = AZ::RPI::MaterialPropertyValue::FromAny(any);
        m_diffuseMapInstances.emplace(emitterIndex, value);

        m_changedDiffuseMap = true;
    }

    void ParticleSystem::SetBoneAndVertexBuffer(SimuCore::ParticleCore::ParticleEmitter& emitter)
    {
        if (m_actorInstance == nullptr)
        {
            EMotionFX::Integration::ActorComponentRequestBus::EventResult(
                m_actorInstance, m_entityId, &EMotionFX::Integration::
                ActorComponentRequestBus::Events::GetActorInstance);
            if (m_actorInstance == nullptr)
            {
                return;
            }
        }

        const EMotionFX::Actor* actor = m_actorInstance->GetActor();
        EMotionFX::TransformData* transData = m_actorInstance->GetTransformData();
        if (actor == nullptr || transData == nullptr)
        {
            return;
        }

        AZ::u32 emitterId = emitter.GetEmitterId();
        if (emitter.GetMeshSampleType() == SimuCore::ParticleCore::MeshSampleType::BONE)
        {
            GetSkeletonBoneBuffer(emitterId, *actor, *transData);
        }
        else if (emitter.GetMeshSampleType() == SimuCore::ParticleCore::MeshSampleType::VERTEX)
        {
            GetMeshVertexBuffer(emitterId, *actor, *transData);
        }
        else
        {
            GetMeshAreaBuffer(emitterId, *actor, *transData);
        }

        if (m_boneStreams.at(emitterId).size() > 0 || m_vertexStreams.at(emitterId).size() > 0)
        {
            const auto& boneStream = m_boneStreams.at(emitterId);
            const auto& vertexStream = m_vertexStreams.at(emitterId);
            const auto& indiceStream = m_indiceStreams.at(emitterId);
            const auto& areaStream = m_areaStreams.at(emitterId);
            emitter.SetSkeletonMesh(
                boneStream.data(), static_cast<AZ::u32>(boneStream.size()),
                vertexStream.data(), static_cast<AZ::u32>(vertexStream.size()),
                indiceStream.data(), static_cast<AZ::u32>(indiceStream.size()),
                areaStream.data());
        }
    }

    void ParticleSystem::GetSkeletonBoneBuffer(
        const AZ::u32 emitterId, const EMotionFX::Actor& actor, const EMotionFX::TransformData& transData)
    {
        const EMotionFX::Skeleton* skeleton = actor.GetSkeleton();
        const EMotionFX::Pose* pose = transData.GetCurrentPose();
        if (skeleton == nullptr || pose == nullptr)
        {
            return;
        }
        const size_t numJoints = skeleton->GetNumNodes();
        auto& boneStream = m_boneStreams.at(emitterId);
        boneStream.clear();
        for (size_t jointIndex = 0; jointIndex < numJoints; ++jointIndex)
        {
            const EMotionFX::Node* jointNode = skeleton->GetNode(jointIndex);
            const size_t parentIndex = jointNode->GetParentIndex();
            if (parentIndex == InvalidIndex32)
            {
                continue;
            }

            const AZ::Vector3 bonePosTmp = pose->GetWorldSpaceTransform(jointIndex).m_position;
            const AZ::Vector3 bonePos = m_transform.GetInverse().TransformPoint(bonePosTmp);
            boneStream.push_back({bonePos.GetX(), bonePos.GetY(), bonePos.GetZ()});
        }
    }

    void ParticleSystem::GetMeshVertexBuffer(const AZ::u32 emitterId, const EMotionFX::Actor& actor, const EMotionFX::TransformData& transData)
    {
        AZ::Data::Instance<AZ::RPI::Model> m_modelInstance =
            AZ::RPI::Model::FindOrCreate(m_asset->GetParticleArchive().m_emitterInfos[emitterId].m_skeletonModel);
        if (!m_modelInstance)
        {
            return;
        }
        auto curLodLevel = m_actorInstance->GetLODLevel();
        auto motionSys = m_actorInstance->GetMotionSystem();
        auto isPlayMotion = motionSys->GetIsPlaying();
        if (curLodLevel == m_lastLodLevel && !isPlayMotion)
        {
            return;
        }

        bool lodChanged{false};
        auto& allMeshIndices = m_indiceStreams.at(emitterId);
        auto& allMeshVertexes = m_vertexStreams.at(emitterId);
        auto& allFaceAreas = m_areaStreams.at(emitterId);

        auto lodAssets = m_modelInstance->GetModelAsset()->GetLodAssets();
        auto subMeshes = lodAssets[curLodLevel]->GetMeshes();
        if (m_lastLodLevel != curLodLevel)
        {
            lodChanged = true;
            ResetMeshBuffers(subMeshes, allMeshIndices, allMeshVertexes, allFaceAreas);
        }
        m_lastLodLevel = curLodLevel;

        AZ::u32 curMeshVertexOffset{0};
        AZ::u32 curMeshIndceOffset{0};
        for (size_t j = 0; j < subMeshes.size(); j++)
        {
            if (lodChanged)
            {
                // Merge sub mesh indices
                const auto subMeshIndince = subMeshes[j].GetIndexBufferTyped<AZ::u32>();
                std::transform(subMeshIndince.begin(), subMeshIndince.end(), allMeshIndices.begin() + curMeshIndceOffset,
                    [curMeshVertexOffset](AZ::u32 indice) {
                        return indice + curMeshVertexOffset;
                    });
                curMeshIndceOffset += subMeshes[j].GetIndexCount();
            }
            // Calculate sub mesh deformations and merge all vertex
            const auto sourcePosition = subMeshes[j].GetSemanticBufferTyped<float>(AZ::Name("POSITION"));
            const auto sourceSkinWeights = subMeshes[j].GetSemanticBufferTyped<float>(AZ::Name("SKIN_WEIGHTS"));
            const auto sourceSkinJointIndices = subMeshes[j].GetSemanticBufferTyped<AZ::u16>(AZ::Name("SKIN_JOINTINDICES"));
            const size_t numOfInfluencesPerVertex = sourceSkinWeights.size() / (sourcePosition.size() / VERTEX_DIMENSION);
            const auto& skinToSkeletonIndexMap = actor.GetSkinToSkeletonIndexMap();
            const AZ::Matrix3x4* skinningMatrices = transData.GetSkinningMatrices();
            if (sourceSkinWeights.size() == 0 || skinningMatrices == nullptr)
            {
                return;
            }

            for (size_t vertexIndex = 0; vertexIndex < sourcePosition.size() / VERTEX_DIMENSION; ++vertexIndex)
            {
                const size_t meshVertexIndex = vertexIndex * numOfInfluencesPerVertex;
                AZ::Matrix3x4 vertexSkinningTransform = AZ::Matrix3x4::CreateZero();
                for (size_t influenceIndex = 0; influenceIndex < numOfInfluencesPerVertex; ++influenceIndex)
                {
                    const size_t meshVertexInfluenceIndex = meshVertexIndex + influenceIndex;
                    const AZ::u16 jointIndex = sourceSkinJointIndices[meshVertexInfluenceIndex];
                    AZ::u16 m_jointIndex = AZStd::numeric_limits<AZ::u16>::max();
                    auto skeletonIndexIt = skinToSkeletonIndexMap.find(jointIndex);
                    if (skeletonIndexIt != skinToSkeletonIndexMap.end())
                    {
                        m_jointIndex = skeletonIndexIt->second;
                        const float jointWeight = sourceSkinWeights[meshVertexInfluenceIndex];
                        vertexSkinningTransform += skinningMatrices[m_jointIndex] * jointWeight;
                    }
                }
                const AZ::Vector3 position = vertexSkinningTransform *
                    AZ::Vector3(sourcePosition[vertexIndex * VERTEX_DIMENSION], sourcePosition[vertexIndex * VERTEX_DIMENSION + 1],
                                sourcePosition[vertexIndex * VERTEX_DIMENSION + 2]);
                allMeshVertexes.at(curMeshVertexOffset + vertexIndex) = AZ::Vector3{position.GetX(), position.GetY(), position.GetZ()};
            }
            curMeshVertexOffset += subMeshes[j].GetVertexCount();
        }
    }

    void ParticleSystem::GetMeshAreaBuffer(AZ::u32 emitterId, const EMotionFX::Actor& actor, const EMotionFX::TransformData& transData)
    {
        GetMeshVertexBuffer(emitterId, actor, transData);
        UpdateArea(emitterId);
    }

    void ParticleSystem::UpdateArea(AZ::u32 emitterId)
    {
        auto& areaStream = m_areaStreams.at(emitterId);
        auto& vertexSteam = m_vertexStreams.at(emitterId);
        auto& indiceStream = m_indiceStreams.at(emitterId);

        areaStream.resize(indiceStream.size() / FACE_DIMENSION, 0.0);

        areaStream.at(0) = CalTriArea(
            vertexSteam.at(indiceStream.at(0)),
            vertexSteam.at(indiceStream.at(1)),
            vertexSteam.at(indiceStream.at(2)));

        //calculate each triangle's area and add to cumulative
        double curArea;
        for (AZ::u32 i = FACE_DIMENSION, j = 1; i < indiceStream.size(); i = i + FACE_DIMENSION, j++)
        {
            curArea = CalTriArea(
                vertexSteam.at(indiceStream.at(i)),
                vertexSteam.at(indiceStream.at(i + 1)),
                vertexSteam.at(indiceStream.at(i + 2)));
            areaStream[j] = areaStream[j - 1] + curArea;
        }
    }


    void ParticleSystem::SetParticleLight(
        SimuCore::ParticleCore::ParticleEmitter& emitter, const SimuCore::ParticleCore::DrawItem& item)
    {

        ClearLightEffects(emitter.GetEmitterId());
        AZStd::vector<SimuCore::ParticleCore::LightParticle> lightParticles;
        emitter.GatherSimpleLight(lightParticles);
        if (lightParticles.size() != item.positionBuffer.size() || lightParticles.size() == 0)
        {
            AZ_Error("ParticleSystem", false, "particle light size doesn't match position buffer size!");
            return;
        }

        AZ::Render::SimplePointLightFeatureProcessorInterface* lightFP =
            m_scene->GetFeatureProcessor<AZ::Render::SimplePointLightFeatureProcessorInterface>();

        AZStd::vector<AZ::Render::SimplePointLightFeatureProcessorInterface::LightHandle> lights;
        for (size_t lightIndex = 0; lightIndex < item.positionBuffer.size(); ++lightIndex)
        {
            AZ::Render::SimplePointLightFeatureProcessorInterface::LightHandle light = lightFP->AcquireLight();
            auto lightParticle = lightParticles[lightIndex];
            lightFP->SetAttenuationRadius(light, lightParticle.radianScale);
            AZ::Render::PhotometricColor<AZ::Render::PhotometricUnit::Candela> color(
                AZ::Color(lightParticle.lightColor.GetR(), lightParticle.lightColor.GetG(),
                lightParticle.lightColor.GetB(), lightParticle.lightColor.GetA()));
            lightFP->SetRgbIntensity(light, color);
            AZ::Vector3 position{ item.positionBuffer[lightIndex].GetX(),
                item.positionBuffer[lightIndex].GetY(), item.positionBuffer[lightIndex].GetZ() };
            lightFP->SetPosition(light, position);
            lights.emplace_back(light);
        }
        lightHandles.emplace(emitter.GetEmitterId(), lights);
    }

    void ParticleSystem::ClearLightEffects(AZ::u32 emitterId)
    {
        AZ::Render::SimplePointLightFeatureProcessorInterface* lightFP =
            m_scene->GetFeatureProcessor<AZ::Render::SimplePointLightFeatureProcessorInterface>();

        auto iter = lightHandles.find(emitterId);
        if (iter != lightHandles.end())
        {
            for (auto lightHandle : iter->second)
            {
                lightFP->ReleaseLight(lightHandle);
            }
            lightHandles.erase(iter);
        }
    }

    void ParticleSystem::ClearAllLightEffects()
    {
        AZ::Render::SimplePointLightFeatureProcessorInterface* lightFP =
            m_scene->GetFeatureProcessor<AZ::Render::SimplePointLightFeatureProcessorInterface>();
        for (auto iter = lightHandles.begin(); iter != lightHandles.end();)
        {
            for (auto lightHandle : iter->second)
            {
                lightFP->ReleaseLight(lightHandle);
            }
            lightHandles.erase(iter++);
        }
    }

    AZ::u32 ParticleSystem::GetPipelineKey(SimuCore::ParticleCore::RenderType type) const
    {
        return static_cast<AZ::u32>(type);
    }

    void ParticleSystem::RenderParticle(const AZ::RHI::DrawListTag drawListTag, const DrawParam& drawParam,
        EmitterInstance& instance, AZ::RPI::View& view, int meshIndex)
    {
        AZ_PROFILE_SCOPE(AzCore, "ParticleSystem::RenderParticle");
        auto& efd = instance.m_emitterForDrawPair[drawParam.shader.get()];
        auto pipeline = m_particleFp->Fetch(GetPipelineKey(drawParam.item.type));
        AZ::RHI::PipelineStateDescriptorForDraw pipelineStateDescriptor;
        auto& shaderOptions = efd.optionGroup;
        drawParam.variant.ConfigurePipelineState(pipelineStateDescriptor, shaderOptions);
        AZ::RHI::MergeStateInto(efd.states, pipelineStateDescriptor.m_renderStates);

        auto buffer = reinterpret_cast<PtrHolder<AZ::RHI::Buffer>*>(drawParam.item.vertexBuffer.buffer.data.ptr);
        AZ::RHI::StreamBufferView bufferView = {*buffer->ptr, drawParam.item.vertexBuffer.offset,
            drawParam.item.vertexBuffer.size, drawParam.item.vertexBuffer.stride };

        auto& geometryView = efd.m_geometryView;
        geometryView.ClearStreamBufferViews();

        if (meshIndex >= 0) { // mesh particle
            instance.m_model.SetParticleStreamBufferView(bufferView, meshIndex);
            if (!instance.m_model.IsInputStreamLayoutsValid(meshIndex))
            {
                return;
            }
            for (const auto& meshBufferView : instance.m_model.GetStreamBufferViewList(meshIndex))
            {
                geometryView.AddStreamBufferView(meshBufferView);
            }
            pipelineStateDescriptor.m_inputStreamLayout = instance.m_model.GetInputStreamLayout(meshIndex);
        } else {
            geometryView.AddStreamBufferView(bufferView);
            pipelineStateDescriptor.m_inputStreamLayout = pipeline->m_streamLayout;
        }

        AZ::RHI::DrawPacketBuilder drawPacketBuilder(AZ::RHI::MultiDevice::AllDevices);
        drawPacketBuilder.Begin(nullptr);
        AZ::RHI::DrawInstanceArguments instanceArguments;

        if (drawParam.item.drawArgs.type == SimuCore::ParticleCore::DrawType::LINEAR)
        { // sprite particle
            AZ::RHI::DrawLinear linear;
            linear.m_vertexCount = drawParam.item.drawArgs.linear.vertexCount;
            linear.m_vertexOffset = 0;
            instanceArguments.m_instanceCount = drawParam.item.drawArgs.linear.instanceCount;

            geometryView.SetDrawArguments(linear);
            drawPacketBuilder.SetDrawInstanceArguments(instanceArguments);
            drawPacketBuilder.AddShaderResourceGroup(instance.m_objSrg->GetRHIShaderResourceGroup());
            AZ_Assert(instance.m_objSrg->GetRHIShaderResourceGroup(), "srg nullptr");
            AZ_Assert(efd.m_drawSrg->GetRHIShaderResourceGroup(), "srg nullptr");
        }
        else
        {
            if (meshIndex >= 0)
            { // mesh particle
                const auto& mesh = instance.m_model.GetModelLod()->GetMeshes()[meshIndex];
                instanceArguments.m_instanceCount = drawParam.item.drawArgs.linear.instanceCount;

                geometryView.SetDrawArguments(mesh.GetDrawArguments());
                geometryView.SetIndexBufferView(mesh.GetIndexBufferView());

                drawPacketBuilder.SetDrawInstanceArguments(instanceArguments);
                drawPacketBuilder.AddShaderResourceGroup(instance.m_objSrg->GetRHIShaderResourceGroup());
                AZ_Assert(instance.m_objSrg->GetRHIShaderResourceGroup(), "srg nullptr");
            }
            else
            { // ribbon particle
                AZ::RHI::DrawIndexed indexed;
                indexed.m_indexCount = drawParam.item.drawArgs.indexed.indexCount;
                auto indexBuffer = reinterpret_cast<PtrHolder<AZ::RHI::Buffer>*>(
                    drawParam.item.indexBuffer.buffer.data.ptr);
                AZ::RHI::IndexBufferView indexBufferView = { *indexBuffer->ptr,
                    drawParam.item.indexBuffer.offset, drawParam.item.indexBuffer.size, AZ::RHI::IndexFormat::Uint32 };
                geometryView.SetDrawArguments(indexed);
                geometryView.SetIndexBufferView(indexBufferView);
            }
        }
        drawPacketBuilder.SetGeometryView(&geometryView);
        drawPacketBuilder.AddShaderResourceGroup(instance.m_material->GetRHIShaderResourceGroup());
        AZ_Assert(instance.m_material->GetRHIShaderResourceGroup(), "srg nullptr");

        m_scene->ConfigurePipelineState(drawListTag, pipelineStateDescriptor);
        const AZ::RHI::PipelineState* pipelineState = drawParam.shader->AcquirePipelineState(pipelineStateDescriptor);
        AZ_Assert(pipelineState, "pipeline nullptr");

        AZ::RHI::DrawPacketBuilder::DrawRequest drawRequest;
        drawRequest.m_listTag = drawListTag;
        drawRequest.m_pipelineState = pipelineState;
        drawRequest.m_sortKey = 0;
        if (efd.m_drawSrg)
        {
            drawRequest.m_uniqueShaderResourceGroup = efd.m_drawSrg->GetRHIShaderResourceGroup();
        }
        if (meshIndex >= 0)
        {
            drawRequest.m_stencilRef = (MaterialRequiresForwardPassIblSpecular(instance.m_material) ?
                AZ::Render::StencilRefs::None : AZ::Render::StencilRefs::UseIBLSpecularPass) |
                AZ::Render::StencilRefs::UseDiffuseGIPass;
        }

        drawRequest.m_streamIndices = geometryView.GetFullStreamBufferIndices();
        drawPacketBuilder.AddDrawItem(drawRequest);

        AZ::RHI::ConstPtr<AZ::RHI::DrawPacket> drawPacket = drawPacketBuilder.End();
        if (drawPacket != nullptr)
        {
            m_drawPacket.emplace_back(drawPacket);
            view.AddDrawPacket(drawPacket.get());
        }
    }

    void ParticleSystem::Rebuild()
    {
        auto emitters = m_particleSystem->GetAllEmitters();
        auto& archive = m_asset->GetParticleArchive();
        auto& lods = m_asset->GetLODs();
        auto& dist = m_asset->GetDistribution();

        AZStd::vector<SimuCore::ParticleCore::LevelsOfDetail> levelsOfDetail;
        for (auto& lod : lods) {
            AZStd::vector<AZ::u32> indexes;
            for (auto& index : lod.m_emitters) {
                indexes.emplace_back(index);
            }
            levelsOfDetail.emplace_back(SimuCore::ParticleCore::LevelsOfDetail{ lod.m_distance, indexes });
        }
        m_particleSystem->SetLODs(levelsOfDetail);

        AZ::u32 maxParticleNum = m_particleSystem->GetMaxParticleNum();
        for (auto& random : dist.randoms)
        {
            m_particleSystem->AddRandom(random.min, random.max, random.tickMode, maxParticleNum);
        }

        for (auto& curve : dist.curves)
        {
            auto curvePtr =
                m_particleSystem->AddCurve(curve.extrapModeLeft, curve.extrapModeRight, curve.timeFactor, curve.valueFactor, curve.tickMode);
            for (auto& point : curve.keyPoints)
            {
                curvePtr->AddKeyPoint(point.time, point.value, point.interpMode);
            }
            curvePtr->NormalizeKeyPoints();
        }

        m_particleSystem->UpdateDistribution();

        for (auto& emitter : emitters)
        {
            emitter.second->SetEmitterTransform(m_transform);
            auto key = GetPipelineKey(emitter.second->GetRenderType());
            auto pipelineState = m_particleFp->FetchOrCreate(key);
            if (pipelineState == nullptr)
            {
                continue;
            }

            auto it = m_emitterInstances.emplace(emitter.second, EmitterInstance());
            auto& efd = it.first->second;
            efd.m_scene = m_scene;
            if (archive.m_emitterInfos[emitter.first].m_render.first == azrtti_typeid<SimuCore::ParticleCore::MeshConfig>())
            {
                efd.m_model.SetupModel(archive.m_emitterInfos[emitter.first].m_model, 0);
                AZ::Aabb aabb = archive.m_emitterInfos[emitter.first].m_model->GetAabb();
                emitter.second->SetAabbExtends(aabb.GetMax(), aabb.GetMin());
            }

            HandleSkeletonModel(*emitter.second);
            efd.Setup(archive.m_emitterInfos[emitter.first].m_material);
            AZ::RHI::ShaderInputNameIndex objectIdIndex = "m_objectId";
            if (efd.m_objSrg != nullptr)
            {
                efd.m_objSrg->SetConstant(objectIdIndex, m_objectId.GetIndex());
                efd.m_objSrg->Compile();
            }
        }
    }

    void ParticleSystem::HandleSkeletonModel(SimuCore::ParticleCore::ParticleEmitter& emitter)
    {
        auto& archive = m_asset->GetParticleArchive();
        AZ::u32 emitterId = emitter.GetEmitterId();
        for (auto effector : archive.m_emitterInfos[emitterId].m_effectors)
        {
            if (AZStd::get<0>(effector) != azrtti_typeid<SimuCore::ParticleCore::SpawnLocSkeleton>())
            {
                continue;
            }
            emitter.SetMeshSampleType(archive.m_emitterInfos[emitterId].m_meshSampleType);
            AZ::Data::Instance<AZ::RPI::Model> m_modelInstance =
                    AZ::RPI::Model::FindOrCreate(archive.m_emitterInfos[emitterId].m_skeletonModel);
            if (!m_modelInstance)
            {
                continue;
            }

            auto vexIter = m_vertexStreams.find(emitterId);
            if (vexIter == m_vertexStreams.end())
            {
                m_vertexStreams.emplace(emitterId, AZStd::vector<AZ::Vector3>{});
                m_indiceStreams.emplace(emitterId, AZStd::vector<AZ::u32>{});
                m_areaStreams.emplace(emitterId, AZStd::vector<double>{});
            }
            auto boneIter = m_boneStreams.find(emitterId);
            if (boneIter == m_boneStreams.end())
            {
                m_boneStreams.emplace(emitterId, AZStd::vector<AZ::Vector3>{});
            }

            auto& allBones = m_boneStreams.at(emitterId);
            auto& allMeshIndices = m_indiceStreams.at(emitterId);
            auto& allMeshVertexes = m_vertexStreams.at(emitterId);
            auto& allFaceAreas = m_areaStreams.at(emitterId);

            // Load LOD Level 0 by default
            m_lastLodLevel = 0;
            auto lodAssets = m_modelInstance->GetModelAsset()->GetLodAssets();
            auto subMeshes = lodAssets[m_lastLodLevel]->GetMeshes();
            ResetMeshBuffers(subMeshes, allMeshIndices, allMeshVertexes, allFaceAreas);

            AZ::u32 curMeshVertexOffset{0};
            AZ::u32 curMeshIndceOffset{0};
            for (size_t j = 0; j < subMeshes.size(); j++)
            {
                // Merge sub mesh indices
                const auto subMeshIndince = subMeshes[j].GetIndexBufferTyped<AZ::u32>();
                std::transform(subMeshIndince.begin(), subMeshIndince.end(), allMeshIndices.begin() + curMeshIndceOffset,
                    [curMeshVertexOffset](AZ::u32 indice) {
                        return indice + curMeshVertexOffset;
                    });
                curMeshIndceOffset += subMeshes[j].GetIndexCount();

                // Merge sub mesh vertexes
                const auto sourcePosition = subMeshes[j].GetSemanticBufferTyped<SimpleVec3>(AZ::Name("POSITION"));
                for (size_t vertexIndex = 0; vertexIndex < sourcePosition.size(); ++vertexIndex)
                {
                    AZ::Vector3 curPosition = {
                        sourcePosition[vertexIndex].x,
                        sourcePosition[vertexIndex].y,
                        sourcePosition[vertexIndex].z
                    };
                    allMeshVertexes.at(curMeshVertexOffset + vertexIndex) = curPosition;
                }
                curMeshVertexOffset += static_cast<AZ::u32>(sourcePosition.size());
            }
            if (emitter.GetMeshSampleType() == SimuCore::ParticleCore::MeshSampleType::AREA)
            {
                UpdateArea(emitterId);
            }

            emitter.SetSkeletonMesh(
                allBones.data(), static_cast<AZ::u32>(allBones.size()),
                allMeshVertexes.data(), static_cast<AZ::u32>(allMeshVertexes.size()),
                allMeshIndices.data(), static_cast<AZ::u32>(allMeshIndices.size()),
                allFaceAreas.data());
        }

    }
} // namespace OpenParticle
