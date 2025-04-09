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
#include <Atom/Feature/SpecularReflections/SpecularReflectionsFeatureProcessorInterface.h>
#include <SpecularReflections/SpecularReflectionsComponentConfig.h>

namespace AZ
{
    namespace Render
    {
        class SpecularReflectionsComponentController final
        {
        public:
            friend class EditorSpecularReflectionsComponent;

            AZ_TYPE_INFO(AZ::Render::SpecularReflectionsComponentController, "{8ED8A722-AB11-4603-9C78-E882B544A7EF}");
            static void Reflect(AZ::ReflectContext* context);
            static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
            static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
            static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);

            SpecularReflectionsComponentController() = default;
            SpecularReflectionsComponentController(const SpecularReflectionsComponentConfig& config);

            void Activate(EntityId entityId);
            void Deactivate();
            void SetConfiguration(const SpecularReflectionsComponentConfig& config);
            const SpecularReflectionsComponentConfig& GetConfiguration() const;

        private:
            AZ_DISABLE_COPY(SpecularReflectionsComponentController);

            void OnConfigChanged();

            SpecularReflectionsComponentConfig m_configuration;
            SpecularReflectionsFeatureProcessorInterface* m_featureProcessor = nullptr;
        };
    }
}
