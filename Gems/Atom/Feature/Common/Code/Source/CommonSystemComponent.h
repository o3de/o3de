/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Component/Component.h>
#include <Atom_Feature_Traits_Platform.h>

#include <Atom/RPI.Public/Pass/PassSystemInterface.h>

#if AZ_TRAIT_LUXCORE_SUPPORTED
#include "LuxCore/LuxCoreRenderer.h"
#endif

namespace AZ
{
    namespace Render
    {
        class ModelReloaderSystem;

        class CommonSystemComponent
            : public AZ::Component
        {
        public:
            AZ_COMPONENT(CommonSystemComponent, "{BFB8FE2B-C952-4D0C-8E32-4FE7C7A97757}");

            CommonSystemComponent();
            ~CommonSystemComponent();

            static void Reflect(AZ::ReflectContext* context);

            static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
            static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
            static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
            static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent);

        protected:
            // AZ::Component interface overrides...
            void Init() override;
            void Activate() override;
            void Deactivate() override;

            // Load pass template mappings for this gem
            void LoadPassTemplateMappings();

            RPI::PassSystemInterface::OnReadyLoadTemplatesEvent::Handler m_loadTemplatesHandler;

            AZStd::unique_ptr<ModelReloaderSystem> m_modelReloaderSystem;

#if AZ_TRAIT_LUXCORE_SUPPORTED
            // LuxCore
            LuxCoreRenderer m_luxCore;
#endif
        };
    } // namespace Render
} // namespace AZ
