/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Memory/Memory.h>
#include <AzCore/RTTI/RTTI.h>
#include <EMotionFX/Source/EMotionFXConfig.h>
#include <MCore/Source/MemoryObject.h>


namespace EMotionFX
{
    /**
     * A reference counted base API object.
     * This interface provides a unified consistent way of destroying objects.
     * To delete an object make a call to the Destroy method, which is inherited from the MCore::MemoryObject class.
     */
    class EMFX_API BaseObject
        : public MCore::MemoryObject
    {
    public:
        AZ_RTTI(BaseObject, "{82AC952B-8F47-4929-BC59-6D453B482570}")

        /**
         * The constructor.
         */
        BaseObject();

        /**
         * The destructor, which does NOT automatically call Destroy.
         */
        virtual ~BaseObject();

    protected:
        /**
         * This will delete the actual object from memory.
         * Unlike Destroy, this really forces a delete on the object's memory, calling the destructor and releasing the allocated memory.
         * Basically it internally does a "delete this;".
         */
        void Delete() override;
    };
}   // namespace EMotionFX
