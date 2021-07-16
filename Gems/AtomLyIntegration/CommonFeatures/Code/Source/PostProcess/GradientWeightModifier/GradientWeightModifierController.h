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
#include <AtomLyIntegration/CommonFeatures/PostProcess/GradientWeightModifier/GradientWeightModifierComponentConfig.h>
#include <AtomLyIntegration/CommonFeatures/PostProcess/PostFxWeightRequestBus.h>

namespace AZ
{
    namespace Render
    {
        class GradientWeightModifierComponentController final
            : public PostFxWeightRequestBus::Handler
        {
        public:
            friend class EditorGradientWeightModifierComponent;

            AZ_TYPE_INFO(AZ::Render::GradientWeightModifierComponentController, "{62AB316D-8B8E-434E-8F87-C4ABC42642A6}");
            static void Reflect(AZ::ReflectContext* context);
            static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
            static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
            static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);

            GradientWeightModifierComponentController() = default;
            GradientWeightModifierComponentController(const GradientWeightModifierComponentConfig& config);

            void Activate(EntityId entityId);
            void Deactivate();
            void SetConfiguration(const GradientWeightModifierComponentConfig& config);
            const GradientWeightModifierComponentConfig& GetConfiguration() const { return m_configuration; }
            virtual float GetWeightAtPosition(const AZ::Vector3& influencerPosition) const override;
        private:
            AZ_DISABLE_COPY(GradientWeightModifierComponentController);

            GradientWeightModifierComponentConfig m_configuration;

            EntityId m_entityId;
        };
    }
}
