/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

// constructor
template <class StorageType>
MCORE_INLINE TCompressedQuaternion<StorageType>::TCompressedQuaternion()
    : mX(0)
    , mY(0)
    , mZ(0)
    , mW(CONVERT_VALUE)
{
}


// constructor
template <class StorageType>
MCORE_INLINE TCompressedQuaternion<StorageType>::TCompressedQuaternion(float xVal, float yVal, float zVal, float wVal)
    : mX((StorageType)xVal)
    , mY((StorageType)yVal)
    , mZ((StorageType)zVal)
    , mW((StorageType)wVal)
{
}


// constructor
template <class StorageType>
MCORE_INLINE TCompressedQuaternion<StorageType>::TCompressedQuaternion(const AZ::Quaternion& quat)
    : mX((StorageType)(static_cast<float>(quat.GetX()) * CONVERT_VALUE))
    , mY((StorageType)(static_cast<float>(quat.GetY()) * CONVERT_VALUE))
    , mZ((StorageType)(static_cast<float>(quat.GetZ()) * CONVERT_VALUE))
    , mW((StorageType)(static_cast<float>(quat.GetW()) * CONVERT_VALUE))
{
}


// create from a quaternion
template <class StorageType>
MCORE_INLINE void TCompressedQuaternion<StorageType>::FromQuaternion(const AZ::Quaternion& quat)
{
    // pack it
    mX = (StorageType)(static_cast<float>(quat.GetX()) * CONVERT_VALUE);
    mY = (StorageType)(static_cast<float>(quat.GetY()) * CONVERT_VALUE);
    mZ = (StorageType)(static_cast<float>(quat.GetZ()) * CONVERT_VALUE);
    mW = (StorageType)(static_cast<float>(quat.GetW()) * CONVERT_VALUE);
}

// uncompress into a quaternion
template <class StorageType>
MCORE_INLINE void TCompressedQuaternion<StorageType>::UnCompress(AZ::Quaternion* output) const
{
    const float f = 1.0f / (float)CONVERT_VALUE;
    output->Set(mX * f, mY * f, mZ * f, mW * f);
}


// uncompress into a quaternion
template <>
MCORE_INLINE void TCompressedQuaternion<int16>::UnCompress(AZ::Quaternion* output) const
{
    output->Set(mX * 0.000030518509448f, mY * 0.000030518509448f, mZ * 0.000030518509448f, mW * 0.000030518509448f);
}


// convert to a quaternion
template <class StorageType>
MCORE_INLINE AZ::Quaternion TCompressedQuaternion<StorageType>::ToQuaternion() const
{
    const float f = 1.0f / (float)CONVERT_VALUE;
    return AZ::Quaternion(mX * f, mY * f, mZ * f, mW * f);
}


// convert to a quaternion
template <>
MCORE_INLINE AZ::Quaternion TCompressedQuaternion<int16>::ToQuaternion() const
{
    return AZ::Quaternion(mX * 0.000030518509448f, mY * 0.000030518509448f, mZ * 0.000030518509448f, mW * 0.000030518509448f);
}
