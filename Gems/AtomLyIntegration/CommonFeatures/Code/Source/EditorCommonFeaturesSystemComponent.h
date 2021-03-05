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

#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/Component/Component.h>
#include <AzToolsFramework/API/EditorLevelNotificationBus.h>
#include <AzToolsFramework/Entity/SliceEditorEntityOwnershipServiceBus.h>

namespace AZ
{
    namespace Render
    {
        class SkinnedMeshDebugDisplay;

        //! This is the editor-counterpart to this gem's main CommonSystemComponent class.
        class EditorCommonFeaturesSystemComponent
            : public AZ::Component
            , public AzToolsFramework::EditorLevelNotificationBus::Handler
            , public AzToolsFramework::SliceEditorEntityOwnershipServiceNotificationBus::Handler
        {
        public:
            AZ_COMPONENT(EditorCommonFeaturesSystemComponent, "{D73D77CF-D5AF-428B-909B-324E96D3DEF5}");

            EditorCommonFeaturesSystemComponent();
            ~EditorCommonFeaturesSystemComponent();

            static void Reflect(AZ::ReflectContext* context);

            static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
            static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
            static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
            static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent);

        protected:
            // AZ::Component interface implementation
            void Init() override;
            void Activate() override;
            void Deactivate() override;

            // EditorLevelNotificationBus overrides ...
            void OnNewLevelCreated() override;

            // SliceEditorEntityOwnershipServiceBus overrides ...
            void OnSliceInstantiated(const AZ::Data::AssetId&, AZ::SliceComponent::SliceInstanceAddress&, const AzFramework::SliceInstantiationTicket&) override;
            void OnSliceInstantiationFailed(const AZ::Data::AssetId&, const AzFramework::SliceInstantiationTicket&) override;

        private:
            AZStd::unique_ptr<SkinnedMeshDebugDisplay> m_skinnedMeshDebugDisplay;

            AZ::Data::AssetId m_levelDefaultSliceAssetId;
            AZStd::string m_atomLevelDefaultAssetPath{ "LevelAssets/default.slice" };
            float m_envProbeHeight{ 200.0f };
        };
    } // namespace Render
} // namespace AZ
