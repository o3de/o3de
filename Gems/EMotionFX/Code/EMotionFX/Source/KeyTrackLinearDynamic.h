/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

// include required headers
#include <AzCore/RTTI/TypeInfo.h>
#include <AzCore/Math/MathUtils.h>
#include <AzCore/std/containers/vector.h>
#include <MCore/Source/StandardHeaders.h>
#include <MCore/Source/Compare.h>

#include "EMotionFXConfig.h"
#include "KeyFrame.h"
#include "CompressedKeyFrames.h"
#include "KeyFrameFinder.h"



namespace AZ
{
    class ReflectContext;
}

namespace EMotionFX
{
    /**
     * The dynamic linear keyframe track.
     * The difference between the standard and dynamic one is that the dynamic one can reserve memory and grow its contents more efficiently, with far less reallocations.
     * For that reason the dynamic version is more efficient for adding and removing keys dynamically.
     * This is a class holding a set of keyframes.
     */
    template <class ReturnType, class StorageType = ReturnType>
    class KeyTrackLinearDynamic
    {
        MCORE_MEMORYOBJECTCATEGORY(KeyTrackLinearDynamic, EMFX_DEFAULT_ALIGNMENT, EMFX_MEMCATEGORY_MOTIONS_KEYTRACKS);

    public:
        AZ_TYPE_INFO_WITH_NAME(KeyTrackLinearDynamic, "EMotionFX::KeyTrackLinear", "{8C6EB52A-9720-467B-9D96-B4B967A113D1}", StorageType);

        KeyTrackLinearDynamic() = default;

        /**
         * @param nrKeys The number of keyframes which the keytrack contains (preallocates this amount of keyframes).
         */
        KeyTrackLinearDynamic(size_t nrKeys);

        static void Reflect(AZ::ReflectContext* context);

        /**
         * Reserve space for a given number of keys. This pre-allocates data, so that adding keys doesn't always do a reallocation.
         * @param numKeys The number of keys to reserve space for. This is the absolute number of keys, NOT the number to reserve extra.
         */
        void Reserve(size_t numKeys);

        /**
         * Calculate the memory usage, in bytes.
         * @param includeMembers Specifies whether to include member variables of the keytrack class itself or not (default=true).
         * @result The number of bytes used by this keytrack.
         */
        size_t CalcMemoryUsage(bool includeMembers = true) const;

        /**
         * Initialize all keyframes in this keytrack.
         * Call this after all keys are added and setup, otherwise the interpolation won't work and nothing will happen.
         */
        void Init();

        /**
         * Shrink the memory usage of this track to as small as possible.
         * This removes any possible pre-allocated data for the array.
         */
        void Shrink();

        /**
         * Perform interpolation between two keyframes.
         * @param startKey The first keyframe index. The interpolation will take place between this keyframe and the one after this one.
         * @param currentTime The global time, in seconds. This time value has to be between the time value of the startKey and the one after that.
         * @result The interpolated value.
         */
        MCORE_INLINE ReturnType Interpolate(size_t startKey, float currentTime) const;

        /**
         * Add a key to the track (at the back).
         * @param time The time of the keyframe, in seconds.
         * @param value The value of the keyframe.
         * @param smartPreAlloc Set to true if you wish to automatically pre-allocate space for multiple keys if we run out of space. This can make adding more keys faster.
         */
        MCORE_INLINE void AddKey(float time, const ReturnType& value, bool smartPreAlloc = true);

        /**
         * Adds a key to the track, and automatically detects where to place it.
         * NOTE: you will have to call the Init() method again when you finished adding keys.
         * @param time The time value of the keyframe, in seconds.
         * @param value The value of the keyframe at that time.
         * @param smartPreAlloc Set to true if you wish to automatically pre-allocate space for multiple keys if we run out of space. This can make adding more keys faster.
         */
        MCORE_INLINE void AddKeySorted(float time, const ReturnType& value, bool smartPreAlloc = true);

        /**
         * Remove a given keyframe, by number.
         * Do not forget that you have to re-initialize the keytrack after you have removed one or more
         * keyframes. The reason for this is that the possible tangents inside the interpolators have to be
         * recalculated when the key structure has changed.
         * @param keyNr The keyframe number, must be in range of [0..GetNumKeys()-1].
         */
        MCORE_INLINE void RemoveKey(size_t keyNr);

        /**
         * Clear all keys.
         */
        void ClearKeys();

        /**
         * Check if a given keytrack is animated or not.
         * @param initialPose The base or initial pose to compare to. If the keyframe values are different from
         *                    this value, the keytrack is considered to be animated.
         * @param maxError The maximum error/difference between the specified initial pose and the keyframes.
         * @result True in case the keytrack is animated, otherwise false is returned.
         */
        MCORE_INLINE bool CheckIfIsAnimated(const ReturnType& initialPose, float maxError) const;

