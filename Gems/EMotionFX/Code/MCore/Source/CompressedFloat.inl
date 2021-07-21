/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

// constructor
template <class StorageType>
MCORE_INLINE TCompressedFloat<StorageType>::TCompressedFloat()
{
}


// constructor
template <class StorageType>
MCORE_INLINE TCompressedFloat<StorageType>::TCompressedFloat(float value, float minValue, float maxValue)
{
    // TODO: make sure due to rounding/floating point errors the result is not negative?
    const StorageType f = (1.0f / (maxValue - minValue)) * CONVERT_VALUE;
    mValue = (value - minValue) * f;
}


// constructor
template <class StorageType>
MCORE_INLINE TCompressedFloat<StorageType>::TCompressedFloat(StorageType value)
{
    mValue = value;
}


// create from a floating point value
template <class StorageType>
MCORE_INLINE void TCompressedFloat<StorageType>::FromFloat(float value, float minValue, float maxValue)
{
    // TODO: make sure due to rounding/floating point errors the result is not negative?
    const StorageType f = (StorageType)(1.0f / (maxValue - minValue)) * CONVERT_VALUE;
    mValue = (StorageType)((value - minValue) * f);
}


// uncompress into a floating point value
template <class StorageType>
MCORE_INLINE void TCompressedFloat<StorageType>::UnCompress(float* output, float minValue, float maxValue) const
{
    // unpack and normalize
    const float f = (1.0f / (float)CONVERT_VALUE) * (maxValue - minValue);
    *output = ((float)mValue * f) + minValue;
}


// convert to a floating point value
template <class StorageType>
MCORE_INLINE float TCompressedFloat<StorageType>::ToFloat(float minValue, float maxValue) const
{
    const float f = (1.0f / (float)CONVERT_VALUE) * (maxValue - minValue);
    return ((mValue * f) + minValue);
}
