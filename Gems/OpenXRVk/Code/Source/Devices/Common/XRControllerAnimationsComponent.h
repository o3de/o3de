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
#include <AzCore/Asset/AssetCommon.h>

#include <OpenXRVk/OpenXRVkActionsInterface.h>
#include "XRControllersConfig.h"

namespace OpenXRVk
{
    //! XRControllerAnimationsComponent uses the OpenXRVk::OpenXRActionsInterface to read user input to animate a XR controller.
    class XRControllerAnimationsComponent
        : public AZ::Component
        , public AZ::TickBus::Handler
    {
    public:
        AZ_COMPONENT(XRControllerAnimationsComponent, "{A57B8F98-39A0-4B70-B7F7-2331E62A3276}");

        static void Reflect(AZ::ReflectContext* context);

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent);

    protected:
        // AZ::Component
        void Activate() override;
        void Deactivate() override;

        // AZ::TickBus::Handler
        void OnTick(float deltaTime, AZ::ScriptTimePoint timePoint) override;

    private:
        // Serialized data...
		AZStd::vector<XRControllersConfig> m_controllersConfig;
    };

} // namespace OpenXRVk
