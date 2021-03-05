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

// include MCore related files
#include "EMotionFXConfig.h"
#include "BaseObject.h"


namespace EMotionFX
{
    /**
     * The node attribute base class.
     * Custom attributes can be attached to every node.
     * An example of an attribute could be physics properties.
     * In order to create your own node attribute, simply inherit a class from this base class.
     */
    class EMFX_API NodeAttribute
        : public BaseObject
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
        virtual NodeAttribute* Clone() = 0;

    protected:
        /**
         * The constructor.
         */
        NodeAttribute()
            : BaseObject() {}

        /**
         * The destructor.
         */
        virtual ~NodeAttribute() {}
    };
} // namespace EMotionFX
