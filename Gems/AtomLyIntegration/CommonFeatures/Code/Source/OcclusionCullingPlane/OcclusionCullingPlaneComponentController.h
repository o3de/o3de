/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/Component/Component.h>
#include <AzCore/Component/TransformBus.h>
#include <Atom/Feature/OcclusionCullingPlane/OcclusionCullingPlaneFeatureProcessorInterface.h>
#include <Atom/RPI.Public/Model/Model.h>
#include <LmbrCentral/Shape/BoxShapeComponentBus.h>
#include <OcclusionCullingPlane/OcclusionCullingPlaneComponentConstants.h>

namespace AZ
{
    namespace Render
    {
        class OcclusionCullingPlaneComponentConfig final
            : public AZ::ComponentConfig
        {
        public:
            AZ_RTTI(AZ::Render::OcclusionCullingPlaneComponentConfig, "{D0E107CA-5AFB-4675-BC97-94BCA5F248DB}", ComponentConfig);
            AZ_CLASS_ALLOCATOR(OcclusionCullingPlaneComponentConfig, SystemAllocator);
            static void Reflect(AZ::ReflectContext* context);

            bool m_showVisualization = true;
            bool m_transparentVisualization = false;

            OcclusionCullingPlaneComponentConfig() = default;
        };

        class OcclusionCullingPlaneComponentController final
            : public Data::AssetBus::MultiHandler
            , private TransformNotificationBus::Handler
        {
        public:
            friend class EditorOcclusionCullingPlaneComponent;

            AZ_CLASS_ALLOCATOR(OcclusionCullingPlaneComponentController, AZ::SystemAllocator);
            AZ_RTTI(AZ::Render::OcclusionCullingPlaneComponentController, "{8EDA3C7D-5171-4843-9969-4D84DB13F221}");

            static void Reflect(AZ::ReflectContext* context);
            static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent);
            static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
            static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
            static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);

            OcclusionCullingPlaneComponentController() = default;
            OcclusionCullingPlaneComponentController(const OcclusionCullingPlaneComponentConfig& config);

            void Activate(AZ::EntityId entityId);
            void Deactivate();
            void SetConfiguration(const OcclusionCullingPlaneComponentConfig& config);
            const OcclusionCullingPlaneComponentConfig& GetConfiguration() const;

        private:

            AZ_DISABLE_COPY(OcclusionCullingPlaneComponentController);

            // TransformNotificationBus overrides
            void OnTransformChanged(const AZ::Transform& local, const AZ::Transform& world) override;

            // handle for this occlusion plane in the feature processor
            OcclusionCullingPlaneHandle m_handle;

            OcclusionCullingPlaneFeatureProcessorInterface* m_featureProcessor = nullptr;
            TransformInterface* m_transformInterface = nullptr;
            AZ::EntityId m_entityId;
            OcclusionCullingPlaneComponentConfig m_configuration;
        };
    } // namespace Render
} // namespace AZ
