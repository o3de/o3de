/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
