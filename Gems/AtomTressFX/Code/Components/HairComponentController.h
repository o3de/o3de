/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Math/Vector2.h>
#include <AzCore/Math/Vector3.h>

#include <AzCore/Component/Component.h>
#include <AzCore/Component/TickBus.h>

// Hair specific
#include <Components/HairBus.h>
#include <Rendering/HairGlobalSettingsBus.h>
#include <Components/HairComponentConfig.h>
#include <Rendering/HairRenderObject.h>

// EMotionFX
#include <Integration/ActorComponentBus.h>

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

            //! This is the controller class for both EditorComponent and in game Component.
            //! It is responsible for the creation and activation of the hair object itself
            //! and the update and synchronization of any changed configuration.
            //! It also responsible to the connection with the entity's Actor to whom the hair
            //! is associated and gets the skinning matrices and visibility. 
            class HairComponentController final
                : public HairRequestsBus::Handler
                , public HairGlobalSettingsNotificationBus::Handler
                , private AZ::Data::AssetBus::MultiHandler
                , private AZ::TickBus::Handler
                , private EMotionFX::Integration::ActorComponentNotificationBus::Handler
            {
            public:
                friend class EditorHairComponent;

                AZ_TYPE_INFO(HairComponentController, "{81D3EA93-7EAC-44B7-B8CB-0B573DD8D634}");
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

            private:
                AZ_DISABLE_COPY(HairComponentController);

                void OnHairConfigChanged();
                void OnHairAssetChanged();

                // AZ::Render::Hair::HairGlobalSettingsNotificationBus Overrides
                void OnHairGlobalSettingsChanged(const HairGlobalSettings& hairGlobalSettings) override;

                // AZ::Data::AssetBus::Handler
                void OnAssetReady(AZ::Data::Asset<AZ::Data::AssetData> asset) override;
                void OnAssetReloaded(AZ::Data::Asset<AZ::Data::AssetData> asset) override;

                // EMotionFX::Integration::ActorComponentNotificationBus::Handler
                void OnActorInstanceCreated(EMotionFX::ActorInstance* actorInstance) override; 
                void OnActorInstanceDestroyed(EMotionFX::ActorInstance* actorInstance) override;

                // AZ::TickBus::Handler
                void OnTick(float deltaTime, AZ::ScriptTimePoint time) override;
                int GetTickOrder() override;

                bool GenerateLocalToGlobalBoneIndex( EMotionFX::ActorInstance* actorInstance, AMD::TressFXAsset* hairAsset);

                bool CreateHairObject();
                void RemoveHairObject();

                // Extract actor matrix from the actor instance.
                bool UpdateActorMatrices();

                HairFeatureProcessor* m_featureProcessor = nullptr;

                bool m_configChanged = false; // Flag used to defer the configuration change to onTick.
                HairComponentConfig m_configuration;     // Settings per hair component

                //! Hair render object for connecting to the skeleton and connecting to the feature processor.
                Data::Instance<HairRenderObject> m_renderObject;     // unique to this component - this is the data source.

                EntityId m_entityId;

                // Store a cache of the bone index lookup we generated during the creation of hair object.
                AMD::LocalToGlobalBoneIndexLookup m_hairBoneIndexLookup;
                AMD::LocalToGlobalBoneIndexLookup m_collisionBoneIndexLookup;

                // Cache the bone matrices array to avoid frequent allocation.
                AZStd::vector<AZ::Matrix3x4> m_cachedHairBoneMatrices;
                AZStd::vector<AZ::Matrix3x4> m_cachedCollisionBoneMatrices;

                AZ::Matrix3x4 m_entityWorldMatrix;
            };
        } // namespace Hair
    }
}
