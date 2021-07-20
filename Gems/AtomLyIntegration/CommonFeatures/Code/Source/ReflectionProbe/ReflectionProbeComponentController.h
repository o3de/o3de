/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/Component/Component.h>
#include <AzCore/Component/TransformBus.h>
#include <AzFramework/Visibility/BoundsBus.h>
#include <Atom/Feature/ReflectionProbe/ReflectionProbeFeatureProcessorInterface.h>
#include <Atom/RPI.Public/Model/Model.h>
#include <LmbrCentral/Shape/BoxShapeComponentBus.h>
#include <ReflectionProbe/ReflectionProbeComponentConstants.h>

namespace AZ
{
    namespace Render
    {
        enum class BakedCubeMapQualityLevel : uint32_t
        {
            VeryLow,    // 64
            Low,        // 128
            Medium,     // 256
            High,       // 512
            VeryHigh,   // 1024

            Count
        };

        static const char* BakedCubeMapFileSuffixes[] =
        {
            "_iblspecularcm64.dds",
            "_iblspecularcm128.dds",
            "_iblspecularcm256.dds",
            "_iblspecularcm512.dds",
            "_iblspecularcm1024.dds"
        };

        static_assert(AZ_ARRAY_SIZE(BakedCubeMapFileSuffixes) == aznumeric_cast<uint32_t>(BakedCubeMapQualityLevel::Count),
            "BakedCubeMapFileSuffixes must have the same number of entries as BakedCubeMapQualityLevel");

        class ReflectionProbeComponentConfig final
            : public AZ::ComponentConfig
        {
        public:
            AZ_RTTI(AZ::Render::ReflectionProbeComponentConfig, "{D61730A1-CAF5-448C-B2A3-50D5DC909F31}", ComponentConfig);
            AZ_CLASS_ALLOCATOR(ReflectionProbeComponentConfig, SystemAllocator, 0);
            static void Reflect(AZ::ReflectContext* context);

            float m_outerHeight = DefaultReflectionProbeExtents;
            float m_outerLength = DefaultReflectionProbeExtents;
            float m_outerWidth = DefaultReflectionProbeExtents;
            float m_innerHeight = DefaultReflectionProbeExtents;
            float m_innerLength = DefaultReflectionProbeExtents;
            float m_innerWidth = DefaultReflectionProbeExtents;

            bool m_useParallaxCorrection = true;
            bool m_showVisualization = true;
            bool m_useBakedCubemap = true;

            BakedCubeMapQualityLevel m_bakedCubeMapQualityLevel = BakedCubeMapQualityLevel::Medium;
            AZStd::string m_bakedCubeMapRelativePath;
            Data::Asset<RPI::StreamingImageAsset> m_bakedCubeMapAsset;
            Data::Asset<RPI::StreamingImageAsset> m_authoredCubeMapAsset;
            AZ::u64 m_entityId{ EntityId::InvalidEntityId };
        };

        class ReflectionProbeComponentController final
            : public Data::AssetBus::MultiHandler
            , private TransformNotificationBus::Handler
            , private LmbrCentral::ShapeComponentNotificationsBus::Handler
            , public AzFramework::BoundsRequestBus::Handler
        {
        public:
            friend class EditorReflectionProbeComponent;

            AZ_CLASS_ALLOCATOR(ReflectionProbeComponentController, AZ::SystemAllocator, 0);
            AZ_RTTI(AZ::Render::ReflectionProbeComponentController, "{EFFA88F1-7ED2-4552-B6F6-5E6B2B6D9311}");

            static void Reflect(AZ::ReflectContext* context);
            static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent);
            static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
            static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
            static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);

            ReflectionProbeComponentController() = default;
            ReflectionProbeComponentController(const ReflectionProbeComponentConfig& config);

            void Activate(AZ::EntityId entityId);
            void Deactivate();
            void SetConfiguration(const ReflectionProbeComponentConfig& config);
            const ReflectionProbeComponentConfig& GetConfiguration() const;

            // returns the outer extent Aabb for this reflection
            AZ::Aabb GetAabb() const;

            // initiate the reflection probe bake, invokes callback when complete
            void BakeReflectionProbe(BuildCubeMapCallback callback, const AZStd::string& relativePath);

            // update the currently rendering cubemap asset for this probe
            void UpdateCubeMap();

            // BoundsRequestBus overrides ...
            AZ::Aabb GetWorldBounds() override;
            AZ::Aabb GetLocalBounds() override;

        private:

            AZ_DISABLE_COPY(ReflectionProbeComponentController);

            // Data::AssetBus overrides ...
            void OnAssetReady(Data::Asset<Data::AssetData> asset) override;
            void OnAssetReloaded(Data::Asset<Data::AssetData> asset) override;

            // TransformNotificationBus overrides ...
            void OnTransformChanged(const AZ::Transform& local, const AZ::Transform& world) override;

            // ShapeComponentNotificationsBus overrides ...
            void OnShapeChanged(ShapeChangeReasons changeReason) override;

            // update the feature processor and configuration outer extents
            void UpdateOuterExtents();

            // box shape component, used for defining the outer extents of the probe area
            LmbrCentral::BoxShapeComponentRequests* m_boxShapeInterface = nullptr;
            LmbrCentral::ShapeComponentRequests* m_shapeBus = nullptr;

            // handle for this probe in the feature processor
            ReflectionProbeHandle m_handle;

            ReflectionProbeFeatureProcessorInterface* m_featureProcessor = nullptr;
            TransformInterface* m_transformInterface = nullptr;
            AZ::EntityId m_entityId;
            ReflectionProbeComponentConfig m_configuration;
        };
    } // namespace Render
} // namespace AZ