        /**
         * Return the interpolated value at the specified time.
         * @param currentTime The time in seconds.
         * @param cachedKey The keyframe number that should first be checked before finding the keyframes to
         *                  interpolate between using the keyframe finder. If this value is nullptr (default), this cached key is ignored.
         *                  The cached value will also be overwritten with the new cached key in case of a cache miss.
         * @param outWasCacheHit This output value will contain 0 when this method had an internal cache miss and a value of 1 in case it was a cache hit.
         * @param interpolate Should we interpolate between the keyframes?
         * @result Returns the value at the specified time.
         */
        ReturnType GetValueAtTime(float currentTime, size_t* cachedKey = nullptr, uint8* outWasCacheHit = nullptr, bool interpolate = true) const;

        /**
         * Get a given keyframe.
         * @param nr The index, so the keyframe number.
         * @result A pointer to the keyframe.
         */
        MCORE_INLINE KeyFrame<ReturnType, StorageType>* GetKey(size_t nr);

        /**
         * Returns the first keyframe.
         * @result A pointer to the first keyframe.
         */
        MCORE_INLINE KeyFrame<ReturnType, StorageType>* GetFirstKey();

        /**
         * Returns the last keyframe.
         * @result A pointer to the last keyframe.
         */
        MCORE_INLINE KeyFrame<ReturnType, StorageType>* GetLastKey();

        /**
         * Get a given keyframe.
         * @param nr The index, so the keyframe number.
         * @result A pointer to the keyframe.
         */
        MCORE_INLINE const KeyFrame<ReturnType, StorageType>* GetKey(size_t nr) const;

        /**
         * Returns the first keyframe.
         * @result A pointer to the first keyframe.
         */
        MCORE_INLINE const KeyFrame<ReturnType, StorageType>* GetFirstKey() const;

        /**
         * Returns the last keyframe.
         * @result A pointer to the last keyframe.
         */
        MCORE_INLINE const KeyFrame<ReturnType, StorageType>* GetLastKey() const;


        /**
         * Returns the time value of the first keyframe.
         * @result The time value of the first keyframe, in seconds.
         */
        MCORE_INLINE float GetFirstTime() const;

        /**
         * Return the time value of the last keyframe.
         * @result The time value of the last keyframe, in seconds.
         */
        MCORE_INLINE float GetLastTime() const;

        /**
         * Returns the number of keyframes in this track.
         * @result The number of currently stored keyframes.
         */
        MCORE_INLINE size_t GetNumKeys() const;

        /**
         * Find a key at a given time.
         * @param curTime The time of the key.
         * @return A pointer to the keyframe.
         */
        MCORE_INLINE KeyFrame<ReturnType, StorageType>* FindKey(float curTime) const;

        /**
         * Find a key number at a given time.
         * You will need to interpolate between this and the next key.
         * @param curTime The time to retreive the key for.
         * @result Returns the key number or MCORE_INVALIDINDEX32 when not found.
         */
        MCORE_INLINE size_t FindKeyNumber(float curTime) const;

        /**
         * Make the keytrack loopable, by adding a new keyframe at the end of the keytrack.
         * This added keyframe will have the same value as the first keyframe.
         * @param fadeTime The relative offset after the last keyframe. If this value is 0.5, it means it will add
         *                 a keyframe half a second after the last keyframe currently in the keytrack.
         */
        void MakeLoopable(float fadeTime = 0.3f);

        /**
         * Optimize the keytrack by removing redundant frames.
         * The way this is done is by comparing differences between the resulting curves when removing specific keyframes.
         * If the error (difference) between those curve before and after keyframe removal is within a given maximum error value, the keyframe can be
         * safely removed since there will not be much "visual" difference.
         * The metric of this maximum error depends on the ReturnType of the keytrack (so the return type values of the keyframes).
         * For vectors this will be the squared length between those two vectors.
         * Partial template specialization needs to be used to add support for optimizing different types, such as quaternions.
         * You can do this by creating a specialization of the MCore::Compare<ReturnType>::CheckIfIsClose(...) method. You can find it in the
         * MCore project in the file Compare.h. We provide implementations for several different types, such as vectors, quaternions and floats.
         * @param maxError The maximum allowed error value. The higher you set this value, the more keyframes will be removed.
         * @result The method returns the number of removed keyframes.
         */
        size_t Optimize(float maxError);

        /**
         * Pre-allocate a given number of keys.
         * Please keep in mind that any existing keys will remain unchanged.
         * However, newly created keys will be uninitialized.
         * @param numKeys The number of keys to allocate.
         */
        void SetNumKeys(size_t numKeys);

        /**
         * Set the value of a key.
         * Please note that you have to make sure your keys remain in sorted order! (sorted on time value).
         * @param keyNr The keyframe number.
         * @param time The time value, in seconds.
         * @param value The value of the key.
         */
        MCORE_INLINE void SetKey(size_t keyNr, float time, const ReturnType& value);

        /**
         * Set the storage type value of a key.
         * Please note that you have to make sure your keys remain in sorted order! (sorted on time value).
         * @param keyNr The keyframe number.
         * @param time The time value, in seconds.
         * @param value The storage type value of the key.
         */
        MCORE_INLINE void SetStorageTypeKey(size_t keyNr, float time, const StorageType& value);

    protected:
        AZStd::vector<KeyFrame<ReturnType, StorageType>> m_keys;          /**< The collection of keys which form the track. */
    };


    // include keytrack inline code
} // namespace EMotionFX

#include "KeyTrackLinearDynamic.inl"
