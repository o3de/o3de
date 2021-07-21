/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

// constructor
template <class StorageType>
MCORE_INLINE TCompressedVector3<StorageType>::TCompressedVector3()
{
}


// constructor
template <class StorageType>
MCORE_INLINE TCompressedVector3<StorageType>::TCompressedVector3(const AZ::Vector3& vec, float minValue, float maxValue)
{
    // TODO: make sure due to rounding/floating point errors the result is not negative?
    const float f = static_cast<float>(CONVERT_VALUE) / (maxValue - minValue);
    mX = static_cast<StorageType>((vec.GetX() - minValue) * f);
    mY = static_cast<StorageType>((vec.GetY() - minValue) * f);
    mZ = static_cast<StorageType>((vec.GetZ() - minValue) * f);
}


// create from a Vector3
template <class StorageType>
MCORE_INLINE void TCompressedVector3<StorageType>::FromVector3(const AZ::Vector3& vec, float minValue, float maxValue)
{
    // TODO: make sure due to rounding/floating point errors the result is not negative?
    const float f = static_cast<float>(CONVERT_VALUE) / (maxValue - minValue);
    mX = static_cast<StorageType>((vec.GetX() - minValue) * f);
    mY = static_cast<StorageType>((vec.GetY() - minValue) * f);
    mZ = static_cast<StorageType>((vec.GetZ() - minValue) * f);
}


// convert to a Vector3
template <class StorageType>
MCORE_INLINE AZ::Vector3 TCompressedVector3<StorageType>::ToVector3(float minValue, float maxValue) const
{
    const float f = (maxValue - minValue) / static_cast<float>(CONVERT_VALUE);
    return AZ::Vector3(static_cast<float>(mX) * f + minValue, static_cast<float>(mY) * f + minValue, static_cast<float>(mZ) * f + minValue);
}

