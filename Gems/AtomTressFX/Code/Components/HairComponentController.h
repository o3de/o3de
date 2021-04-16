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

#pragma once

#include <AzCore/Math/Vector2.h>
#include <AzCore/Math/Vector3.h>

//#include <AtomCore/Instance/Instance.h>

#include <AzCore/Component/Component.h>
#include <AzCore/Component/TransformBus.h>
#include <AzCore/Component/TickBus.h>

// Hair specific
#include <Components/HairBus.h>
#include <Components/HairComponentConfig.h>

// EMotionFX
#include <Integration/ActorComponentBus.h>
 
//#include <Rendering/HairRenderObject.h>
//#include <Components/HairSettingsInterface.h>

namespace AMD
{
    class TressFXSimulationSettings;
    class TressFXRenderingSettings;
    class TressFXAsset;
}

namespace AZ
{
    namespace Render
    {
        namespace Hair
        {
            class HairFeatureProcessor;
            class HairRenderObject;

            class HairComponentController final
                : public HairRequestsBus::Handler
                , private AZ::Data::AssetBus::MultiHandler
                , private AZ::TickBus::Handler
                , private EMotionFX::Integration::ActorComponentNotificationBus::Handler
            {
            public:
                friend class EditorHairComponent;

                AZ_TYPE_INFO(AZ::Render::HairComponentController, "{81D3EA93-7EAC-44B7-B8CB-0B573DD8D634}");
                static void Reflect(AZ::ReflectContext* context);
                static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
                static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
                static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);

                HairComponentController() = default;
                HairComponentController(const HairComponentConfig& config);
                ~HairComponentController();

                void Activate(EntityId entityId);
                void Deactivate();
                void SetConfiguration(const HairComponentConfig& config);
                const HairComponentConfig& GetConfiguration() const;

                HairFeatureProcessor* GetFeatureProcessor() { return m_featureProcessor; }

                void SetEntity(Entity* entity) { m_entity = entity;  }
                void InitHairDefaultParameters();
                bool UpdateActorMatrices();
                bool CreateBonesIndicesLUT(
                    std::vector<std::string>& bonesNames, std::vector<int32_t>& bonesIndicesLUT);

                bool CreateHairObject();

            private:
                AZ_DISABLE_COPY(HairComponentController);

                void OnConfigChanged();

                // AZ::Data::AssetBus::Handler
                void OnAssetReady(AZ::Data::Asset<AZ::Data::AssetData> asset) override;
                void OnAssetReloaded(AZ::Data::Asset<AZ::Data::AssetData> asset) override;

                // EMotionFX::Integration::ActorComponentNotificationBus::Handler
                void OnActorInstanceCreated(EMotionFX::ActorInstance* actorInstance) override; 
                void OnActorInstanceDestroyed(EMotionFX::ActorInstance* actorInstance) override;

                // AZ::TickBus::Handler
                void OnTick(float deltaTime, AZ::ScriptTimePoint time) override;
                int GetTickOrder() override;

                HairFeatureProcessor* m_featureProcessor = nullptr;
                // Possibly connect here to the object's settings or specifically create object

                //! Hair render object for connecting to the skeleton and connecting to the feature processor.
//                Data::Instance<HairRenderObject> m_renderObject = nullptr;  // consider moving to the component

                HairComponentConfig m_configuration;     // Settings per hair component

                //! Hair render object for connecting to the skeleton and connecting to the feature processor.
                HairRenderObject* m_renderObject = nullptr;     // unique to this component - this is the data source.

                //!Temporary default settings for testing & initial pass
                AMD::TressFXSimulationSettings* m_simDefaultSettings = nullptr;
                AMD::TressFXRenderingSettings* m_renderDefaultSettings = nullptr;

                Entity* m_entity = nullptr;
                EntityId m_entityId;
                bool m_initilized = false;
                EMotionFX::ActorInstance* m_actorInstance = nullptr;

                // Store a cache of the boneIndexMap we generated during the creation of hair object. The index of this map is the local bone index
                // from the tressFX asset, and the value is the bone index from the emotionfx actor.
                AMD::TressFXAsset::BoneIndexMap m_boneIndexMap;

                // Cache the bone matrices array to avoid frequent allocation.
                AZStd::vector<AZ::Matrix3x4> m_cachedBoneMatrices;
            };
        } // namespace Hair
    }
}
