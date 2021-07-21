/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Component/Component.h>

namespace Vegetation
{
    /**
    * The system component that manages and routes the vegetation data to a manager
    */
    class VegetationSystemComponent
        : public AZ::Component
    {
    public:
        AZ_COMPONENT(VegetationSystemComponent, "{1D766E74-37F4-47BA-B4B8-1D9590B01F23}");
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& services);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& services);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& services);
        static void Reflect(AZ::ReflectContext* context);

        VegetationSystemComponent();
        ~VegetationSystemComponent() override;

    private:
        //////////////////////////////////////////////////////////////////////////
        // AZ::Component
        void Activate() override;
        void Deactivate() override;
    };

} // namespace Vegetation

