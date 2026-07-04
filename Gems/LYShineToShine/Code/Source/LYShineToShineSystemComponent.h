/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Component/Component.h>
#include <AzCore/Console/IConsole.h>
#include <AzFramework/API/ApplicationAPI.h>

namespace LYShineToShine
{
    //! System component that provides the "upgrade_canvases" console command
    //! for batch-converting old LyShine v1/v2 .uicanvas files to Shine v3 format.
    class LYShineToShineSystemComponent
        : public AZ::Component
    {
    public:
        AZ_COMPONENT(LYShineToShineSystemComponent, "{A3F7E2B1-94D0-4C8E-B5A1-6D3F2E8C9B47}");

        static void Reflect(AZ::ReflectContext* context);
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);

        void Activate() override;
        void Deactivate() override;
    };
} // namespace LYShineToShine
