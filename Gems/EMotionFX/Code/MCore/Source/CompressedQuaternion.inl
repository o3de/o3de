/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

// constructor
template <class StorageType>
MCORE_INLINE TCompressedQuaternion<StorageType>::TCompressedQuaternion()
    : m_x(0)
    , m_y(0)
    , m_z(0)
    , m_w(CONVERT_VALUE)
{
}


// constructor
template <class StorageType>
MCORE_INLINE TCompressedQuaternion<StorageType>::TCompressedQuaternion(float xVal, float yVal, float zVal, float wVal)
    : m_x((StorageType)xVal)
    , m_y((StorageType)yVal)
    , m_z((StorageType)zVal)
    , m_w((StorageType)wVal)
{
}


// constructor
template <class StorageType>
MCORE_INLINE TCompressedQuaternion<StorageType>::TCompressedQuaternion(const AZ::Quaternion& quat)
    : m_x((StorageType)(static_cast<float>(quat.GetX()) * CONVERT_VALUE))
    , m_y((StorageType)(static_cast<float>(quat.GetY()) * CONVERT_VALUE))
    , m_z((StorageType)(static_cast<float>(quat.GetZ()) * CONVERT_VALUE))
    , m_w((StorageType)(static_cast<float>(quat.GetW()) * CONVERT_VALUE))
{
}


// create from a quaternion
template <class StorageType>
MCORE_INLINE void TCompressedQuaternion<StorageType>::FromQuaternion(const AZ::Quaternion& quat)
{
    // pack it
    m_x = (StorageType)(static_cast<float>(quat.GetX()) * CONVERT_VALUE);
    m_y = (StorageType)(static_cast<float>(quat.GetY()) * CONVERT_VALUE);
    m_z = (StorageType)(static_cast<float>(quat.GetZ()) * CONVERT_VALUE);
    m_w = (StorageType)(static_cast<float>(quat.GetW()) * CONVERT_VALUE);
}

// uncompress into a quaternion
template <class StorageType>
MCORE_INLINE void TCompressedQuaternion<StorageType>::UnCompress(AZ::Quaternion* output) const
{
    const float f = 1.0f / (float)CONVERT_VALUE;
    output->Set(m_x * f, m_y * f, m_z * f, m_w * f);
}


// uncompress into a quaternion
template <>
MCORE_INLINE void TCompressedQuaternion<int16>::UnCompress(AZ::Quaternion* output) const
{
    output->Set(m_x * 0.000030518509448f, m_y * 0.000030518509448f, m_z * 0.000030518509448f, m_w * 0.000030518509448f);
}


// convert to a quaternion
template <class StorageType>
MCORE_INLINE AZ::Quaternion TCompressedQuaternion<StorageType>::ToQuaternion() const
{
    const float f = 1.0f / (float)CONVERT_VALUE;
    return AZ::Quaternion(m_x * f, m_y * f, m_z * f, m_w * f);
}


// convert to a quaternion
template <>
MCORE_INLINE AZ::Quaternion TCompressedQuaternion<int16>::ToQuaternion() const
{
    return AZ::Quaternion(m_x * 0.000030518509448f, m_y * 0.000030518509448f, m_z * 0.000030518509448f, m_w * 0.000030518509448f);
}
