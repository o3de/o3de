/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Asset/AssetSerializer.h>
#include <AzCore/Component/Entity.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Script/ScriptContext.h>

#include <Integration/Components/SimpleMotionComponent.h>
#include <MCore/Source/AttributeString.h>
#include <EMotionFX/Source/ActorInstance.h>
#include <EMotionFX/Source/MotionSystem.h>
#include <EMotionFX/Source/MotionInstance.h>

namespace EMotionFX
{
    namespace Integration
    {

        void SimpleMotionComponent::Configuration::Reflect(AZ::ReflectContext *context)
        {
            auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
            if (serializeContext)
            {
                serializeContext->Class<Configuration>()
                    ->Version(2)
                    ->Field("MotionAsset", &Configuration::m_motionAsset)
                    ->Field("Loop", &Configuration::m_loop)
                    ->Field("Retarget", &Configuration::m_retarget)
                    ->Field("Reverse", &Configuration::m_reverse)
                    ->Field("Mirror", &Configuration::m_mirror)
                    ->Field("PlaySpeed", &Configuration::m_playspeed)
                    ->Field("BlendIn", &Configuration::m_blendInTime)
                    ->Field("BlendOut", &Configuration::m_blendOutTime)
                    ->Field("PlayOnActivation", &Configuration::m_playOnActivation)
                    ->Field("InPlace", &Configuration::m_inPlace)
                    ;

                AZ::EditContext* editContext = serializeContext->GetEditContext();
                if (editContext)
                {
                    editContext->Class<Configuration>( "Configuration", "Settings for this Simple Motion")
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->DataElement(AZ::Edit::UIHandlers::Default, &Configuration::m_motionAsset, "Motion", "EMotion FX motion to be loaded for this actor")
                        ->DataElement(AZ::Edit::UIHandlers::Default, &Configuration::m_loop, "Loop motion", "Toggles looping of the animation")
                        ->DataElement(AZ::Edit::UIHandlers::Default, &Configuration::m_retarget, "Retarget motion", "Toggles retargeting of the animation")
                        ->DataElement(AZ::Edit::UIHandlers::Default, &Configuration::m_reverse, "Reverse motion", "Toggles reversing of the animation")
                        ->DataElement(AZ::Edit::UIHandlers::Default, &Configuration::m_mirror, "Mirror motion", "Toggles mirroring of the animation")
                        ->DataElement(AZ::Edit::UIHandlers::Default, &Configuration::m_playspeed, "Play speed", "Determines the rate at which the motion is played")
                            ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                        ->DataElement(AZ::Edit::UIHandlers::Default, &Configuration::m_blendInTime, "Blend In Time", "Determines the blend in time in seconds")
                            ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                        ->DataElement(AZ::Edit::UIHandlers::Default, &Configuration::m_blendOutTime, "Blend Out Time", "Determines the blend out time in seconds")
                            ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                        ->DataElement(AZ::Edit::UIHandlers::Default, &Configuration::m_playOnActivation, "Play on active", "Playing animation immediately after activation.")
                        ->DataElement(AZ::Edit::UIHandlers::Default, &Configuration::m_inPlace, "In-place",
                            "Plays the animation in-place and removes any positional and rotational changes from root joints.")
                        ;
                }
            }
        }

