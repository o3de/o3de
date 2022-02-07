/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

// include the required headers
#include <AzCore/RTTI/TypeInfo.h>
#include "StandardHeaders.h"
#include "Algorithms.h"


namespace MCore
{
    /**
     * The compressed / packed float class.
     * The class can automatically compresses a float of type 'float' into a float that
     * represents this value inside the 'StorageType' type. An example would be a floating point value
     * (float=float) compressed into a CompressedFloat that stores the value as 8-bit byte.
     * This would reduce the float size from 4 bytes into 1 byte, at the trade of a bit loss in precision and CPU overhead for
     * uncompression into a float again. When used with care, the CPU overhead and precision loss are negligible.
     * During both compression and uncompression you also have to specify the range (minimum and maximum possible values) of the
     * value you want to compress/decompress. If you are dealing with normalized values, the minimum value
     * would be 0, and the maximum value would be +1. When uncompressing (converting back to a float) you
     * have to be sure you use the same minimum and maximum values as when you used to compress it! This is very important!
     * The bigger the range, the more precision loss. There is however no performance impact linked to the range.
     */
    template <class StorageType>
    class TCompressedFloat
    {
    public:
        /**
         * Default constructor.
         * This leaves the member uninitialized, so if you get the uncompressed version, the result is unknown.
         */
        MCORE_INLINE TCompressedFloat();

        /**
         * Constructor. Create a compressed float from an uncompressed one.
         * @param value The floating point value you want to compress.
         * @param minValue The minimum possible value of the uncompressed floating point value.
         * @param maxValue The maximum possible value of the uncompressed floating point value.
         */
        MCORE_INLINE TCompressedFloat(float value, float minValue, float maxValue);

        /**
         * Constructor. Create a compressed float.
         * @param value The compessed floating point value.
         */
        MCORE_INLINE TCompressedFloat(StorageType value);

        /**
         * Create a compressed float from an uncompressed one.
         * @param value The floating point value you want to compress.
         * @param minValue The minimum possible value of the uncompressed floating point value.
         * @param maxValue The maximum possible value of the uncompressed floating point value.
         */
        MCORE_INLINE void FromFloat(float value, float minValue, float maxValue);

        /**
         * Uncompress this compressed floating point value into an uncompressed float.
         * Please note that the minimum and maximum values you specify are the same as when you created
         * this compressed floating point value or when you for the last time called the FromFloat method.
         * @param output The uncompressed floating point value to store the result in.
         * @param minValue The minimum possible value of the uncompressed floating point value.
         * @param maxValue The maximum possible value of the uncompressed floating point value.
         */
        MCORE_INLINE void UnCompress(float* output, float minValue, float maxValue) const;

        /**
         * Uncompress this compressed floating point value into an uncompressed float.
         * Please note that the minimum and maximum values you specify are the same as when you created
         * this compressed floating point value or when you for the last time called the FromFloat method.
         * @param minValue The minimum possible value of the uncompressed floating point value.
         * @param maxValue The maximum possible value of the uncompressed floating point value.
         * @result The uncompressed version of this floating point value.
         */
        MCORE_INLINE float ToFloat(float minValue, float maxValue) const;

    public:
        StorageType m_value; /**< The compressed/packed value. */

        // the number of steps within the specified range
        enum
        {
            CONVERT_VALUE = (1 << (sizeof(StorageType) << 3)) - 1
        };
    };

    // include the inline code
#include "CompressedFloat.inl"

    // declare standard types
    typedef TCompressedFloat<uint16>    Compressed16BitFloat;
    typedef TCompressedFloat<uint8>     Compressed8BitFloat;
} // namespace MCore

namespace AZ
{
    AZ_TYPE_INFO_TEMPLATE(MCore::TCompressedFloat, "{BFA1578B-66F8-4536-8CA6-FF5CC3E441AD}", AZ_TYPE_INFO_CLASS);
}
