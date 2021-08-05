/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/EBus/EBus.h>

namespace PhysX
{
    namespace Editor
    {
        /// Request bus for communicating with the configuration window.
        /// Ensure the configuration window has been opened by 
        /// calling EditorRequests::OpenViewPane before using this bus.
        class ConfigurationWindowRequests
            : public AZ::EBusTraits
        {
        public:
            
            /// Shows the collision layers tab in the configuration window
            virtual void ShowCollisionLayersTab() = 0;

            /// Shows the collision groups tab in the configuration window
            virtual void ShowCollisionGroupsTab() = 0;

            /// Shows the global settings tab in the configuration window
            virtual void ShowGlobalSettingsTab() = 0;

        };

        using ConfigurationWindowRequestBus = AZ::EBus<ConfigurationWindowRequests>;
    }
}
