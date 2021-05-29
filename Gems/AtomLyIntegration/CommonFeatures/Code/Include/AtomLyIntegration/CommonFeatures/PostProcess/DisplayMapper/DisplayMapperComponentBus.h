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

#include <AzCore/Component/ComponentBus.h>
#include <Atom/Feature/Material/MaterialAssignment.h>
#include <ACES/Aces.h>

namespace AZ
{
    namespace Render
    {
        struct AcesParameterOverrides;

        //! DisplayMapperComponentRequests provides an interface to request operations on a DisplayMapperComponent
        class DisplayMapperComponentRequests
            : public ComponentBus
        {
        public:
            //! Load preconfigured preset for specific ODT mode
            virtual void LoadPreset(OutputDeviceTransformType preset) = 0;
            //! Set display mapper type
            virtual void SetDisplayMapperOperationType(DisplayMapperOperationType displayMapperOperationType) = 0;
            //! Set custom ACES parameters for ACES mapping, display mapper must be set to Aces to see the difference
            virtual void SetAcesParameterOverrides(const AcesParameterOverrides& parameterOverrides) = 0;
        };
        using DisplayMapperComponentRequestBus = EBus<DisplayMapperComponentRequests>;

        //! DisplayMapperComponent can send out notifications on the DisplayMapperComponentNotifications
        class DisplayMapperComponentNotifications : public ComponentBus
        {
        public:
            //! Notifies that display mapper type changed
            virtual void OntDisplayMapperOperationTypeUpdated([[maybe_unused]] const DisplayMapperOperationType& displayMapperOperationType)
            {
            }

            //! Notifies that ACES parameter overrides changed
            virtual void OnAcesParameterOverridesUpdated([[maybe_unused]] const AcesParameterOverrides& acesParameterOverrides)
            {
            }
        };
        using DisplayMapperComponentNotificationBus = EBus<DisplayMapperComponentNotifications>;

    } // namespace Render
} // namespace AZ
