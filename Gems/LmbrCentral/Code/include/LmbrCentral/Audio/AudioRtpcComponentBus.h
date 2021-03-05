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
     * AudioRtpcComponentRequests EBus Interface
     * Messages serviced by AudioRtpcComponents.
     * Rtpc = Real-Time Parameter Control
     * See \ref AudioRtpcComponent for details.
     */
    class AudioRtpcComponentRequests
        : public AZ::ComponentBus
    {
    public:
        //! Set the Rtpc Value, uses a serialized Rtpc name.
        virtual void SetValue(float value) = 0;

        //! Set the Rtpc Value, specify an Rtpc name at runtime (i.e. script).
        virtual void SetRtpcValue(const char* rtpcName, float value) = 0;
    };

    using AudioRtpcComponentRequestBus = AZ::EBus<AudioRtpcComponentRequests>;

} // namespace LmbrCentral
