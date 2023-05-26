/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if defined(CARBONATED)
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzCore/Script/ScriptProperty.h>

#include <AzToolsFramework/ToolsComponents/EditorComponentBase.h>

#include <Integration/Components/ApplyMotionSetComponent.h>

namespace EMotionFX
{
    namespace Integration
    {
        class EditorApplyMotionSetComponent
            : public AzToolsFramework::Components::EditorComponentBase
            , private AZ::Data::AssetBus::MultiHandler
            , private EditorApplyMotionSetComponentRequestBus::Handler
        {
        public:

            AZ_EDITOR_COMPONENT(EditorApplyMotionSetComponent, "{2734A694-3E28-46B4-9917-342FDA60BC0E}");

            EditorApplyMotionSetComponent();
            ~EditorApplyMotionSetComponent() override;

            //////////////////////////////////////////////////////////////////////////
            // AZ::Component interface implementation
            void Activate() override;
            void Deactivate() override;

            //////////////////////////////////////////////////////////////////////////
            // AZ::Data::AssetBus::Handler
            void OnAssetReady(AZ::Data::Asset<AZ::Data::AssetData> asset) override;
            void OnAssetReloaded(AZ::Data::Asset<AZ::Data::AssetData> asset) override;
            //////////////////////////////////////////////////////////////////////////

            //////////////////////////////////////////////////////////////////////////
            // EditorAnimGraphComponentRequestBus::Handler
            const AZ::Data::AssetId& GetMotionSetAssetId() override;

            //////////////////////////////////////////////////////////////////////////
            static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
            {
                ApplyMotionSetComponent::GetProvidedServices(provided);
            }

            static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
            {
                ApplyMotionSetComponent::GetIncompatibleServices(incompatible);
            }

            static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent)
            {
                ApplyMotionSetComponent::GetDependentServices(dependent);
            }

            static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
            {
                ApplyMotionSetComponent::GetRequiredServices(required);
            }

            static void Reflect(AZ::ReflectContext* context);
            //////////////////////////////////////////////////////////////////////////

        private:
#if defined(CARBONATED)
            const AZStd::vector<AZStd::string>& GetMotionAssetOptionNames() { return m_motionSetAssetNames; }
#else           
            AZ::Data::Asset<MotionSetAsset>* GetMotionAsset() { return &m_motionSetAsset; }
#endif

            // Property callbacks.
            AZ::u32 OnMotionSetAssetSelected();

            // Called at edit-time when creating the component directly from an asset.
            void SetPrimaryAsset(const AZ::Data::AssetId& assetId) override;

            // Called at export-time to produce runtime entities/components.
            void BuildGameEntity(AZ::Entity* gameEntity) override;

            AZ::Data::Asset<MotionSetAsset>                                     m_motionSetAsset;           ///< Selected motion set asset.
            AZStd::string                                                       m_activeMotionSetName;      ///< Selected motion set name.

#if defined(CARBONATED)
            AZ::Data::Asset<MotionSetAsset>                                     m_derivedMotionSetAsset;    ///< Derived motion set asset.
            MotionSetGender                                                     m_derivedMotionSetGender;   ///< Derived motion set gender options.
            AZStd::string                                                       m_derivedMotionSetName;     ///< Derived motion set name.

            AZStd::vector<AZStd::string>                                        m_motionSetAssetNames;      ///< Possible motion set assets names.
            AZStd::map<MotionSetGender, AZ::Data::Asset<MotionSetAsset>>        m_motionSetAssetMap;        ///< Possible motion set assets.

            bool m_loadDerivedDeferred = true;
#endif
        };

    } // namespace Integration
} // namespace EMotionFX
#endif