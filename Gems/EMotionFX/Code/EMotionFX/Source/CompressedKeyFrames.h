/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

// include required files
#include "EMotionFXConfig.h"
#include <MCore/Source/CompressedQuaternion.h>
#include <MCore/Source/CompressedFloat.h>
#include "KeyFrame.h"


namespace EMotionFX
{
    //--------------------------------------------------------------------------------------
    // Partial Template Specialization for KeyFrames with the following template arguments:
    // ReturnType:  Quaternion
    // StorageType: Compressed8BitQuaternion
    //--------------------------------------------------------------------------------------
    // compress a quaternion
    template<>
    MCORE_INLINE void KeyFrame<AZ::Quaternion, MCore::Compressed8BitQuaternion>::SetValue(const AZ::Quaternion& value)                                      { mValue.FromQuaternion(value); }

    // decompress into a quaternion
    template<>
    MCORE_INLINE AZ::Quaternion KeyFrame<AZ::Quaternion, MCore::Compressed8BitQuaternion>::GetValue() const                                                 { return mValue.ToQuaternion(); }

    // decompress into a quaternion (without return value)
    template<>
    MCORE_INLINE void KeyFrame<AZ::Quaternion, MCore::Compressed8BitQuaternion>::GetValue(AZ::Quaternion* outValue)                                         { mValue.UnCompress(outValue); }

    // direct access to compressed values
    template<>
    MCORE_INLINE void KeyFrame<AZ::Quaternion, MCore::Compressed8BitQuaternion>::SetStorageTypeValue(const MCore::Compressed8BitQuaternion& value)          { mValue = value; }

    template<>
    MCORE_INLINE const MCore::Compressed8BitQuaternion& KeyFrame<AZ::Quaternion, MCore::Compressed8BitQuaternion>::GetStorageTypeValue() const              { return mValue; }


    //--------------------------------------------------------------------------------------
    // Partial Template Specialization for KeyFrames with the following template arguments:
    // ReturnType:  Quaternion
    // StorageType: Compressed16BitQuaternion
    //--------------------------------------------------------------------------------------
    // compress a quaternion
    template<>
    MCORE_INLINE void KeyFrame<AZ::Quaternion, MCore::Compressed16BitQuaternion>::SetValue(const AZ::Quaternion& value)                                     { mValue.FromQuaternion(value); }

    // decompress into a quaternion
    template<>
    MCORE_INLINE AZ::Quaternion KeyFrame<AZ::Quaternion, MCore::Compressed16BitQuaternion>::GetValue() const                                                { return mValue.ToQuaternion(); }

    // decompress into a quaternion
    template<>
    MCORE_INLINE void KeyFrame<AZ::Quaternion, MCore::Compressed16BitQuaternion>::GetValue(AZ::Quaternion* outValue)                                        { return mValue.UnCompress(outValue); }

    // direct access to compressed values
    template<>
    MCORE_INLINE void KeyFrame<AZ::Quaternion, MCore::Compressed16BitQuaternion>::SetStorageTypeValue(const MCore::Compressed16BitQuaternion& value)        { mValue = value; }

    template<>
    MCORE_INLINE const MCore::Compressed16BitQuaternion& KeyFrame<AZ::Quaternion, MCore::Compressed16BitQuaternion>::GetStorageTypeValue() const            { return mValue; }


    //--------------------------------------------------------------------------------------
    // Partial Template Specialization for KeyFrames with the following template arguments:
    // ReturnType:  float
    // StorageType: Compressed8BitFloat
    //--------------------------------------------------------------------------------------
    // compress a float
    template<>
    MCORE_INLINE void KeyFrame<float, MCore::Compressed8BitFloat>::SetValue(const float& value)                                                             { mValue.FromFloat(value, 0.0f, 1.0f); }

    // decompress into a float
    template<>
    MCORE_INLINE float KeyFrame<float, MCore::Compressed8BitFloat>::GetValue() const                                                                        { return mValue.ToFloat(0.0f, 1.0f); }

    // decompress into a float
    template<>
    MCORE_INLINE void KeyFrame<float, MCore::Compressed8BitFloat>::GetValue(float* outValue)                                                                { mValue.UnCompress(outValue, 0.0f, 1.0f); }

    // direct access to compressed values
    template<>
    MCORE_INLINE void KeyFrame<AZ::Quaternion, MCore::Compressed8BitFloat>::SetStorageTypeValue(const MCore::Compressed8BitFloat& value)                    { mValue = value; }

    template<>
    MCORE_INLINE const MCore::Compressed8BitFloat& KeyFrame<AZ::Quaternion, MCore::Compressed8BitFloat>::GetStorageTypeValue() const                        { return mValue; }


    //--------------------------------------------------------------------------------------
    // Partial Template Specialization for KeyFrames with the following template arguments:
    // ReturnType:  float
    // StorageType: Compressed16BitFloat
    //--------------------------------------------------------------------------------------
    // compress a float
    template<>
    MCORE_INLINE void KeyFrame<float, MCore::Compressed16BitFloat>::SetValue(const float& value)                                                            { mValue.FromFloat(value, 0.0f, 1.0f); }

    // decompress into a float
    template<>
    MCORE_INLINE float KeyFrame<float, MCore::Compressed16BitFloat>::GetValue() const                                                                       { return mValue.ToFloat(0.0f, 1.0f); }

    // decompress into a float
    template<>
    MCORE_INLINE void KeyFrame<float, MCore::Compressed16BitFloat>::GetValue(float* outValue)                                                               { return mValue.UnCompress(outValue, 0.0f, 1.0f); }

    // direct access to compressed values
    template<>
    MCORE_INLINE void KeyFrame<AZ::Quaternion, MCore::Compressed16BitFloat>::SetStorageTypeValue(const MCore::Compressed16BitFloat& value)                  { mValue = value; }

    template<>
    MCORE_INLINE const MCore::Compressed16BitFloat& KeyFrame<AZ::Quaternion, MCore::Compressed16BitFloat>::GetStorageTypeValue() const                      { return mValue; }
} // namespace EMotionFX

