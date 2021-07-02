/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Component/ComponentBus.h>

namespace LmbrCentral
{
    /*!
     * AudioSwitchComponentRequests EBus Interface
     * Messages serviced by AudioSwitchComponents.
     * A 'Switch' is something that can be in one 'State' at a time.
     * See \ref AudioSwitchComponent for details.
     */
    class AudioSwitchComponentRequests
        : public AZ::ComponentBus
    {
    public:
        //! Set the State on the default assigned Switch.
        virtual void SetState(const char* stateName) = 0;

        //! Set the specified switch to the specified state.
        virtual void SetSwitchState(const char* switchName, const char* stateName) = 0;
    };

    using AudioSwitchComponentRequestBus = AZ::EBus<AudioSwitchComponentRequests>;

} // namespace LmbrCentral
