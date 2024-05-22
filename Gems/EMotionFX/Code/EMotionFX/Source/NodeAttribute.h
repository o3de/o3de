/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

// include MCore related files
#include "EMotionFXConfig.h"
#include <MCore/Source/RefCounted.h>


namespace EMotionFX
{
    /**
     * The node attribute base class.
     * Custom attributes can be attached to every node.
     * An example of an attribute could be physics properties.
     * In order to create your own node attribute, simply inherit a class from this base class.
     */
    class EMFX_API NodeAttribute
        : public MCore::RefCounted
    {
    public:
        /**
         * Get the attribute type.
         * @result The attribute ID.
         */
        virtual uint32 GetType() const = 0;

        /**
         * Get the attribute type as a string.
         * This string should contain the name of the class.
         * @result The string containing the type name.
         */
        virtual const char* GetTypeString() const = 0;

        /**
         * Clone the node attribute.
         * @result Returns a pointer to a newly created exact copy of the node attribute.
         */
        virtual NodeAttribute* Clone() const = 0;

    protected:
        /**
         * The constructor.
         */
        NodeAttribute()
            : MCore::RefCounted()
        {
        }

        /**
         * The destructor.
         */
        virtual ~NodeAttribute() {}
    };
} // namespace EMotionFX
