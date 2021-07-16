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

#include <AzCore/RTTI/BehaviorContext.h>

#include <Atom/RPI.Public/Scene.h>

#include <AzFramework/Components/TransformComponent.h>

#include <Integration/Components/ActorComponent.h>
#include <EMotionFX/Source/TransformData.h>
#include <EMotionFX/Source/ActorInstance.h>
#include <EMotionFX/Source/Node.h>

// Hair Specific
#include <TressFX/TressFXAsset.h>
#include <TressFX/TressFXSettings.h>

#include <Rendering/HairRenderObject.h>
#include <Rendering/HairFeatureProcessor.h>
#include <Components/HairComponentController.h>



#pragma optimize("", off)

namespace AZ
{
    namespace Render
    {
        namespace Hair
        {

            HairComponentController::~HairComponentController()
            {
                if (m_featureProcessor)
                {
                    m_featureProcessor->RemoveHairRenderObject(m_renderObject);
                }
                // the memory should NOW be removed by the instancing mechanism once it is
                // registered in the feature processor.
                // When the feature processor will remove its instance, it'll be deleted.
                m_renderObject = nullptr;
            }

            void HairComponentController::Reflect(ReflectContext* context)
            {
                HairComponentConfig::Reflect(context);

                if (auto* serializeContext = azrtti_cast<SerializeContext*>(context))
                {
                    serializeContext->Class<HairComponentController>()
                        ->Version(1)
                        ->Field("HairAsset", &HairComponentController::m_hairAsset)
                        ->Field("Configuration", &HairComponentController::m_configuration)
                        ;
                }

                if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
                {
                    behaviorContext->EBus<HairRequestsBus>("HairRequestsBus")
                        ->Attribute(AZ::Script::Attributes::Module, "render")
                        ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)

                        // Insert auto-gen behavior context here...
                        ;
                }
            }

            void HairComponentController::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
            {
                provided.push_back(AZ_CRC_CE("HairService"));
            }

            void HairComponentController::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
            {
                incompatible.push_back(AZ_CRC_CE("HairService"));
            }

            void HairComponentController::GetRequiredServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& required)
            {
                // Dependency in the Actor due to the need to get the bone / join matrices
//                required.push_back(AZ_CRC("ActorSystemService", 0x5e493d6c));
                required.push_back(AZ_CRC_CE("EMotionFXActorService"));
            }

            HairComponentController::HairComponentController(const HairComponentConfig& config)
                : m_configuration(config)
            {
            }


            void HairComponentController::Activate(EntityId entityId)
            {
                m_entityId = entityId;

                m_featureProcessor = RPI::Scene::GetFeatureProcessorForEntity<Hair::HairFeatureProcessor>(m_entityId);
                if (m_featureProcessor)
                {
                    // Call this function to trigger the load of the existing asset
                    OnHairAssetChanged();
                }

                // [To Do] Adi: affect / show the hair object via the feature processor

                EMotionFX::Integration::ActorComponentNotificationBus::Handler::BusConnect(m_entityId);
                HairRequestsBus::Handler::BusConnect(m_entityId);
                TickBus::Handler::BusConnect();
            }

            void HairComponentController::Deactivate()
            {
                HairRequestsBus::Handler::BusDisconnect(m_entityId);
                EMotionFX::Integration::ActorComponentNotificationBus::Handler::BusDisconnect(m_entityId);
                Data::AssetBus::MultiHandler::BusDisconnect();
                TickBus::Handler::BusDisconnect();

                m_entityId.SetInvalid();
            }

            void HairComponentController::SetConfiguration(const HairComponentConfig& config)
            {
                m_configuration = config;
                OnHairConfigChanged();
            }

            const HairComponentConfig& HairComponentController::GetConfiguration() const
            {
                return m_configuration;
            }

            void HairComponentController::OnHairAssetChanged()
            {
                Data::AssetBus::MultiHandler::BusDisconnect();
                if (m_hairAsset.GetId().IsValid())
                {
                    Data::AssetBus::MultiHandler::BusConnect(m_hairAsset.GetId());
                    m_hairAsset.QueueLoad();
                }
                else
                {
                    RemoveHairObject();
                }
            }

            void HairComponentController::RemoveHairObject()
            {
                if (m_renderObject && m_featureProcessor)
                {
                    m_featureProcessor->RemoveHairRenderObject(m_renderObject);
                    m_renderObject = nullptr; // Actual removal is via the instance mechanism in the feature processor
                }
            }

