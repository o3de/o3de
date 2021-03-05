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

// include the required headers
#include "EMotionFXConfig.h"
#include "BaseObject.h"


namespace EMotionFX
{
    // forward declarations
    class MotionLayerSystem;


    /**
     * The layer pass base class.
     * Layer passes can be seen as post processes that take the motion layers of the MotionLayerSystem class as input.
     * Sometimes it is needed to manually process blending of some specific properties such as facial expression weights.
     */
    class EMFX_API LayerPass
        : public BaseObject
    {
    public:
        /**
         * Get the unique type ID of the layer pass class.
         * @result The unique type ID number of this layer pass type.
         */
        virtual uint32 GetType() const = 0;

        /**
         * Process the layer pass.
         */
        virtual void Process() = 0;


    protected:
        MotionLayerSystem*  mMotionSystem;  /**< The motion system where this layer pass works on. */

        /**
         * The constructor.
         * @param motionLayerSystem The motion layer system where this pass will be added to.
         */
        LayerPass(MotionLayerSystem* motionLayerSystem)
            : BaseObject()                  { mMotionSystem = motionLayerSystem; }

        /**
         * The destructor.
         */
        virtual ~LayerPass() {}
    };
} // namespace EMotionFX
