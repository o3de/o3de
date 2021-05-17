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
#include <Atom_Feature_Traits_Platform.h>

#include <Atom/RPI.Public/Pass/PassSystemInterface.h>

#if AZ_TRAIT_LUXCORE_SUPPORTED
#include "LuxCore/LuxCoreRenderer.h"
#endif

namespace AZ
{
    namespace Render
    {
        class CommonSystemComponent
            : public AZ::Component
        {
        public:
            AZ_COMPONENT(CommonSystemComponent, "{BFB8FE2B-C952-4D0C-8E32-4FE7C7A97757}");

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

#if AZ_TRAIT_LUXCORE_SUPPORTED
            // LuxCore
            LuxCoreRenderer m_luxCore;
#endif
        };
    } // namespace Render
} // namespace AZ
