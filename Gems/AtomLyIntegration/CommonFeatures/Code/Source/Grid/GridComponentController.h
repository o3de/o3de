/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/Component.h>
#include <AzCore/Component/TransformBus.h>
#include <AtomLyIntegration/CommonFeatures/Grid/GridComponentBus.h>
#include <AtomLyIntegration/CommonFeatures/Grid/GridComponentConfig.h>
#include <Atom/RPI.Public/SceneBus.h>

namespace AZ
{
    namespace Render
    {
        //! Controls behavior and rendering of a wireframe grid
        class GridComponentController final
            : public GridComponentRequestBus::Handler
            , public AZ::TransformNotificationBus::Handler
            , public AZ::RPI::SceneNotificationBus::Handler
        {
        public:
            friend class EditorGridComponent;

            AZ_CLASS_ALLOCATOR(GridComponentController, SystemAllocator, 0)
            AZ_RTTI(AZ::Render::GridComponentController, "{D2FF04F5-2F8D-44C5-99CA-A6FF800187DD}");

            static void Reflect(ReflectContext* context);
            static void GetProvidedServices(ComponentDescriptor::DependencyArrayType& provided);
            static void GetIncompatibleServices(ComponentDescriptor::DependencyArrayType& incompatible);

            GridComponentController() = default;
            GridComponentController(const GridComponentConfig& config);

            void Activate(EntityId entityId);
            void Deactivate();
            void SetConfiguration(const GridComponentConfig& config);
            const GridComponentConfig& GetConfiguration() const;

            static constexpr float MinGridSize = 0.0f;
            static constexpr float MaxGridSize = 1000000.0f;
            static constexpr float MinSpacing = 0.01f;

        private:
            AZ_DISABLE_COPY(GridComponentController);

            //! GridComponentRequestBus overrides...
            void SetSize(float gridSize) override;
            float GetSize() const override;
            void SetPrimarySpacing(float gridPrimarySpacing) override;
            float GetPrimarySpacing() const override;
            void SetSecondarySpacing(float gridSecondarySpacing) override;
            float GetSecondarySpacing() const override;

            void SetAxisColor(const AZ::Color& gridAxisColor) override;
            AZ::Color GetAxisColor() const override;
            void SetPrimaryColor(const AZ::Color& gridPrimaryColor) override;
            AZ::Color GetPrimaryColor() const override;
            void SetSecondaryColor(const AZ::Color& gridSecondaryColor) override;
            AZ::Color GetSecondaryColor() const override;

            //! AZ::TransformNotificationBus::Handler overrides ...
            void OnTransformChanged(const Transform& local, const Transform& world) override;

            // AZ::RPI::SceneNotificationBus::Handler overrides ...
            void OnBeginPrepareRender() override;

            void BuildGrid();

            EntityId m_entityId;
            GridComponentConfig m_configuration;
            AZStd::vector<AZ::Vector3> m_axisGridPoints;
            AZStd::vector<AZ::Vector3> m_primaryGridPoints;
            AZStd::vector<AZ::Vector3> m_secondaryGridPoints;
            bool m_dirty = true; // must be set to true for any configuration change that rebuilds the grid
        };
    } // namespace Render
} // namespace AZeneccccckfdljrndicnfefjdklthhfifnnvkffebgkbd
