/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Component/Component.h>

namespace AzToolsFramework
{
    class AzToolsFrameworkConfigurationSystemComponent
        : public AZ::Component
    {
    public:
        AZ_COMPONENT(AzToolsFrameworkConfigurationSystemComponent, "{088BE6DC-C01C-415F-AE6D-83C6C0CDB108}", AZ::Component);

        AzToolsFrameworkConfigurationSystemComponent() = default;
        ~AzToolsFrameworkConfigurationSystemComponent() override = default;

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
} // AzToolsFramework
