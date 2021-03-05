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
#include <AzCore/Component/TransformBus.h>
#include <Atom/Feature/DiffuseProbeGrid/DiffuseProbeGridFeatureProcessorInterface.h>
#include <Atom/RPI.Public/Model/Model.h>
#include <LmbrCentral/Shape/BoxShapeComponentBus.h>
#include <DiffuseProbeGrid/DiffuseProbeGridComponentConstants.h>

namespace AZ
{
    namespace Render
    {
        class DiffuseProbeGridComponentConfig final
            : public AZ::ComponentConfig
        {
        public:
            AZ_RTTI(AZ::Render::DiffuseProbeGridComponentConfig, "{BF190F2A-D7F7-453B-9D42-5CE940180DCE}", ComponentConfig);
            AZ_CLASS_ALLOCATOR(DiffuseProbeGridComponentConfig, SystemAllocator, 0);
            static void Reflect(AZ::ReflectContext* context);

            DiffuseProbeGridComponentConfig() = default;

            Vector3 m_extents = AZ::Vector3(DefaultDiffuseProbeGridExtents);
            Vector3 m_probeSpacing = AZ::Vector3(DefaultDiffuseProbeGridSpacing);
            float m_ambientMultiplier = DefaultDiffuseProbeGridAmbientMultiplier;
            float m_viewBias = DefaultDiffuseProbeGridViewBias;
            float m_normalBias = DefaultDiffuseProbeGridNormalBias;
        };

        class DiffuseProbeGridComponentController final
            : public Data::AssetBus::MultiHandler
            , private TransformNotificationBus::Handler
            , private LmbrCentral::ShapeComponentNotificationsBus::Handler
        {
        public:
            friend class EditorDiffuseProbeGridComponent;

            AZ_CLASS_ALLOCATOR(DiffuseProbeGridComponentController, AZ::SystemAllocator, 0);
            AZ_RTTI(AZ::Render::DiffuseProbeGridComponentController, "{108588E8-355E-4A19-94AC-955E64A37CE2}");

            static void Reflect(AZ::ReflectContext* context);
            static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent);
            static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
            static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
            static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);

            DiffuseProbeGridComponentController() = default;
            DiffuseProbeGridComponentController(const DiffuseProbeGridComponentConfig& config);

            void Activate(AZ::EntityId entityId);
            void Deactivate();
            void SetConfiguration(const DiffuseProbeGridComponentConfig& config);
            const DiffuseProbeGridComponentConfig& GetConfiguration() const;

            // returns the Aabb for this grid
            AZ::Aabb GetAabb() const;

        private:

            AZ_DISABLE_COPY(DiffuseProbeGridComponentController);

            // TransformNotificationBus overrides
            void OnTransformChanged(const AZ::Transform& local, const AZ::Transform& world) override;

            // ShapeComponentNotificationsBus overrides
            void OnShapeChanged(ShapeChangeReasons changeReason) override;

            // Property handlers
            bool ValidateProbeSpacing(const AZ::Vector3& newSpacing);
            void SetProbeSpacing(const AZ::Vector3& probeSpacing);
            void SetAmbientMultiplier(float ambientMultiplier);
            void SetViewBias(float viewBias);
            void SetNormalBias(float normalBias);

            // box shape component, used for defining the outer extents of the probe area
            LmbrCentral::BoxShapeComponentRequests* m_boxShapeInterface = nullptr;
            LmbrCentral::ShapeComponentRequests* m_shapeBus = nullptr;

            // handle for this probe in the feature processor
            DiffuseProbeGridHandle m_handle;

            DiffuseProbeGridFeatureProcessorInterface* m_featureProcessor = nullptr;
            TransformInterface* m_transformInterface = nullptr;
            AZ::EntityId m_entityId;
            DiffuseProbeGridComponentConfig m_configuration;
            bool m_inShapeChangeHandler = false;
        };
    } // namespace Render
} // namespace AZ
