/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/ComponentBus.h>

namespace Vegetation
{
    class AreaSystemConfig;
    class InstanceSystemConfig;

    class LevelSettingsRequests
        : public AZ::ComponentBus
    {
    public:
        /**
         * Overrides the default AZ::EBusTraits handler policy to allow one
         * listener only.
         */
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;

        virtual AreaSystemConfig* GetAreaSystemConfig() = 0;
        virtual InstanceSystemConfig* GetInstanceSystemConfig() = 0;

    };

    using LevelSettingsRequestBus = AZ::EBus<LevelSettingsRequests>;
}
