/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

// include the required headers
#include <AzCore/RTTI/TypeInfo.h>
#include <AzCore/Math/Quaternion.h>
#include "StandardHeaders.h"
#include "Algorithms.h"


namespace MCore
{
    /**
     * The compressed / packed quaternion class.
     * This represents a unit (normalized) quaternion on a packed way, which is 8 bytes per quaternion instead of 16 bytes
     * when using floating point quaternions. Of course we loose a bit of precision, but it isn't too bad for most things.
     * The class provides methods to convert from and to uncompressed quaternions.
     */
    template <class StorageType>
    class TCompressedQuaternion
    {
    public:
        /**
         * Default constructor.
         * This sets the quaternion to identity.
         */
        MCORE_INLINE TCompressedQuaternion();

        /**
         * Create a compressed quaternion from compressed x, y, z, w values.
         * @param xVal The value of x.
         * @param yVal The value of y.
         * @param zVal The value of z.
         * @param wVal The value of w.
         */
        MCORE_INLINE TCompressedQuaternion(float xVal, float yVal, float zVal, float wVal);

        /**
         * Create a compressed quaternion from an uncompressed one.
         * Please note that the uncompressed quaternion has to be normalized or a unit quaternion!
         * @param quat The normalized uncompressed quaternion.
         */
        MCORE_INLINE TCompressedQuaternion(const AZ::Quaternion& quat);

        /**
         * Update the compressed quaternion from an uncompressed one.
         * Please note that the uncompressed quaternion has to be normalized or a unit quaternion!
         * @param quat The normalized uncompressed quaternion.
         */
        MCORE_INLINE void FromQuaternion(const AZ::Quaternion& quat);

        /**
         * Uncompress the compressed quaternion into an uncompressed one.
         * @param output The output uncompressed quaternion to write the result in.
         */
        MCORE_INLINE void UnCompress(AZ::Quaternion* output) const;

        /**
         * Convert the compressed quaternion into an uncompressed one.
         * This method works the same as the UnCompress method, but it returns the result instead of specifying
         * the output quaternion.
         * @result The uncompressed version of this compressed quaternion.
         */
        MCORE_INLINE AZ::Quaternion ToQuaternion() const;

        AZ_INLINE operator AZ::Quaternion() const { return ToQuaternion(); }

    public:
        StorageType mX, mY, mZ, mW; /**< The compressed/packed quaternion components values. */

        // the number of steps within the specified range
        enum
        {
            CONVERT_VALUE = ((1 << (sizeof(StorageType) << 3)) >> 1) - 1
        };
    };

    // include the inline code
#include "CompressedQuaternion.inl"

    // declare standard types
    typedef TCompressedQuaternion<int16>    Compressed16BitQuaternion;
    typedef TCompressedQuaternion<int8>     Compressed8BitQuaternion;
} // namespace MCore

namespace AZ
{
    AZ_TYPE_INFO_TEMPLATE(MCore::TCompressedQuaternion, "{31AD5C7F-A999-40C5-AC3A-E13D150036E3}", AZ_TYPE_INFO_CLASS);
}
