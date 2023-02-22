/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/RTTI/ReflectContext.h>
#include <AzCore/std/string/string.h>
#include <EMotionFX/Source/Allocators.h>


namespace EMotionFX
{
    /**
     * Object ID type.
     * IDs are used to uniquely identify objects.
     */
    class ObjectId
    {
    public:
        AZ_TYPE_INFO(ObjectId, "{B7DCAC0C-0F48-4350-B169-0387C2602328}")
        AZ_CLASS_ALLOCATOR(ObjectId, EMotionFX::AnimGraphAllocator)

        static const ObjectId InvalidId;

        ObjectId();
        ObjectId(const ObjectId& id);
        ObjectId(AZ::u64 id);

        /**
         * Generate a random and valid id.
         * @result A randomized, valid id.
         */
        static ObjectId Create();

        /**
         * Convert the given string to an id.
         * @param[in] text The string to parse and convert.
         * @result The resulting id. The id will be invalid in case the text could not be parsed correctly.
         */
        static ObjectId CreateFromString(const AZStd::string& text);

        /**
         * Casts the id to u64. Needed for reflection.
         * @result The id as an 64-bit unsigned int.
         */
        operator AZ::u64() const
        {
            return m_id;
        }

        /**
         * Convert the id to a string and return it.
         */
        AZStd::string ToString() const;

        /**
         * Determines whether this id is valid.
         * An id is invalid if you did not provide an argument to the constructor.
         * @result Returns true if the id is valid. Otherwise, false.
         */
        bool IsValid() const;

        /**
         * Set the id to the invalid value.
         */
        void SetInvalid();

        /**
         * Compare two ids for equality.
         * @param rhs Id to compare against.
         * @return True if the ids are equal. Otherwise, false.
         */
        bool operator==(const ObjectId& rhs) const;

        /**
         * Compare two ids for inequality.
         * @param rhs Id to compare against.
         * @return True if the ids are different. Otherwise, false.
         */
        bool operator!=(const ObjectId& rhs) const;

        static void Reflect(AZ::ReflectContext* context);

    protected:
        AZ::u64 m_id;
    };
} // namespace EMotionFX