        void SimpleMotionComponent::Reflect(AZ::ReflectContext* context)
        {
            Configuration::Reflect(context);

            auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
            if (serializeContext)
            {
                serializeContext->Class<SimpleMotionComponent, AZ::Component>()
                    ->Version(1)
                    ->Field("Configuration", &SimpleMotionComponent::m_configuration)
                    ;
            }

            auto* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context);
            if (behaviorContext)
            {
                behaviorContext->EBus<SimpleMotionComponentRequestBus>("SimpleMotionComponentRequestBus")
                    ->Event("LoopMotion", &SimpleMotionComponentRequestBus::Events::LoopMotion)
                    ->Event("GetLoopMotion", &SimpleMotionComponentRequestBus::Events::GetLoopMotion)
                        ->Attribute("Hidden", AZ::Edit::Attributes::PropertyHidden)
                    ->VirtualProperty("LoopMotion", "GetLoopMotion", "LoopMotion")
                    ->Event("RetargetMotion", &SimpleMotionComponentRequestBus::Events::RetargetMotion)
                    ->Event("ReverseMotion", &SimpleMotionComponentRequestBus::Events::ReverseMotion)
                    ->Event("MirrorMotion", &SimpleMotionComponentRequestBus::Events::MirrorMotion)
                    ->Event("SetPlaySpeed", &SimpleMotionComponentRequestBus::Events::SetPlaySpeed)
                    ->Event("GetPlaySpeed", &SimpleMotionComponentRequestBus::Events::GetPlaySpeed)
                        ->Attribute("Hidden", AZ::Edit::Attributes::PropertyHidden)
                    ->VirtualProperty("PlaySpeed", "GetPlaySpeed", "SetPlaySpeed")
                    ->Event("PlayTime", &SimpleMotionComponentRequestBus::Events::PlayTime)
                    ->Event("GetPlayTime", &SimpleMotionComponentRequestBus::Events::GetPlayTime)
                        ->Attribute("Hidden", AZ::Edit::Attributes::PropertyHidden)
                    ->VirtualProperty("PlayTime", "GetPlayTime", "PlayTime")
                    ->Event("Motion", &SimpleMotionComponentRequestBus::Events::Motion)
                    ->Event("GetMotion", &SimpleMotionComponentRequestBus::Events::GetMotion)
                    ->VirtualProperty("Motion", "GetMotion", "Motion")
                    ->Event("BlendInTime", &SimpleMotionComponentRequestBus::Events::BlendInTime)
                    ->Event("GetBlendInTime", &SimpleMotionComponentRequestBus::Events::GetBlendInTime)
                        ->Attribute("Hidden", AZ::Edit::Attributes::PropertyHidden)
                    ->VirtualProperty("BlendInTime", "GetBlendInTime", "BlendInTime")
                    ->Event("BlendOutTime", &SimpleMotionComponentRequestBus::Events::BlendOutTime)
                    ->Event("GetBlendOutTime", &SimpleMotionComponentRequestBus::Events::GetBlendOutTime)
                        ->Attribute("Hidden", AZ::Edit::Attributes::PropertyHidden)
                    ->VirtualProperty("BlendOutTime", "GetBlendOutTime", "BlendOutTime")
                    ->Event("PlayMotion", &SimpleMotionComponentRequestBus::Events::PlayMotion)
                    ;

                behaviorContext->Class<SimpleMotionComponent>()->RequestBus("SimpleMotionComponentRequestBus");
            }
        }

        SimpleMotionComponent::Configuration::Configuration()
            : m_loop(false)
            , m_retarget(false)
            , m_reverse(false)
            , m_mirror(false)
            , m_playspeed(1.f)
            , m_blendInTime(0.0f)
            , m_blendOutTime(0.0f)
            , m_playOnActivation(true)
            , m_inPlace(false)
        {
        }

        SimpleMotionComponent::SimpleMotionComponent(const Configuration* config)
            : m_actorInstance(nullptr)
            , m_motionInstance(nullptr)
            , m_lastMotionInstance(nullptr)
        {
            if (config)
            {
                m_configuration = *config;
            }
        }

        SimpleMotionComponent::~SimpleMotionComponent()
        {
        }

        void SimpleMotionComponent::Init()
        {
        }

        void SimpleMotionComponent::Activate()
        {
            m_motionInstance = nullptr;

            AZ::Data::AssetBus::MultiHandler::BusDisconnect();

            SimpleMotionComponentRequestBus::Handler::BusConnect(GetEntityId());

            auto& cfg = m_configuration;

            if (cfg.m_motionAsset.GetId().IsValid())
            {
                AZ::Data::AssetBus::MultiHandler::BusConnect(cfg.m_motionAsset.GetId());
                cfg.m_motionAsset.QueueLoad();
            }

            ActorComponentNotificationBus::Handler::BusConnect(GetEntityId());
        }

        void SimpleMotionComponent::Deactivate()
        {
            SimpleMotionComponentRequestBus::Handler::BusDisconnect();
            ActorComponentNotificationBus::Handler::BusDisconnect();
            AZ::Data::AssetBus::MultiHandler::BusDisconnect();

            RemoveMotionInstanceFromActor(m_motionInstance);
            m_motionInstance = nullptr;
            RemoveMotionInstanceFromActor(m_lastMotionInstance);
            m_lastMotionInstance = nullptr;
            m_configuration.m_motionAsset.Release();
            m_lastMotionAsset.Release();
            m_actorInstance.reset();
        }

        const MotionInstance* SimpleMotionComponent::GetMotionInstance()
        {
            return m_motionInstance;
        }

        void SimpleMotionComponent::SetMotionAssetId(const AZ::Data::AssetId& assetId)
        {
            m_configuration.m_motionAsset = AZ::Data::Asset<MotionAsset>(assetId, azrtti_typeid<MotionAsset>());
        }

