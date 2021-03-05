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

#include <AzCore/Component/Component.h>
#include <AzCore/Asset/AssetCommon.h>

#include <AzToolsFramework/ToolsComponents/EditorComponentBase.h>
#include <Integration/Components/SimpleMotionComponent.h>
#include <Integration/Components/ActorComponent.h>
#include <Integration/SimpleMotionComponentBus.h>
#include <Integration/EditorSimpleMotionComponentBus.h>

namespace EMotionFX
{
    namespace Integration
    {
        class EditorSimpleMotionComponent
            : public AzToolsFramework::Components::EditorComponentBase
            , private AZ::Data::AssetBus::MultiHandler
            , private ActorComponentNotificationBus::Handler
            , private SimpleMotionComponentRequestBus::Handler
            , private EditorSimpleMotionComponentRequestBus::Handler
        {
        public:

            AZ_EDITOR_COMPONENT(EditorSimpleMotionComponent, "{0CF1ADF7-DA51-4183-89EC-BDD7D2E17D36}");

            EditorSimpleMotionComponent();
            ~EditorSimpleMotionComponent() override;

            // AZ::Component interface implementation
            void Activate() override;
            void Deactivate() override;

            // ActorComponentNotificationBus::Handler
            void OnActorInstanceCreated(EMotionFX::ActorInstance* actorInstance) override;
            void OnActorInstanceDestroyed(EMotionFX::ActorInstance* actorInstance) override;

            // AZ::Data::AssetBus::Handler
            void OnAssetReady(AZ::Data::Asset<AZ::Data::AssetData> asset) override;
            void OnAssetReloaded(AZ::Data::Asset<AZ::Data::AssetData> asset) override;

            static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
            {
                SimpleMotionComponent::GetProvidedServices(provided);
            }

            static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent)
            {
                SimpleMotionComponent::GetDependentServices(dependent);
            }

            static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
            {
                SimpleMotionComponent::GetRequiredServices(required);
            }

            static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
            {
                SimpleMotionComponent::GetIncompatibleServices(incompatible);
            }

            static void Reflect(AZ::ReflectContext* context);

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
            void Motion(AZ::Data::AssetId assetId) override;
            AZ::Data::AssetId  GetMotion() const override;
            void BlendInTime(float time) override;
            float GetBlendInTime() const override;
            void BlendOutTime(float time) override;
            float GetBlendOutTime() const override;
            void PlayMotion() override;

            // EditorSimpleMotionComponentRequestBus::Handler
            void SetPreviewInEditor(bool enable) override;
            bool GetPreviewInEditor() const override;
            float GetAssetDuration(const AZ::Data::AssetId& assetId) override;

        private:
            EditorSimpleMotionComponent(const EditorSimpleMotionComponent&) = delete;

            void RemoveMotionInstanceFromActor(EMotionFX::MotionInstance* motionInstance);
            void BuildGameEntity(AZ::Entity* gameEntity) override;
            void VerifyMotionAssetState();
            AZ::Crc32 OnEditorPropertyChanged();

            bool                                    m_previewInEditor;      ///< Plays motion in Editor.
            SimpleMotionComponent::Configuration    m_configuration;
            EMotionFX::ActorInstance*               m_actorInstance;        ///< Associated actor instance (retrieved from Actor Component).
            EMotionFX::MotionInstance*              m_motionInstance;       ///< Motion to play on the actor
            AZ::Data::Asset<MotionAsset>            m_lastMotionAsset;      ///< Last active motion asset, kept alive for blending.
            EMotionFX::MotionInstance*              m_lastMotionInstance;   ///< Last active motion instance, kept alive for blending.
        };
    }
}