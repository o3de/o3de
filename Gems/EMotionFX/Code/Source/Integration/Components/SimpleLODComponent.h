/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/Component.h>
#include <AzCore/Component/TransformBus.h>
#include <AzCore/Component/TickBus.h>
#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/Script/ScriptProperty.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>

#include <Integration/Assets/MotionAsset.h>
#include <Integration/ActorComponentBus.h>
#include <AtomLyIntegration/CommonFeatures/Mesh/MeshComponentBus.h>

namespace EMotionFX
{
    namespace Integration
    {
        class SimpleLODComponent
            : public AZ::Component
            , private AZ::TickBus::Handler
            , private ActorComponentNotificationBus::Handler
        {
        public:

            friend class EditorSimpleLODComponent;

            AZ_COMPONENT(SimpleLODComponent, "{9380B039-EB03-4920-9F06-D90481E739E6}");

            /**
            * Configuration struct for procedural configuration of SimpleLODComponents.
            */
            struct Configuration
            {
                AZ_TYPE_INFO(Configuration, "{262470E5-57D8-4C45-8BB4-88EDFBC54D7E}");
                Configuration() = default;
                void Reset();

                // Generate the default value based on LOD level.
                void GenerateDefaultValue(size_t numLODs);
                bool GetEnableLodSampling();

                static void Reflect(AZ::ReflectContext* context);

                AZStd::vector<float> m_lodDistances;         // LOD distances that decide which lod the actor should choose.
                AZStd::vector<float> m_lodSampleRates;       // Per LOD sample rate.
                bool m_enableLodSampling = false;            // Enable per LOD sampling rate. This will allow animation to sample at a lower rate for performance improvement.
            };

            SimpleLODComponent(const Configuration* config = nullptr);
            ~SimpleLODComponent();

            // AZ::Component interface implementation
            void Init() override;
            void Activate() override;
            void Deactivate() override;

            static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
            {
                provided.push_back(AZ_CRC_CE("EMotionFXSimpleLODService"));
            }
            static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent)
            {
                AZ_UNUSED(dependent);
            }
            static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
            {
                required.push_back(AZ_CRC_CE("EMotionFXActorService"));
            }
            static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
            {
                incompatible.push_back(AZ_CRC_CE("EMotionFXSimpleLODService"));
            }
            static void Reflect(AZ::ReflectContext* context);

        private:
            // ActorComponentNotificationBus::Handler
            void OnActorInstanceCreated(EMotionFX::ActorInstance* actorInstance) override;
            void OnActorInstanceDestroyed(EMotionFX::ActorInstance* actorInstance) override;

            // AZ::TickBus::Handler
            void OnTick(float deltaTime, AZ::ScriptTimePoint time) override;

            static size_t GetLodByDistance(const AZStd::vector<float>& distances, float distance);
            static void UpdateLodLevelByDistance(EMotionFX::ActorInstance* actorInstance, const Configuration& configuration, AZ::EntityId entityId);

            Configuration                               m_configuration;        // Component configuration.
            EMotionFX::ActorInstance*                   m_actorInstance;        // Associated actor instance (retrieved from Actor Component).

            AZ::RPI::Cullable::LodType m_previousLodType = AZ::RPI::Cullable::LodType::Default;
            size_t m_previousLodLevel = 0;
        };

    } // namespace Integration
} // namespace EMotionFX

