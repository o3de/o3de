/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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

#include <Rendering/HairFeatureProcessor.h>
#include <Components/HairComponentController.h>

namespace AZ
{
    namespace Render
    {
        namespace Hair
        {
            HairComponentController::~HairComponentController()
            {
                RemoveHairObject();
            }

            void HairComponentController::Reflect(ReflectContext* context)
            {
                HairComponentConfig::Reflect(context);

                if (auto* serializeContext = azrtti_cast<SerializeContext*>(context))
                {
                    serializeContext->Class<HairComponentController>()
                        ->Version(2)
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
                // Dependency in the Actor due to the need to get the bone / joint matrices
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
                    m_featureProcessor->SetHairGlobalSettings(m_configuration.m_hairGlobalSettings);
                    if (!m_renderObject)
                    {
                        // Call this function if object doesn't exist to trigger the load of the existing asset
                        OnHairAssetChanged();
                    }
                }

                EMotionFX::Integration::ActorComponentNotificationBus::Handler::BusConnect(m_entityId);
                HairRequestsBus::Handler::BusConnect(m_entityId);
                TickBus::Handler::BusConnect();
                HairGlobalSettingsNotificationBus::Handler::BusConnect();
            }

            void HairComponentController::Deactivate()
            {
                HairRequestsBus::Handler::BusDisconnect(m_entityId);
                EMotionFX::Integration::ActorComponentNotificationBus::Handler::BusDisconnect(m_entityId);
                Data::AssetBus::MultiHandler::BusDisconnect();
                TickBus::Handler::BusDisconnect();
                HairGlobalSettingsNotificationBus::Handler::BusDisconnect();

                RemoveHairObject();
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
                if (m_configuration.m_hairAsset.GetId().IsValid())
                {
                    Data::AssetBus::MultiHandler::BusConnect(m_configuration.m_hairAsset.GetId());
                    m_configuration.m_hairAsset.QueueLoad();
                }
                else
                {
                    RemoveHairObject();
                }
            }

            void HairComponentController::OnHairGlobalSettingsChanged(const HairGlobalSettings& hairGlobalSettings)
            {
                m_configuration.m_hairGlobalSettings = hairGlobalSettings;
            }

            void HairComponentController::RemoveHairObject()
            {
                if (m_featureProcessor)
                {
                    m_featureProcessor->RemoveHairRenderObject(m_renderObject);
                }
                m_renderObject.reset();
            }

            void HairComponentController::OnHairConfigChanged()
            {
                // The actual config change to render object happens in the onTick function. We do this to make sure it 
                // always happens pre-rendering. There is no need to do it before render object created, because the object
                // will always be created with the updated configuration.
                if (m_renderObject)
                {
                    m_configChanged = true;
                }
            }

            void HairComponentController::OnAssetReady(Data::Asset<Data::AssetData> asset)
            {
                if (asset.GetId() == m_configuration.m_hairAsset.GetId())
                {
                    m_configuration.m_hairAsset = asset;
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
                    const float MAX_SIMULATION_TIME_STEP = 0.033f;  // Assuming minimal of 30 fps
                    float currentDeltaTime = AZStd::min(deltaTime, MAX_SIMULATION_TIME_STEP);
                    m_renderObject->UpdateSimulationParameters(&m_configuration.m_simulationSettings, currentDeltaTime);

                    // [To Do] Hair - Allow update of the following settings to control dynamic LOD
                    const float distanceFromCamera = 1.0f;
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

                // Update the enable flag for hair render object
                // The enable flag depends on the visibility of render actor instance and the flag of hair configuration.
                bool actorVisible = false;
                EMotionFX::Integration::ActorComponentRequestBus::EventResult(
                    actorVisible, m_entityId, &EMotionFX::Integration::ActorComponentRequestBus::Events::GetRenderActorVisible);
                m_renderObject->SetEnabled(actorVisible);

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

                m_entityWorldMatrix = Matrix3x4::CreateFromTransform(actorInstance->GetWorldSpaceTransform().ToAZTransform());
                m_renderObject->UpdateBoneMatrices(m_entityWorldMatrix, m_cachedHairBoneMatrices);
                return true;
            }

            bool HairComponentController::GenerateLocalToGlobalBoneIndex(
                EMotionFX::ActorInstance* actorInstance, AMD::TressFXAsset* hairAsset)
            {
                // Generate local TressFX to global EMFX bone index lookup.
                AMD::BoneNameToIndexMap globalNameToIndexMap;
                const EMotionFX::Skeleton* skeleton = actorInstance->GetActor()->GetSkeleton();
                if (!skeleton)
                {
                    AZ_Error("Hair Gem", false, "Actor could not retrieve his skeleton.");
                    return false;
                }

                const uint32_t numBones = uint32_t(skeleton->GetNumNodes());
                globalNameToIndexMap.reserve(size_t(numBones));
                for (uint32_t i = 0; i < numBones; ++i)
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

                return true;
            }

            // The hair object will only be created if both conditions are met:
            //  1. The hair asset is loaded 
            //  2. The actor instance is created
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

                if (!m_configuration.m_hairAsset.GetId().IsValid() || !m_configuration.m_hairAsset.IsReady())
                {
                    AZ_Warning("Hair Gem", false, "Hair Asset was not ready - second attempt will be made when ready");
                    return false;
                }

                AMD::TressFXAsset* hairAsset = m_configuration.m_hairAsset.Get()->m_tressFXAsset.get();
                if (!hairAsset)
                {
                    AZ_Error("Hair Gem", false, "Hair asset could not be loaded");
                    return false;
                }

                if (!GenerateLocalToGlobalBoneIndex(actorInstance, hairAsset))
                {
                    return false;
                }
                
                // First remove the existing hair object - this can happen if the configuration
                // or the hair asset selected changes.
                RemoveHairObject();

                // create a new instance - will remove the old one.
                m_renderObject.reset(new HairRenderObject());
                AZStd::string hairName;
                AzFramework::StringFunc::Path::GetFileName(m_configuration.m_hairAsset.GetHint().c_str(), hairName);
                if (!m_renderObject->Init( m_featureProcessor, hairName.c_str(), hairAsset,
                    &m_configuration.m_simulationSettings, &m_configuration.m_renderingSettings))
                {
                    AZ_Warning("Hair Gem", false, "Hair object was not initialize succesfully");
                    m_renderObject.reset();    // no instancing yet - remove manually
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

        } // namespace Hair
    } // namespace Render
} // namespace AZ
