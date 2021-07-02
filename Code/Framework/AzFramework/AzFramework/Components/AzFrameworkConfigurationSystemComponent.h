/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Component/Component.h>

namespace AzFramework
{
    class AzFrameworkConfigurationSystemComponent
        : public AZ::Component
    {
    public:
        AZ_COMPONENT(AzFrameworkConfigurationSystemComponent, "{19BB423E-B7FD-45D3-8673-A7FA5A4C92F6}", AZ::Component);

        AzFrameworkConfigurationSystemComponent() = default;
        ~AzFrameworkConfigurationSystemComponent() override = default;

        //////////////////////////////////////////////////////////////////////////
        // Component overrides
        void Activate() override;
        void Deactivate() override;
        //////////////////////////////////////////////////////////////////////////

        static void Reflect(AZ::ReflectContext* context);

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent);

    };
} // AzFramework
