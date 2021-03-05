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

#include <CryCommon/CryFontBus.h>
#include <CryCommon/CrySystemBus.h>

namespace AZ
{
    namespace Render
    {
        class AtomFontSystemComponent
            : public AZ::Component
            , private AZ::CryFontCreationRequestBus::Handler
            , private CrySystemEventBus::Handler
        {
        public:
            AZ_COMPONENT(AtomFontSystemComponent, "{29DC7010-CF2A-4EE4-91F8-8E3C8BE65F41}");

            static void Reflect(AZ::ReflectContext* context);

            static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
            static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
            static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
            static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent);

        protected:
            ////////////////////////////////////////////////////////////////////////
            // AZ::Component interface implementation
            void Activate() override;
            void Deactivate() override;
            ////////////////////////////////////////////////////////////////////////

            ////////////////////////////////////////////////////////////////////////
            // CryFontCreationBus
            bool CreateCryFont(SSystemGlobalEnvironment& env, const SSystemInitParams& initParams) override;
            ////////////////////////////////////////////////////////////////////////

            ////////////////////////////////////////////////////////////////////////
            // CryFontCreationBus
            void OnCrySystemShutdown(ISystem& system) override;
            ////////////////////////////////////////////////////////////////////////
        };
    } // namespace Render
} // namespace AZ
