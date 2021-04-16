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
//#include <TressFX/AMD_Types.h>
//#include <TressFX/AMD_TressFX.h>
//#include <TressFX/TressFXCommon.h>

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
                delete m_simDefaultSettings;
                delete m_renderDefaultSettings;
                delete m_renderObject;

                // Notice the following - the memory should be removed by the
                // instancing mechanism once it is registered in the feature processor.
                // When the feature processor will remove its instance, it'll be deleted.
                m_renderObject = nullptr;
                m_entity = nullptr;
            }

            void HairComponentController::Reflect(ReflectContext* context)
            {
                HairComponentConfig::Reflect(context);

                if (auto* serializeContext = azrtti_cast<SerializeContext*>(context))
                {
                    serializeContext->Class<HairComponentController>()
                        ->Version(0)
                        ->Field("Configuration", &HairComponentController::m_configuration);
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
                /*
                AZ::Entity* GetEntityById(m_entityId);

                AZ::Entity* entity = AzToolsFramework::GetEntityById(entityId);
                if (const AZ::Entity* entity = GetEntityById(entityId))
                {
                    if (const AZ::Component* component = entity->FindComponent<Components::TransformComponent>())
                    {
                        return component->GetId();
                    }
                }
                */
//                if (auto whiteBoxComponent = GetEntity()->FindComponent<WhiteBox::EditorWhiteBoxComponent>())
//                AZ::TransformInterface* transformHandler = GetEntity()->FindComponent<AzFramework::TransformComponent>();

                m_featureProcessor = RPI::Scene::GetFeatureProcessorForEntity<Hair::HairFeatureProcessor>(m_entityId);
                if (!m_initilized && m_featureProcessor)
                {
                    // Try to initialize the feature processor - it should be initialized only when the scene actually exists
 ////                   if (!m_featureProcessor->Init())
 ////                   {
////                        return; // adibugbug
////                    }

                    // Call this function to trigger the load of
                    OnConfigChanged();
                }

                EMotionFX::Integration::ActorComponentNotificationBus::Handler::BusConnect(m_entityId);
                HairRequestsBus::Handler::BusConnect(m_entityId);
            }

            void HairComponentController::Deactivate()
            {
                HairRequestsBus::Handler::BusDisconnect(m_entityId);
                EMotionFX::Integration::ActorComponentNotificationBus::Handler::BusDisconnect(m_entityId);
                Data::AssetBus::MultiHandler::BusDisconnect();
                AZ::TickBus::Handler::BusDisconnect();
                    // turn the lights off before leaving
//                    m_settingsInterface->SetEnabled(false);
//                    m_settingsInterface->OnSettingsChanged();
//                m_settingsInterface = nullptr;

                m_entityId.SetInvalid();
            }

            void HairComponentController::SetConfiguration(const HairComponentConfig& config)
            {
                m_configuration = config;
                OnConfigChanged();
            }

            const HairComponentConfig& HairComponentController::GetConfiguration() const
            {
                return m_configuration;
            }

            void HairComponentController::OnConfigChanged()
            {
                /*
                * [To Do] Adi: set this up in the future for settings control over hair render and simulation
                if (m_settingsInterface)
                {
                    // Set SRG constants
                    m_configuration.CopySettingsTo(m_settingsInterface);

                    // Enable / disable the pass
                    m_settingsInterface->SetEnabled(m_configuration.GetIsEnabled());

                    m_settingsInterface->OnSettingsChanged();
                }
                */

                // Load Hair Asset
                Data::AssetBus::MultiHandler::BusDisconnect();
                if (m_configuration.m_hairAsset.GetId().IsValid())
                {
                    Data::AssetBus::MultiHandler::BusConnect(m_configuration.m_hairAsset.GetId());
                    m_configuration.m_hairAsset.QueueLoad();
                }
            }

            void HairComponentController::OnAssetReady(Data::Asset<Data::AssetData> asset)
            {
                AZ_Assert(asset == m_configuration.m_hairAsset, "Unexpected asset");

                m_configuration.m_hairAsset = asset;

                // [rhhong-TODO] After the hair asset is loaded, we need to fix the skinning data to contain the bone index
                // that reconginized by the engine.
                // m_configuration.m_hairAsset->FixBoneIndexInSkinningData();

                // Hair data is ready to be used at this point.
                m_initilized = CreateHairObject();
            }

            void HairComponentController::OnAssetReloaded(Data::Asset<Data::AssetData> asset)
            {
                OnAssetReady(asset);
            }

            void HairComponentController::OnActorInstanceCreated(EMotionFX::ActorInstance* actorInstance)
            {
                m_actorInstance = actorInstance;

                if (!m_initilized)
                {
                    m_initilized = CreateHairObject();
                }
            }

            void HairComponentController::OnActorInstanceDestroyed([[maybe_unused]]EMotionFX::ActorInstance* actorInstance)
            {
                m_actorInstance = nullptr;
                // Disconnect the tick bus when actor instance got destroyed. (No need to update the bone matrices anymore).
                AZ::TickBus::Handler::BusDisconnect();
            }

            void HairComponentController::OnTick([[maybe_unused]]float deltaTime, [[maybe_unused]]AZ::ScriptTimePoint time)
            {
                UpdateActorMatrices();
            }

            int HairComponentController::GetTickOrder()
            {
                return AZ::TICK_PRE_RENDER;
            }

            // Temporary method
            void HairComponentController::InitHairDefaultParameters()
            {
                // initialize settings with default settings
                m_simDefaultSettings = new AMD::TressFXSimulationSettings;
                m_simDefaultSettings->m_vspCoeff = 0.758f;
                m_simDefaultSettings->m_vspAccelThreshold = 1.208f;
                m_simDefaultSettings->m_localConstraintStiffness = 0.908f;
                m_simDefaultSettings->m_localConstraintsIterations = 3;
                m_simDefaultSettings->m_globalConstraintStiffness = 0.408f;
                m_simDefaultSettings->m_globalConstraintsRange = 0.308f;
                m_simDefaultSettings->m_lengthConstraintsIterations = 3;
                m_simDefaultSettings->m_damping = 0.068f;
                m_simDefaultSettings->m_gravityMagnitude = 0.09f;

                // Adi: change the following texture to match the sample's texture
                m_renderDefaultSettings = new AMD::TressFXRenderingSettings;
                m_renderDefaultSettings->m_BaseAlbedoName = "testdata/test_hair_basecolor.png.streamingimage";
                m_renderDefaultSettings->m_EnableThinTip = true;
                m_renderDefaultSettings->m_FiberRadius = 0.002f;
                m_renderDefaultSettings->m_FiberRatio = 0.06f;
                m_renderDefaultSettings->m_HairKDiffuse = 0.22f;
                m_renderDefaultSettings->m_HairKSpec1 = 0.012f;
                m_renderDefaultSettings->m_HairSpecExp1 = 14.40f;
                m_renderDefaultSettings->m_HairKSpec2 = 0.136f;
                m_renderDefaultSettings->m_HairSpecExp2 = 11.80f;
            }


            /*
            size_t ActorComponent::GetJointIndexByName(const char* name) const
            {
            AZ_Assert(m_actorInstance, "The actor instance needs to be valid.");

            Node* node = m_actorInstance->GetActor()->GetSkeleton()->FindNodeByNameNoCase(name);
            if (node)
            {
            return static_cast<size_t>(node->GetNodeIndex());
            }

            return ActorComponentRequests::s_invalidJointIndex;
            }
            */

            bool HairComponentController::CreateBonesIndicesLUT(
                std::vector<std::string>& bonesNames,
                std::vector<int32_t>& bonesIndicesLUT
            )
            {
                uint32_t bonesNum = bonesNames.size();
                bonesIndicesLUT.resize(bonesNum);
                // start by initializing to straight regular order
                for (int32_t bone = 0; bone < bonesNum; ++bone)
                {
                    bonesIndicesLUT[bone] = bone;  
                }

                // Get the Actor instance from which we can get the data
                EMotionFX::ActorInstance* actorInstance = nullptr;
                EMotionFX::Integration::ActorComponentRequestBus::EventResult(actorInstance, m_entityId,
                    &EMotionFX::Integration::ActorComponentRequestBus::Events::GetActorInstance);
                if (!actorInstance)
                {
                    AZ_Warning("HairComponentController", false, "Error getting the ActorInstance");
                }
                //// Moving to another technique - was the instance retrieved?
                
                // [To Do] Adi : verify correctness of bone space.
                // Match bones / joins names to the global index in the Actor joints array
                bool allIndicesFound = true;
                AZStd::vector<AZ::Transform> boneTransforms(bonesNames.size());
                for (int32_t bone = 0; bone < bonesNames.size(); ++bone)
                {
                    size_t boneIndex; // = actorComponent->GetJointIndexByName(bonesNames[bone].c_str());
                    EMotionFX::Integration::ActorComponentRequestBus::EventResult(
                        boneIndex, m_entityId,
                        &EMotionFX::Integration::ActorComponentRequestBus::Events::GetJointIndexByName,
                        bonesNames[bone].c_str()
                    );

                    if (boneIndex == -1)
                    {
                        allIndicesFound = false;
                        // [To Do] Adi: change the following back to -1.0 when acquiring Actor.
                        boneIndex = 0;  // set to be the origin - this is a default fall back for now.
                    }

                    bonesIndicesLUT[bone] = boneIndex;

                    // The following section is for validity only and for future ref when
                    // we need to update the matrices given the index we collect here.
//                    boneTransforms[++bone] = actorComponent->GetJointTransform(boneIndex, EMotionFX::Integration::Space::WorldSpace );
                    EMotionFX::Integration::ActorComponentRequestBus::EventResult(
                        boneTransforms[bone], m_entityId,
                        &EMotionFX::Integration::ActorComponentRequestBus::Events::GetJointTransform,
                        boneIndex,
                        EMotionFX::Integration::Space::LocalSpace
                    );
                }
                return allIndicesFound;
            }

            bool HairComponentController::UpdateActorMatrices()
            {
                if (!m_renderObject)
                {
                    AZ_WarningOnce("HairComponentController", false, "Error getting the renderObject.");
                    return false;
                }

                if (!m_actorInstance)
                {
                    AZ_WarningOnce("HairComponentController", false, "Error getting the actor instance.");
                    return false;
                }

                const EMotionFX::TransformData* transformData = m_actorInstance->GetTransformData();
                if (!transformData)
                {
                    AZ_WarningOnce("HairComponentController", false, "Error getting the transformData from the actorInstance.");
                    return false;
                }

                // In EMotionFX the skinning matrices is stored as a 3x4. The conversion to 4x4 matrices happens at the update bone matrices function.
                // In here we use the boneIndexMap to find the correct EMotionFX bone index (also as the global bone index), and passing the
                // matrices of those bones to the hair render object.
                const AZ::Matrix3x4* matrices = transformData->GetSkinningMatrices();
                const size_t numBoneMatrices = m_cachedBoneMatrices.size(); // Resizing of this matrices happens at createHairObject().
                for (AZ::u32 tressFXBoneIndex = 0; tressFXBoneIndex < numBoneMatrices; ++tressFXBoneIndex)
                {
                    const AZ::u32 emfxBoneIndex = m_boneIndexMap[tressFXBoneIndex];
                    m_cachedBoneMatrices[tressFXBoneIndex] = matrices[emfxBoneIndex];
                }
                m_renderObject->UpdateBoneMatrices(m_cachedBoneMatrices);

                return true;
            }

            bool HairComponentController::CreateHairObject()
            {
                if (!m_actorInstance)
                {
                    // Do not create a hairRenderObject when actor instance hasn't been created.
                    return false;
                }

                if (!m_featureProcessor)
                {
                    AZ_Warning("CreateHairObject", false, "Required feature processor does not exist yet");
                    return false;
                }

                if (!m_configuration.m_hairAsset.GetId().IsValid() || !m_configuration.m_hairAsset.IsReady())
                {
                    AZ_Warning("CreateHairObject", false, "Hair Asset is not loaded yet");
                    return false;
                }

                AMD::TressFXAsset* hairAsset = m_configuration.m_hairAsset.Get()->m_tressFXAsset.get();
                if (!hairAsset)
                {
                    AZ_Warning("CreateHairObject", false, "Hair asset could not be loaded");
                    return false;
                }

                // Fix the bone index in the TressFX asset
                const size_t numBones = hairAsset->m_boneNames.size();
                AZ::u32 numMismatchedBone = 0;
                m_boneIndexMap.resize(numBones);
                for (AZ::u32 tressFXBoneIndex = 0; tressFXBoneIndex < numBones; ++tressFXBoneIndex)
                {
                    const char* boneName = hairAsset->m_boneNames[tressFXBoneIndex].c_str();
                    const EMotionFX::Node* emfxNode = m_actorInstance->GetActor()->GetSkeleton()->FindNodeByName(boneName);
                    if (!emfxNode)
                    {
                        // [rhhong-TODO] Add better error handling in the UI, to show the user they have selected an asset that doesn't match the actor.
                        // If we find any bone name that we do not have in the current actor, that means this is not a suitable actor for the tressFX asset.
                        // Do not create a hair object in this case because the skinning data won't be correct.
                        AZ_TracePrintf("CreateHairObject", "Cannot find bone name %s under emotionfx actor.", boneName);
                        numMismatchedBone++;
                        continue;
                    }
                    m_boneIndexMap[tressFXBoneIndex] = emfxNode->GetNodeIndex();
                }
                if (numMismatchedBone > 0)
                {
                    AZ_Error("CreateHairObject", false, "%zu bones cannot be found under the emotionfx actor. It is likely that the hair asset is incompatible with the actor asset.");
                    return false;
                }
                hairAsset->FixBoneIndices(m_boneIndexMap);

                //==================  FIX ME - FIX ME ==================
                // [To Do] Adi: for some reason when deleting and starting over, the buffers are not
                // being re-generated because they exist.  look into this but for now create once and try.
                if (m_renderObject)
                {
                    AZ_Warning("CreateHairObject", false, "Hair object was already created - FIX REPEATING LOADS");
                    // TEMPORARY - REMOVING TO AVOID CRASH ON RENDER - CHANGE BACK FOR DEBUG!
 //                   m_featureProcessor->RemoveHairRenderObject(m_renderObject);     
                    m_configuration.m_hairAsset.Get()->m_tressFXAsset.release();
                    return true;        // remove this line once you remove the object from the list.
                }
                //==================  FIX ME - FIX ME ==================

                InitHairDefaultParameters();
                
                // First remove the existing hair object - this can happen if the configuration
                // or the hair asset selected changes.
                if (m_renderObject)
                {   // this will remove the old instance in the feature processor
                    m_featureProcessor->RemoveHairRenderObject(m_renderObject);
                }

                // create a new instance - will remove the old one.
                m_renderObject = new HairRenderObject;
                if (!m_renderObject->Init(
                    m_featureProcessor, "test_hair_single_bone_sphere",
                    hairAsset, m_simDefaultSettings, m_renderDefaultSettings))
                {
                    AZ_Warning("CreateHairObject", false, "Hair object was not initialize succesfully");
                    m_configuration.m_hairAsset.Get()->m_tressFXAsset.release();
                    return false;
                }

                // Resize the bone matrices array. The size should equal to the number of bones in the tressFXAsset.
                m_cachedBoneMatrices.resize(numBones);

                // Connect the tick bus in order to update the bone matrices every tick.
                AZ::TickBus::Handler::BusConnect();

                // Feature processor registration that will hold an instance.
                m_featureProcessor->AddHairRenderObject(m_renderObject);

                m_configuration.m_hairAsset.Get()->m_tressFXAsset.release();
                return true;
            }
            // [To Do] Adi: add auto generated getter/setter functions...

        } // namespace Hair
    } // namespace Render
} // namespace AZ

#pragma optimize("", on)
