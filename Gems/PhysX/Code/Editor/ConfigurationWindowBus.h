/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
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
