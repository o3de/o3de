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
#include <Integration/SimpleMotionComponentBus.h>

namespace EMotionFX
{
    namespace Integration
    {
        class SimpleMotionComponent
            : public AZ::Component
            , private AZ::Data::AssetBus::MultiHandler
            , private ActorComponentNotificationBus::Handler
            , private SimpleMotionComponentRequestBus::Handler
        {
        public:

            friend class EditorSimpleMotionComponent;

            AZ_COMPONENT(SimpleMotionComponent, "{DBE3C105-6FC1-418F-A8B1-D0F29FE8D5BD}");

            /**
            * Configuration struct for procedural configuration of SimpleMotionComponents.
            */
            struct Configuration
            {
                AZ_TYPE_INFO(Configuration, "{DA661C5F-E79E-41C3-B055-5F5A4E353F84}")
                Configuration();

                AZ::Data::Asset<MotionAsset>         m_motionAsset;             ///< Assigned motion asset
                bool                                 m_loop;                    ///< Toggles looping of the motion
                bool                                 m_retarget;                ///< Toggles retargeting of the motion
                bool                                 m_reverse;                 ///< Toggles reversing of the motion
                bool                                 m_mirror;                  ///< Toggles mirroring of the motion
                float                                m_playspeed;               ///< Determines the rate at which the motion is played
                float                                m_blendInTime;             ///< Determines the blend in time in seconds.
                float                                m_blendOutTime;            ///< Determines the blend out time in seconds.
                bool                                 m_playOnActivation;        ///< Determines if the motion should be played immediately
                bool                                 m_inPlace;                 ///< Determines if the motion should be played in-place.
                bool                                 m_freezeAtLastFrame = true;///< Determines if the motion will go to bind pose after finishing or freeze at the last frame.

                static void Reflect(AZ::ReflectContext* context);

                AZ::Crc32 GetBlendOutTimeVisibility() const;
                AZ::Crc32 GetFreezeAtLastFrameVisibility() const;
            };

            SimpleMotionComponent(const Configuration* config = nullptr);
            ~SimpleMotionComponent();

            // AZ::Component interface implementation
            void Init() override;
            void Activate() override;
            void Deactivate() override;

            static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
            {
                provided.push_back(AZ_CRC_CE("EMotionFXSimpleMotionService"));
            }
            static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent)
            {
                dependent.push_back(AZ_CRC_CE("MeshService"));
            }
            static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
            {
                required.push_back(AZ_CRC_CE("EMotionFXActorService"));
            }
            static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
            {
                incompatible.push_back(AZ_CRC_CE("EMotionFXAnimGraphService"));
                incompatible.push_back(AZ_CRC_CE("EMotionFXSimpleMotionService"));
                incompatible.push_back(AZ_CRC_CE("NonUniformScaleService"));
            }
            static void Reflect(AZ::ReflectContext* /*context*/);

            // SimpleMotionComponentRequestBus::Handler
            void LoopMotion(bool enable) override;
            bool GetLoopMotion() const override;
            void RetargetMotion(bool enable) override;
            void ReverseMotion(bool enable) override;
            void MirrorMotion(bool enable) override;
            void SetPlaySpeed(float speed) override;
            float GetPlaySpeed() const override;
            void PlayTime(float time) override;
            float GetPlayTime() const override;
            float GetDuration() const override;
            void Motion(AZ::Data::AssetId assetId) override;
            AZ::Data::AssetId  GetMotion() const override;
            void BlendInTime(float time) override;
            float GetBlendInTime() const override;
            void BlendOutTime(float time) override;
            float GetBlendOutTime() const override;
            void PlayMotion() override;

            const EMotionFX::MotionInstance* GetMotionInstance();

            // AZ::Data::AssetBus::Handler
            void SetMotionAssetId(const AZ::Data::AssetId& assetId);
            void OnAssetReady(AZ::Data::Asset<AZ::Data::AssetData> asset) override;
            void OnAssetReloaded(AZ::Data::Asset<AZ::Data::AssetData> asset) override;

        private:
            // ActorComponentNotificationBus::Handler
            void OnActorInstanceCreated(EMotionFX::ActorInstance* actorInstance) override;
            void OnActorInstanceDestroyed(EMotionFX::ActorInstance* actorInstance) override;

            void RemoveMotionInstanceFromActor(EMotionFX::MotionInstance* motionInstance);

            static EMotionFX::MotionInstance* PlayMotionInternal(const EMotionFX::ActorInstance* actorInstance, const SimpleMotionComponent::Configuration& cfg, bool deleteOnZeroWeight);

            Configuration                               m_configuration;        ///< Component configuration.
            EMotionFXPtr<EMotionFX::ActorInstance>      m_actorInstance;        ///< Associated actor instance (retrieved from Actor Component).
            EMotionFX::MotionInstance*                  m_motionInstance;       ///< Motion to play on the actor
            AZ::Data::Asset<MotionAsset>                m_lastMotionAsset;      ///< Last active motion asset, kept alive for blending.
            EMotionFX::MotionInstance*                  m_lastMotionInstance;   ///< Last active motion instance, kept alive for blending.
        };
    } // namespace Integration
} // namespace EMotionFX
