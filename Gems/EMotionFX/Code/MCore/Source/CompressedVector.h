/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

// include the required headers
#include "StandardHeaders.h"
#include "Vector.h"
#include "Algorithms.h"


namespace MCore
{
    /**
     * The compressed / packed vector class.
     * The class can automatically compress a vector which has components of type 'float' into a vector that
     * represents these component values inside 'StorageType' component types. An example would be a Vector3 object
     * of floats (float=float) compressed into a CompressedVector3 that stores the xyz components as 16-byte unsigned shorts.
     * This would reduce the Vector3 size from 12 bytes into 6 bytes, at the trade of a bit loss in precision and CPU overhead for
     * uncompression into a Vector3 of floats again. When used with care, the CPU overhead and precision loss are negligible.
     * During both compression and uncompression you also have to specify the range (minimum and maximum possible values) of the
     * components of the vector you want to compress/decompress. If you are dealing with normalized normals, the minimum value
     * would be -1, and the maximum value would be +1. When uncompressing (converting back to a Vector3 of floats) you
     * have to be sure you use the same minimum and maximum values as when you used to compress it! This is very important!
     * The bigger the range, the more precision loss. There is however no performance impact linked to the range.
     */
    template <class StorageType>
    class TCompressedVector3
    {
    public:
        /**
         * Default constructor.
         * This leaves the members uninitialized, so if you get the uncompressed version, the result is unknown.
         */
        MCORE_INLINE TCompressedVector3();

        /**
         * The constructor that sets the compressed values directly.
         * @param x The compressed x component.
         * @param y The compressed y component.
         * @param z The compressed z component.
         */
        MCORE_INLINE TCompressedVector3(StorageType x, StorageType y, StorageType z)
            : m_x(x)
            , m_y(y)
            , m_z(z) {}

        /**
         * Create a compressed vector from an uncompressed one.
         * @param vec The vector you want to compress.
         * @param minValue The minimum possible value of the xyz components of the uncompressed vector. So in case of a normalized normal, this would be -1.
         * @param maxValue The maximum possible value of the xyz components of the uncompressed vector. So in case of a normalized normal, this would be +1.
         */
        MCORE_INLINE TCompressedVector3(const AZ::Vector3& vec, float minValue, float maxValue);

        /**
         * Create a compressed vector from an uncompressed one.
         * @param vec The vector you want to compress.
         * @param minValue The minimum possible value of the xyz components of the uncompressed vector. So in case of a normalized normal, this would be -1.
         * @param maxValue The maximum possible value of the xyz components of the uncompressed vector. So in case of a normalized normal, this would be +1.
         */
        MCORE_INLINE void FromVector3(const AZ::Vector3& vec, float minValue, float maxValue);

        /**
         * Uncompress this compressed vector into an uncompressed Vector3 of floats.
         * Please note that the minimum and maximum values you specify are the same as when you created this compressed vector3 or when you for the last
         * time called the FromVector3 method.
         * @param minValue The minimum possible value of the xyz components of the uncompressed vector. So in case of a normalized normal, this would be -1.
         * @param maxValue The maximum possible value of the xyz components of the uncompressed vector. So in case of a normalized normal, this would be +1.
         * @result The uncompressed version of this vector.
         */
        MCORE_INLINE AZ::Vector3 ToVector3(float minValue, float maxValue) const;


    public:
        StorageType m_x, m_y, m_z; /**< The compressed/packed vector components. */

        // the number of steps within the specified range
        enum
        {
            CONVERT_VALUE = (1 << (sizeof(StorageType) << 3)) - 1
        };
    };

    // include the inline code
#include "CompressedVector.inl"

    // declare standard types
    typedef TCompressedVector3<uint16>  Compressed16BitVector3;
    typedef TCompressedVector3<uint8>   Compressed8BitVector3;
} // namespace MCore