        void SimpleMotionComponent::OnAssetReady(AZ::Data::Asset<AZ::Data::AssetData> asset)
        {
            auto& cfg = m_configuration;

            if (asset.GetId() == cfg.m_motionAsset.GetId())
            {
                cfg.m_motionAsset = asset;
                if (m_configuration.m_playOnActivation)
                {
                    PlayMotion();
                }
            }
        }

        void SimpleMotionComponent::OnAssetReloaded(AZ::Data::Asset<AZ::Data::AssetData> asset)
        {
            OnAssetReady(asset);
        }

        void SimpleMotionComponent::OnActorInstanceCreated(EMotionFX::ActorInstance* actorInstance)
        {
            m_actorInstance = actorInstance;
            if (m_configuration.m_playOnActivation)
            {
                PlayMotion();
            }
        }

        void SimpleMotionComponent::OnActorInstanceDestroyed(EMotionFX::ActorInstance* actorInstance)
        {
            AZ_UNUSED(actorInstance);
            RemoveMotionInstanceFromActor(m_motionInstance);
            m_motionInstance = nullptr;
            RemoveMotionInstanceFromActor(m_lastMotionInstance);
            m_lastMotionInstance = nullptr;

            m_actorInstance.reset();
        }

        void SimpleMotionComponent::PlayMotion()
        {
            m_motionInstance = PlayMotionInternal(m_actorInstance.get(), m_configuration, /*deleteOnZeroWeight*/true);
        }

        void SimpleMotionComponent::RemoveMotionInstanceFromActor(EMotionFX::MotionInstance* motionInstance)
        {
            if (motionInstance)
            {
                if (m_actorInstance && m_actorInstance->GetMotionSystem())
                {
                    m_actorInstance->GetMotionSystem()->RemoveMotionInstance(motionInstance);
                }
            }
        }

        void SimpleMotionComponent::LoopMotion(bool enable)
        {
            m_configuration.m_loop = enable;
            if (m_motionInstance)
            {
                m_motionInstance->SetMaxLoops(enable ? EMFX_LOOPFOREVER : 1);
            }
        }

        bool SimpleMotionComponent::GetLoopMotion() const
        {
            return m_configuration.m_loop;
        }

        void SimpleMotionComponent::RetargetMotion(bool enable)
        {
            m_configuration.m_retarget = enable;
            if (m_motionInstance)
            {
                m_motionInstance->SetRetargetingEnabled(enable);
            }
        }

        void SimpleMotionComponent::ReverseMotion(bool enable)
        {
            m_configuration.m_reverse = enable;
            if (m_motionInstance)
            {
                m_motionInstance->SetPlayMode(enable ? EMotionFX::EPlayMode::PLAYMODE_BACKWARD : EMotionFX::EPlayMode::PLAYMODE_FORWARD);
            }
        }

        void SimpleMotionComponent::MirrorMotion(bool enable)
        {
            m_configuration.m_mirror = enable;
            if (m_motionInstance)
            {
                m_motionInstance->SetMirrorMotion(enable);
            }
        }

        void SimpleMotionComponent::SetPlaySpeed(float speed)
        {
            m_configuration.m_playspeed = speed;
            if (m_motionInstance)
            {
                m_motionInstance->SetPlaySpeed(speed);
            }
        }

        float SimpleMotionComponent::GetPlaySpeed() const
        {
            return m_configuration.m_playspeed;
        }

        void SimpleMotionComponent::PlayTime(float time)
        {
            if (m_motionInstance)
            {
                float delta = time - m_motionInstance->GetLastCurrentTime();
                m_motionInstance->SetCurrentTime(time, false);

                // Apply the same time step to the last animation
                // so blend out will be good. Otherwise we are just blending
                // from the last frame played of the last animation.
                if (m_lastMotionInstance && m_lastMotionInstance->GetIsBlending())
                {
                    m_lastMotionInstance->SetCurrentTime(m_lastMotionInstance->GetLastCurrentTime() + delta, false);
                }
            }
        }

        float SimpleMotionComponent::GetPlayTime() const
        {
            float result = 0.0f;
            if (m_motionInstance)
            {
                result = m_motionInstance->GetCurrentTimeNormalized();
            }
            return result;
        }

