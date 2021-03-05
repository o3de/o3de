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