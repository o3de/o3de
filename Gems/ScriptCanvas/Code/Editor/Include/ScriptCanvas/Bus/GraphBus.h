/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/EBus/EBus.h>

#include <ScriptCanvas/Core/Core.h>


namespace ScriptCanvasEditor
{   
    class GeneralGraphEvents : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;

        virtual void OnBuildGameEntity([[maybe_unused]] const AZStd::string& name, [[maybe_unused]] const AZ::EntityId& editGraphId, [[maybe_unused]] const ScriptCanvas::ScriptCanvasId& scriptCanvasId) {}
    };

    using GeneralGraphEventBus = AZ::EBus<GeneralGraphEvents>;
}