        void SimpleMotionComponent::Motion(AZ::Data::AssetId assetId)
        {
            if (m_configuration.m_motionAsset.GetId() != assetId)
            {
                // Disconnect the old asset bus
                if (AZ::Data::AssetBus::MultiHandler::BusIsConnectedId(m_configuration.m_motionAsset.GetId()))
                {
                    AZ::Data::AssetBus::MultiHandler::BusDisconnect(m_configuration.m_motionAsset.GetId());
                }

                // Save the motion asset that we are about to be remove in case it can be reused.
                AZ::Data::Asset<MotionAsset> oldLastMotionAsset = m_lastMotionAsset;

                if (m_lastMotionInstance)
                {
                    RemoveMotionInstanceFromActor(m_lastMotionInstance);
                }

                // Store the current motion asset as the last one for possible blending.
                // If we don't keep a reference to the motion asset, the motion instance will be
                // automatically released.
                if (m_configuration.m_motionAsset.GetId().IsValid())
                {
                    m_lastMotionAsset = m_configuration.m_motionAsset;
                }

                // Set the current motion instance as the last motion instance. The new current motion
                // instance will then be set when the load is complete.
                m_lastMotionInstance = m_motionInstance;
                m_motionInstance = nullptr;

                // Start the fade out if there is a blend out time. Otherwise just leave the
                // m_lastMotionInstance where it is at so the next anim can blend from that frame.
                if (m_lastMotionInstance && m_configuration.m_blendOutTime > 0.0f)
                {
                    m_lastMotionInstance->Stop(m_configuration.m_blendOutTime);
                }

                // Reuse the old, last motion asset if possible. Otherwise, request a load.
                if (assetId.IsValid() && oldLastMotionAsset.GetData() && assetId == oldLastMotionAsset.GetId())
                {
                    // Even though we are not calling GetAsset here, OnAssetReady
                    // will be fired when the bus is connected because this asset is already loaded.
                    m_configuration.m_motionAsset = oldLastMotionAsset;
                }
                else
                {
                    // Won't be able to reuse oldLastMotionAsset, release it.
                    oldLastMotionAsset.Release();

                    // Clear the old asset.
                    m_configuration.m_motionAsset.Release();

                    // Create a new asset
                    if (assetId.IsValid())
                    {
                        m_configuration.m_motionAsset = AZ::Data::AssetManager::Instance().GetAsset<MotionAsset>(assetId, m_configuration.m_motionAsset.GetAutoLoadBehavior());
                    }
                }

                // Connect the bus if the asset is is valid.
                if (m_configuration.m_motionAsset.GetId().IsValid())
                {
                    AZ::Data::AssetBus::MultiHandler::BusConnect(m_configuration.m_motionAsset.GetId());
                }

            }
        }

        AZ::Data::AssetId SimpleMotionComponent::GetMotion() const
        {
            return m_configuration.m_motionAsset.GetId();
        }

        void SimpleMotionComponent::BlendInTime(float time)
        {
            m_configuration.m_blendInTime = time;
        }

        float SimpleMotionComponent::GetBlendInTime() const
        {
            return m_configuration.m_blendInTime;
        }

        void SimpleMotionComponent::BlendOutTime(float time)
        {
            m_configuration.m_blendOutTime = time;
        }

        float SimpleMotionComponent::GetBlendOutTime() const
        {
            return m_configuration.m_blendOutTime;
        }

        EMotionFX::MotionInstance* SimpleMotionComponent::PlayMotionInternal(const EMotionFX::ActorInstance* actorInstance, const SimpleMotionComponent::Configuration& cfg, bool deleteOnZeroWeight)
        {
            if (!actorInstance || !cfg.m_motionAsset.IsReady())
            {
                return nullptr;
            }

            if (!actorInstance->GetMotionSystem())
            {
                return nullptr;
            }

            auto* motionAsset = cfg.m_motionAsset.GetAs<MotionAsset>();
            if (!motionAsset)

            {
                AZ_Error("EMotionFX", motionAsset, "Motion asset is not valid.");
                return nullptr;
            }
            //init the PlaybackInfo based on our config
            EMotionFX::PlayBackInfo info;
            info.m_numLoops = cfg.m_loop ? EMFX_LOOPFOREVER : 1;
            info.m_retarget = cfg.m_retarget;
            info.m_playMode = cfg.m_reverse ? EMotionFX::EPlayMode::PLAYMODE_BACKWARD : EMotionFX::EPlayMode::PLAYMODE_FORWARD;
            info.m_freezeAtLastFrame = info.m_numLoops == 1;
            info.m_mirrorMotion = cfg.m_mirror;
            info.m_playSpeed = cfg.m_playspeed;
            info.m_playNow = true;
            info.m_deleteOnZeroWeight = deleteOnZeroWeight;
            info.m_canOverwrite = false;
            info.m_blendInTime = cfg.m_blendInTime;
            info.m_blendOutTime = cfg.m_blendOutTime;
            info.m_inPlace = cfg.m_inPlace;
            return actorInstance->GetMotionSystem()->PlayMotion(motionAsset->m_emfxMotion.get(), &info);
        }

    } // namespace integration
} // namespace EMotionFX

