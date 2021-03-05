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
