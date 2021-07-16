/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#pragma once

#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzCore/Script/ScriptProperty.h>

#include <AzToolsFramework/ToolsComponents/EditorComponentBase.h>

#include <Integration/Components/AnimGraphComponent.h>

namespace EMotionFX
{
    class ValueParameter;

    namespace Integration
    {
        class EditorAnimGraphComponent
            : public AzToolsFramework::Components::EditorComponentBase
            , private AZ::Data::AssetBus::MultiHandler
            , private EditorAnimGraphComponentRequestBus::Handler
        {
        public:

            AZ_EDITOR_COMPONENT(EditorAnimGraphComponent, "{770F0A71-59EA-413B-8DAB-235FB0FF1384}");

            EditorAnimGraphComponent();
            ~EditorAnimGraphComponent() override;

            //////////////////////////////////////////////////////////////////////////
            // AZ::Component interface implementation
            void Activate() override;
            void Deactivate() override;
            //////////////////////////////////////////////////////////////////////////

            void LaunchAnimationEditor(const AZ::Data::AssetId& assetId, const AZ::Data::AssetType&);

            //////////////////////////////////////////////////////////////////////////
            // AZ::Data::AssetBus::Handler
            void OnAssetReady(AZ::Data::Asset<AZ::Data::AssetData> asset) override;
            void OnAssetReloaded(AZ::Data::Asset<AZ::Data::AssetData> asset) override;
            //////////////////////////////////////////////////////////////////////////

            //////////////////////////////////////////////////////////////////////////
            // EditorAnimGraphComponentRequestBus::Handler
            const AZ::Data::AssetId& GetAnimGraphAssetId() override;
            const AZ::Data::AssetId& GetMotionSetAssetId() override;

            void SetAnimGraphAssetId(const AZ::Data::AssetId& assetId);
            void SetMotionSetAssetId(const AZ::Data::AssetId& assetId);

            //////////////////////////////////////////////////////////////////////////
            static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
            {
                AnimGraphComponent::GetProvidedServices(provided);
            }

            static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
            {
                AnimGraphComponent::GetIncompatibleServices(incompatible);
            }

            static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent)
            {
                AnimGraphComponent::GetDependentServices(dependent);
            }

            static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
            {
                AnimGraphComponent::GetRequiredServices(required);
            }

            static void Reflect(AZ::ReflectContext* context);
            //////////////////////////////////////////////////////////////////////////

        private:
            AZ::Data::Asset<MotionSetAsset>* GetMotionAsset() { return &m_motionSetAsset; }

            // Property callbacks.
            AZ::u32 OnAnimGraphAssetSelected();
            AZ::u32 OnMotionSetAssetSelected();

            bool IsSupportedScriptPropertyType(const ValueParameter* param) const;

            // Called at edit-time when creating the component directly from an asset.
            void SetPrimaryAsset(const AZ::Data::AssetId& assetId) override;

            // Called at export-time to produce runtime entities/components.
            void BuildGameEntity(AZ::Entity* gameEntity) override;

            AZ::Data::Asset<AnimGraphAsset>             m_animGraphAsset;       ///< Selected anim graph.
            AZ::Data::Asset<MotionSetAsset>             m_motionSetAsset;       ///< Selected motion set asset.
            AZStd::string                               m_activeMotionSetName;  ///< Selected motion set.
            bool                                        m_visualize = false;    ///< Enable debug visualisation?
            AnimGraphComponent::ParameterDefaults       m_parameterDefaults;    ///< AnimGraph parameter defaults.
        };

    } // namespace Integration
} // namespace EMotionFX

