/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/EBus/EBus.h>


namespace EMotionFX
{
    /**
    * EMotion FX Anim Graph Editor Request Bus
    * Used for making requests to anim graph editor.
    */
    class SimulatedObjectRequests
        : public AZ::EBusTraits
    {
    public:
        /**
        * Called whenever something inside an object changes that influences the simulated object widget.
        * @param[in] object The object that changed and requests the UI sync.
        */
        virtual void UpdateWidget() {}
    };

    using SimulatedObjectRequestBus = AZ::EBus<SimulatedObjectRequests>;
} // namespace EMotionFX
