/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Component/Component.h>
#include <AzCore/Component/TickBus.h>
#include <AzCore/Math/Crc.h>
#include <AzCore/AzCoreAPI.h>

namespace AZ
{
    /**
     *
     */
    class AZCORE_API AssetManagerComponent
        : public Component
        , public SystemTickBus::Handler
    {
    public:
        AZ_COMPONENT(AssetManagerComponent, "{D5A73BCC-0098-4d1e-8FE4-C86101E374AC}", Component)

        AssetManagerComponent();

    protected:
        //////////////////////////////////////////////////////////////////////////
        // Component base
        void Activate() override;
        void Deactivate() override;
        //////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        // SystemTickBus
        void    OnSystemTick() override;
        //////////////////////////////////////////////////////////////////////////

        /// \ref ComponentDescriptor::GetProvidedServices
        static void GetProvidedServices(ComponentDescriptor::DependencyArrayType& provided);
        /// \ref ComponentDescriptor::GetIncompatibleServices
        static void GetIncompatibleServices(ComponentDescriptor::DependencyArrayType& incompatible);
        /// \ref ComponentDescriptor::GetRequiredServices
        static void GetRequiredServices(ComponentDescriptor::DependencyArrayType& required);
        /// \ref ComponentDescriptor::Reflect
        static void Reflect(ReflectContext* reflection);
    };
}
