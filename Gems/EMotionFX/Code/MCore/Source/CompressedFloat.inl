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
