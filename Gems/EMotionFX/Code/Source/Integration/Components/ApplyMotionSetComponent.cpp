/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#if defined(CARBONATED)
#include <AzCore/Component/Entity.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/RTTI/BehaviorContext.h>

#include <MCore/Source/AttributeString.h>
#include <EMotionFX/Source/AnimGraph.h>

#include <Integration/Components/ApplyMotionSetComponent.h>
#include <Integration/AnimGraphComponentBus.h>
#include <EMotionFX/Source/AnimGraphInstance.h>
#include <AzCore/Asset/AssetSerializer.h>

namespace EMotionFX
{
    namespace Integration
    {
        //////////////////////////////////////////////////////////////////////////
        void ApplyMotionSetComponent::Configuration::Reflect(AZ::ReflectContext* context)
        {
            auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
            if (serializeContext)
            {
                serializeContext->Class<Configuration>()
#if defined(CARBONATED)
                    ->Version(2)
                    ->Field("MotionSetAssetMap", &Configuration::m_motionSetAssetMap)
#else
                    ->Version(1)
                    ->Field("MotionSetAsset", &Configuration::m_motionSetAsset)
                    ->Field("ActiveMotionSetName", &Configuration::m_activeMotionSetName)
#endif
                ;
            }
        }

#if defined(CARBONATED)
        void ApplyMotionSetComponent::Apply(const AZ::EntityId & id, const MotionSetGender& preferredGender)
#else
        void ApplyMotionSetComponent::Apply(const AZ::EntityId & id)
#endif
        {
            auto& cfg = m_configuration;

#if defined(CARBONATED)
            AZ_Assert(!cfg.m_motionSetAssetMap.empty(), "Apply Motion Set Component does not contains any motion sets!");

            // Try to find preferredGender, if not present, fallback on gender neutral
            MotionSetGender fetchedGender = MotionSetGender::MOTION_NEUTRAL;
            
            if (cfg.m_motionSetAssetMap.find(preferredGender) != cfg.m_motionSetAssetMap.end())
            {
                fetchedGender = preferredGender;
            }

            AZ_Assert(cfg.m_motionSetAssetMap.find(fetchedGender) != cfg.m_motionSetAssetMap.end(), "Failed to find motion set for gender: %d", (int)fetchedGender);
            m_motionSetAsset = cfg.m_motionSetAssetMap[fetchedGender];

            AZ_Assert(m_motionSetAsset.GetId().IsValid(), "Motion Set Asset for gender (%d) is invalid!", (int)fetchedGender);
#endif

#if defined(CARBONATED)
            if (m_motionSetAsset.GetId().IsValid())
#else
            if (cfg.m_motionSetAsset.GetId().IsValid())
#endif
            {
#if defined(CARBONATED)
                AnimGraphInstance* pInstance = nullptr;
#else
                AnimGraphInstance* pInstance;
#endif
                AnimGraphComponentRequestBus::EventResult(pInstance, id, &AnimGraphComponentRequestBus::Events::GetAnimGraphInstance);

                if (pInstance)
                {
#if defined(CARBONATED)
                    MotionSet* motionSet = m_motionSetAsset.Get()->m_emfxMotionSet.get();
#else
                    auto& cfg = m_configuration;
                    MotionSet* motionSet = cfg.m_motionSetAsset.Get()->m_emfxMotionSet.get();
#endif

                    pInstance->SetMotionSet(motionSet);
                }
            }
        }

        //////////////////////////////////////////////////////////////////////////
        void ApplyMotionSetComponent::Reflect(AZ::ReflectContext* context)
        {
            Configuration::Reflect(context);

            auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
            if (serializeContext)
            {
                serializeContext->Class<ApplyMotionSetComponent, AZ::Component>()
                    ->Version(1)
                    ->Field("Configuration", &ApplyMotionSetComponent::m_configuration)
                ;
            }
        }

        //////////////////////////////////////////////////////////////////////////
        ApplyMotionSetComponent::Configuration::Configuration()
        {
        }

        //////////////////////////////////////////////////////////////////////////
        ApplyMotionSetComponent::ApplyMotionSetComponent(const Configuration* config)
        {
            if (config)
            {
                m_configuration = *config;
            }
        }

        //////////////////////////////////////////////////////////////////////////
        ApplyMotionSetComponent::~ApplyMotionSetComponent()
        {
        }

        //////////////////////////////////////////////////////////////////////////
        void ApplyMotionSetComponent::Init()
        {
        }

        //////////////////////////////////////////////////////////////////////////
        void ApplyMotionSetComponent::Activate()
        {
            AZ::Data::AssetBus::MultiHandler::BusDisconnect();

#if defined(CARBONATED)
            if (m_motionSetAsset.GetId().IsValid())
            {
                AZ::Data::AssetBus::MultiHandler::BusConnect(m_motionSetAsset.GetId());
                m_motionSetAsset.QueueLoad();
            }
#else
            auto& cfg = m_configuration;
            if (cfg.m_motionSetAsset.GetId().IsValid())
            {
                AZ::Data::AssetBus::MultiHandler::BusConnect(cfg.m_motionSetAsset.GetId());
                cfg.m_motionSetAsset.QueueLoad();
            }
#endif

            ApplyMotionSetComponentRequestBus::Handler::BusConnect(GetEntityId());
        }

        //////////////////////////////////////////////////////////////////////////
        void ApplyMotionSetComponent::Deactivate()
        {
            ApplyMotionSetComponentRequestBus::Handler::BusDisconnect();
            AZ::Data::AssetBus::MultiHandler::BusDisconnect();
        }

        //////////////////////////////////////////////////////////////////////////
        void ApplyMotionSetComponent::OnAssetReloaded(AZ::Data::Asset<AZ::Data::AssetData> asset)
        {
            OnAssetReady(asset);
        }

        //////////////////////////////////////////////////////////////////////////
        void ApplyMotionSetComponent::OnAssetReady(AZ::Data::Asset<AZ::Data::AssetData> asset)
        {
#if defined(CARBONATED)
            // Keep the previous asset around until the anim graph instances are removed
            AZ::Data::Asset<AZ::Data::AssetData> prevMotionSetAsset = m_motionSetAsset;
            if (asset == m_motionSetAsset)
            {
                m_motionSetAsset = asset;
            }
#else
            auto& cfg = m_configuration;
            // Keep the previous asset around until the anim graph instances are removed
            AZ::Data::Asset<AZ::Data::AssetData> prevMotionSetAsset = cfg.m_motionSetAsset;
            if (asset == cfg.m_motionSetAsset)
            {
                cfg.m_motionSetAsset = asset;
            }
#endif
        }
    } // namespace Integration
} // namespace EMotionFXAnimation
#endif
