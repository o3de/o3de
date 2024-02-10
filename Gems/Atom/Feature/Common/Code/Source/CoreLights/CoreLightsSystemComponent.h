/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Component/Component.h>
#include <CoreLights/LtcCommon.h>

namespace AZ
{
    namespace Render
    {
        class CoreLightsSystemComponent
            : public Component
        {
        public:
            AZ_COMPONENT(CoreLightsSystemComponent, "{40EF99C6-3CA1-4F31-89FB-8E4447A3241F}");

            static void Reflect(ReflectContext* context);

            static void GetProvidedServices(ComponentDescriptor::DependencyArrayType& provided);
            static void GetIncompatibleServices(ComponentDescriptor::DependencyArrayType& incompatible);
            static void GetRequiredServices(ComponentDescriptor::DependencyArrayType& required);

        protected:
        
            // Component overrides ...
            void Init() override;
            void Activate() override;
            void Deactivate() override;

           AZStd::unique_ptr<LtcCommon> m_ltcCommonInterface;
        };
    } // namespace Render
} // namespace AZ
