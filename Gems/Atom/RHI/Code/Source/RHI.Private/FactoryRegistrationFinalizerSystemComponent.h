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

namespace AZ
{
    namespace RHI
    {
        /*
         * System component in charge of firing the event that tells the Factory Manager that all 
         * available factories have registered. In order to do this it depends on the service
         * provided by the platform factories implementation. This guarantees that it will always be activated after the platform
         * factories. It also provides the RHI component service so other components can activate after the RHI is ready.
         */
        class FactoryRegistrationFinalizerSystemComponent final
            : public AZ::Component
        {
        public:
            AZ_COMPONENT(FactoryRegistrationFinalizerSystemComponent, "{03F8ABE7-C1A9-4B37-AA77-982A28CCA630}");
            static void Reflect(AZ::ReflectContext* context);

            FactoryRegistrationFinalizerSystemComponent() = default;
            ~FactoryRegistrationFinalizerSystemComponent() override = default;

            static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
            static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent);
            static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);

            // AZ::Component overrides ...
            void Activate() override;
            void Deactivate() override;

        private:
            FactoryRegistrationFinalizerSystemComponent(const FactoryRegistrationFinalizerSystemComponent&) = delete;
        };
    } // namespace RHI
} // namespace AZ
