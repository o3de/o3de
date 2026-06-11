/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/Component.h>
#include <Atom/Feature/GradientGI/GradientGIFeatureProcessorInterface.h>
#include <AtomLyIntegration/CommonFeatures/GradientGI/GradientGIComponentBus.h>
#include <AtomLyIntegration/CommonFeatures/GradientGI/GradientGIComponentConfig.h>

namespace AZ
{
    namespace Render
    {
        class GradientGIComponentController final
            : GradientGIComponentRequestBus::Handler
        {
        public:
            friend class EditorGradientGIComponent;

            AZ_CLASS_ALLOCATOR(GradientGIComponentController, SystemAllocator);
            AZ_RTTI(AZ::Render::GradientGIComponentController, "{D4E5F6A7-B8C9-0123-DEF0-123456789ABC}");

            static void Reflect(ReflectContext* context);
            static void GetProvidedServices(ComponentDescriptor::DependencyArrayType& provided);
            static void GetIncompatibleServices(ComponentDescriptor::DependencyArrayType& incompatible);

            GradientGIComponentController() = default;
            GradientGIComponentController(const GradientGIComponentConfig& config);

            void Activate(EntityId entityId);
            void Deactivate();
            void SetConfiguration(const GradientGIComponentConfig& config);
            const GradientGIComponentConfig& GetConfiguration() const;

        private:
            AZ_DISABLE_COPY(GradientGIComponentController);

            // =====================================================================
            // GradientGIComponentRequestBus
            // =====================================================================

            void SetLowColor(const Color& color) override;
            Color GetLowColor() const override;
            void SetMidColor(const Color& color) override;
            Color GetMidColor() const override;
            void SetHighColor(const Color& color) override;
            Color GetHighColor() const override;
            void SetGradientColors(const Color& low, const Color& mid, const Color& high) override;
            void SetExposure(float exposure) override;
            float GetExposure() const override;
            void SetFaceResolution(int resolution) override;
            int GetFaceResolution() const override;
            void SetUpdateMode(GradientGIUpdateMode mode) override;
            GradientGIUpdateMode GetUpdateMode() const override;

            // =====================================================================
            // Helpers
            // =====================================================================

            void UpdateColors();

            EntityId                         m_entityId;
            GradientGIComponentConfig        m_configuration;
            GradientGIFeatureProcessorInterface* m_featureProcessor = nullptr;
        };

    } // namespace Render
} // namespace AZ
