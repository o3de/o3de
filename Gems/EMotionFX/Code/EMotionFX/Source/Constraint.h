/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include "EMotionFXConfig.h"
#include <Source/Integration/System/SystemCommon.h>


namespace EMotionFX
{

    /**
     * The constraint base class.
     * This applies a given constraint to a given input transform which is then modified.
     */     
    class EMFX_API Constraint
    {
        public:
            AZ_RTTI(Constraint, "{DC5998AD-EE56-4642-AAE7-EA285F0B9B0A}")
            AZ_CLASS_ALLOCATOR(Constraint, EMotionFX::Integration::EMotionFXAllocator)

            Constraint() {}
            virtual ~Constraint() {}

            /**
             * Get the type ID of the constraint, which identifies the type of constraint.
             * @result The type ID.
             */
            virtual uint32 GetType() const = 0;

            /**
             * Get the type string of the constraint, which identifies the type of constraint, using its class name.
             * @result The type string.
             */
            virtual const char* GetTypeString() const = 0;

            /**
             * The main execution function, which performs the actual constraint operation.
             */
            virtual void Execute() = 0;
    };


}   // namespace EMotionFX
