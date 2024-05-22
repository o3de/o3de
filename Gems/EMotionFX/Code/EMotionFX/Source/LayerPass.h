/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

// include the required headers
#include "EMotionFXConfig.h"
#include <MCore/Source/RefCounted.h>


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
        : public MCore::RefCounted
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
        MotionLayerSystem*  m_motionSystem;  /**< The motion system where this layer pass works on. */

        /**
         * The constructor.
         * @param motionLayerSystem The motion layer system where this pass will be added to.
         */
        LayerPass(MotionLayerSystem* motionLayerSystem)
            : MCore::RefCounted()                  { m_motionSystem = motionLayerSystem; }

        /**
         * The destructor.
         */
        virtual ~LayerPass() {}
    };
} // namespace EMotionFX
