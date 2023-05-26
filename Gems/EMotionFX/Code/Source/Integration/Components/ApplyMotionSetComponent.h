/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if defined(CARBONATED)
#include <AzCore/Component/Component.h>
#include <AzCore/Component/TransformBus.h>
#include <AzCore/Component/TickBus.h>
#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/Script/ScriptProperty.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>

#include <Integration/Assets/MotionSetAsset.h>
#include <Integration/ApplyMotionSetComponentBus.h>
#include <AzCore/Component/EntityId.h>

namespace EMotionFX
{
    namespace Integration
    {
        class ApplyMotionSetComponent
            : public AZ::Component
            , private AZ::Data::AssetBus::MultiHandler
            , private ApplyMotionSetComponentRequestBus::Handler
        {
        public:

            friend class EditorApplyMotionSetComponent;

            AZ_COMPONENT(ApplyMotionSetComponent, "{1B4ED2C1-58F5-44A4-BF6E-C22667AC60CD}");

            /**
            * Configuration struct for procedural configuration of Actor Components.
            */
            struct Configuration
            {
                AZ_TYPE_INFO(Configuration, "{C1DD0FAF-0DEA-4965-940A-0E8A3FE8EABD}")

                Configuration();

#if defined(CARBONATED)
                AZStd::map<MotionSetGender, AZ::Data::Asset<MotionSetAsset>>        m_motionSetAssetMap;        ///< Possible motion set assets.
#else
                AZ::Data::Asset<MotionSetAsset>     m_motionSetAsset;           ///< Selected motion set asset.
                AZStd::string                       m_activeMotionSetName;      ///< Selected motion set.
#endif

                static void Reflect(AZ::ReflectContext* context);
            };

            ApplyMotionSetComponent(const Configuration* config = nullptr);
            ~ApplyMotionSetComponent() override;
            
            //////////////////////////////////////////////////////////////////////////
            // AZ::Component interface implementation
            void Init() override;
            void Activate() override;
            void Deactivate() override;
            //////////////////////////////////////////////////////////////////////////

            //////////////////////////////////////////////////////////////////////////
            // ApplyMotionSetComponentRequestBus::Handler
            //////////////////////////////////////////////////////////////////////////
#if defined(CARBONATED)
            void Apply(const AZ::EntityId& id, const MotionSetGender& preferredGender);
#else
            void Apply(const AZ::EntityId& id);
#endif

            //////////////////////////////////////////////////////////////////////////
            static void GetProvidedServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& provided)
            {
            }

            static void GetIncompatibleServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& incompatible)
            {
            }

            static void GetDependentServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& dependent)
            {
            }

            static void GetRequiredServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& required)
            {
            }

            static void Reflect(AZ::ReflectContext* context);
            //////////////////////////////////////////////////////////////////////////

        private:
            // AZ::Data::AssetBus::Handler
            void OnAssetReady(AZ::Data::Asset<AZ::Data::AssetData> asset) override;
            void OnAssetReloaded(AZ::Data::Asset<AZ::Data::AssetData> asset) override;

            Configuration                               m_configuration;        ///< Component configuration.
#if defined(CARBONATED)
            AZ::Data::Asset<MotionSetAsset>             m_motionSetAsset;       ///< Selected motion set asset.
#endif
        };

    } // namespace Integration
} // namespace EMotionFX

#endif