            void HairComponentController::OnHairConfigChanged()
            {
                // The actual config change to render object happens in the onTick function. We do this to make sure it 
                // always happen pre-rendering. There is no need to do it before render object created, because the object
                // will always be created with the updated configuration.
                if (m_renderObject)
                {
                    m_configChanged = true;
                }
            }

            void HairComponentController::OnAssetReady(Data::Asset<Data::AssetData> asset)
            {
                if (asset.GetId() == m_hairAsset.GetId())
                {
                    m_hairAsset = asset;
                    CreateHairObject();
                }
            }

            void HairComponentController::OnAssetReloaded(Data::Asset<Data::AssetData> asset)
            {
                OnAssetReady(asset);
            }

            void HairComponentController::OnActorInstanceCreated([[maybe_unused]]EMotionFX::ActorInstance* actorInstance)
            {
                CreateHairObject();
            }

            void HairComponentController::OnActorInstanceDestroyed([[maybe_unused]]EMotionFX::ActorInstance* actorInstance)
            {
                RemoveHairObject();
            }

            void HairComponentController::OnTick([[maybe_unused]]float deltaTime, [[maybe_unused]]AZ::ScriptTimePoint time)
            {
                if (!m_renderObject)
                {
                    return;
                }

                // Config change to renderObject happens on the OnTick, so we know the it occurs before render update.
                if (m_configChanged)
                {
                    m_renderObject->UpdateSimulationParameters(
                        &m_configuration.m_simulationSettings, 0.035f /*default value in hairRenderObject*/);

                    // [To Do] Adi: change the following settings before the final submit.
                    const float distanceFromCamera = 1.0;
                    const float updateShadows = false;
                    m_renderObject->UpdateRenderingParameters(
                        &m_configuration.m_renderingSettings, RESERVED_PIXELS_FOR_OIT, distanceFromCamera, updateShadows);
                    m_configChanged = false;

                    // Only load the image asset when the dirty flag has been set on the settings.
                    if (m_configuration.m_renderingSettings.m_imgDirty)
                    {
                        m_renderObject->LoadImageAsset(&m_configuration.m_renderingSettings);
                        m_configuration.m_renderingSettings.m_imgDirty = false;
                    }
                }

                // Optional - move this to be done by the feature processor
 //               m_renderObject->SetFrameDeltaTime(deltaTime);

                // Update the enable flag for hair render object
                // The enable flag depends on the visibility of render actor instance and the flag of hair configuration.
                bool actorVisible = false;
                EMotionFX::Integration::ActorComponentRequestBus::EventResult(
                    actorVisible, m_entityId, &EMotionFX::Integration::ActorComponentRequestBus::Events::GetRenderActorVisible);
                m_renderObject->SetEnabled(actorVisible && m_configuration.GetIsEnabled());

                UpdateActorMatrices();
            }

            int HairComponentController::GetTickOrder()
            {
                return AZ::TICK_PRE_RENDER;
            }

            bool HairComponentController::UpdateActorMatrices()
            {
                if (!m_renderObject->IsEnabled())
                {
                    return false;
                }

                EMotionFX::ActorInstance* actorInstance = nullptr;
                EMotionFX::Integration::ActorComponentRequestBus::EventResult(
                    actorInstance, m_entityId, &EMotionFX::Integration::ActorComponentRequestBus::Events::GetActorInstance);
                if (!actorInstance)
                {
                    return false;
                }

                const EMotionFX::TransformData* transformData = actorInstance->GetTransformData();
                if (!transformData)
                {
                    AZ_WarningOnce("Hair Gem", false, "Error getting the transformData from the actorInstance.");
                    return false;
                }

                // In EMotionFX the skinning matrices is stored as a 3x4. The conversion to 4x4 matrices happens at the update bone matrices function.
                // In here we use the boneIndexMap to find the correct EMotionFX bone index (also as the global bone index), and passing the
                // matrices of those bones to the hair render object. We do this for both hair and collision bone matrices.
                const AZ::Matrix3x4* matrices = transformData->GetSkinningMatrices();
                for (AZ::u32 tressFXBoneIndex = 0; tressFXBoneIndex < m_cachedHairBoneMatrices.size(); ++tressFXBoneIndex)
                {
                    const AZ::u32 emfxBoneIndex = m_hairBoneIndexLookup[tressFXBoneIndex];
                    m_cachedHairBoneMatrices[tressFXBoneIndex] = matrices[emfxBoneIndex];
                }
                for (AZ::u32 tressFXBoneIndex = 0; tressFXBoneIndex < m_cachedCollisionBoneMatrices.size(); ++tressFXBoneIndex)
                {
                    const AZ::u32 emfxBoneIndex = m_collisionBoneIndexLookup[tressFXBoneIndex];
                    m_cachedCollisionBoneMatrices[tressFXBoneIndex] = matrices[emfxBoneIndex];
                }

                AZStd::lock_guard<AZStd::mutex> lock(m_mutex);
                m_entityWorldMatrix = Matrix3x4::CreateFromTransform(actorInstance->GetWorldSpaceTransform().ToAZTransform());
                m_renderObject->UpdateBoneMatrices(m_entityWorldMatrix, m_cachedHairBoneMatrices);
                return true;
            }

