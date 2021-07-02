/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/Component.h>
#include <AzCore/Component/TransformBus.h>
#include <Atom/Feature/DiffuseGlobalIllumination/DiffuseGlobalIlluminationFeatureProcessorInterface.h>
#include <DiffuseGlobalIllumination/DiffuseGlobalIlluminationComponentConfig.h>

namespace AZ
{
    namespace Render
    {
        class DiffuseGlobalIlluminationComponentController final
        {
        public:
            friend class EditorDiffuseGlobalIlluminationComponent;

            AZ_TYPE_INFO(AZ::Render::DiffuseGlobalIlluminationComponentController, "{7DE7D2A0-2526-447C-A11F-C31EE1332C26}");
            static void Reflect(AZ::ReflectContext* context);
            static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
            static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
            static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);

            DiffuseGlobalIlluminationComponentController() = default;
            DiffuseGlobalIlluminationComponentController(const DiffuseGlobalIlluminationComponentConfig& config);

            void Activate(EntityId entityId);
            void Deactivate();
            void SetConfiguration(const DiffuseGlobalIlluminationComponentConfig& config);
            const DiffuseGlobalIlluminationComponentConfig& GetConfiguration() const;

        private:
            AZ_DISABLE_COPY(DiffuseGlobalIlluminationComponentController);

            void OnConfigChanged();

            DiffuseGlobalIlluminationComponentConfig m_configuration;
            DiffuseGlobalIlluminationFeatureProcessorInterface* m_featureProcessor = nullptr;
        };
    }
}
