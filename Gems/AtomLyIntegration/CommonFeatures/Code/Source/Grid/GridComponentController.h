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
#include <AzCore/Component/TickBus.h>
#include <AzCore/Component/TransformBus.h>
#include <AtomLyIntegration/CommonFeatures/Grid/GridComponentBus.h>
#include <AtomLyIntegration/CommonFeatures/Grid/GridComponentConfig.h>

namespace AZ
{
    namespace Render
    {
        //! Controls behavior and rendering of a wireframe grid
        class GridComponentController final
            : public GridComponentRequestBus::Handler
            , public AZ::TickBus::Handler
            , public AZ::TransformNotificationBus::Handler
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

            // AZ::TickBus::Handler overrides ...
            void OnTick(float deltaTime, AZ::ScriptTimePoint time) override;

            //! AZ::TransformNotificationBus::Handler overrides ...
            void OnTransformChanged(const Transform& local, const Transform& world) override;

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