            // The hair object will only be created when 1) The hair asset is loaded AND 2) The actor instance is created.
            bool HairComponentController::CreateHairObject()
            {
                // Do not create a hairRenderObject when actor instance hasn't been created.
                EMotionFX::ActorInstance* actorInstance = nullptr;
                EMotionFX::Integration::ActorComponentRequestBus::EventResult(
                    actorInstance, m_entityId, &EMotionFX::Integration::ActorComponentRequestBus::Events::GetActorInstance);
                if (!actorInstance)
                {
                    return false;
                }

                if (!m_featureProcessor)
                {
                    AZ_Error("Hair Gem", false, "Required feature processor does not exist yet");
                    return false;
                }

                if (!m_hairAsset.GetId().IsValid() || !m_hairAsset.IsReady())
                {
                    AZ_Warning("Hair Gem", false, "Hair Asset was not ready - second attempt will be made when ready");
                    return false;
                }

                AMD::TressFXAsset* hairAsset = m_hairAsset.Get()->m_tressFXAsset.get();
                if (!hairAsset)
                {
                    AZ_Error("Hair Gem", false, "Hair asset could not be loaded");
                    return false;
                }

                // Generate local TressFX to global Emfx bone index lookup.
                AMD::BoneNameToIndexMap globalNameToIndexMap; 
                const EMotionFX::Skeleton* skeleton = actorInstance->GetActor()->GetSkeleton();
                const u32 numBones = skeleton->GetNumNodes();
                globalNameToIndexMap.reserve(numBones);
                for (u32 i = 0; i < numBones; ++i)
                {
                    const char* boneName = skeleton->GetNode(i)->GetName();
                    globalNameToIndexMap[boneName] = i;
                }

                if (!hairAsset->GenerateLocaltoGlobalHairBoneIndexLookup(globalNameToIndexMap, m_hairBoneIndexLookup) ||
                    !hairAsset->GenerateLocaltoGlobalCollisionBoneIndexLookup(globalNameToIndexMap, m_collisionBoneIndexLookup))
                {
                    AZ_Error("Hair Gem", false, "Cannot convert local bone index to global bone index. The hair asset may not be compatible with the actor.");
                    return false;
                }
                
                // First remove the existing hair object - this can happen if the configuration
                // or the hair asset selected changes.
                RemoveHairObject();

                // create a new instance - will remove the old one.
                m_renderObject = new HairRenderObject();
                AZStd::string hairName;
                AzFramework::StringFunc::Path::GetFileName(m_hairAsset.GetHint().c_str(), hairName);
                if (!m_renderObject->Init( m_featureProcessor, hairName.c_str(), hairAsset,
                    &m_configuration.m_simulationSettings, &m_configuration.m_renderingSettings))
                {
                    AZ_Warning("Hair Gem", false, "Hair object was not initialize succesfully");
                    SAFE_DELETE(m_renderObject);    // no instancing yet - remove manually
                    return false;
                }

                // Resize the bone matrices array. The size should equal to the number of bones in the tressFXAsset.
                m_cachedHairBoneMatrices.resize(m_hairBoneIndexLookup.size());
                m_cachedCollisionBoneMatrices.resize(m_collisionBoneIndexLookup.size());

                // Feature processor registration that will hold an instance.
                // Remark: DO NOT remove the TressFX asset - it's data might be required for
                // more instance hair objects. 
                m_featureProcessor->AddHairRenderObject(m_renderObject);
                return true;
            }
            // [To Do] Adi: add auto generated getter/setter functions...

        } // namespace Hair
    } // namespace Render
} // namespace AZ

#pragma optimize("", on)
