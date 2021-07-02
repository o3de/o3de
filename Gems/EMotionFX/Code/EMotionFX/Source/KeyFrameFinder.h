/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

// include required headers
#include "KeyFrame.h"


namespace EMotionFX
{
    /**
     * The keyframe finder.
     * This is used to quickly locate the two keys inside the keytrack to interpolate between.
     * For example if there is a keyframe at every second, and we want to calculate a value at
     * time 5.6, then we need to interpolate between the key with the time 5 and the key with the time 6.
     * This class basically uses a kd-tree to perform the search. It quickly culls out parts of the keys
     * which cannot be the one which we are searching for. This means the search times are linear, no matter
     * how many keys we are searching.
     */
    template <class ReturnType, class StorageType>
    class KeyFrameFinder
    {
    public:
        /**
         * The constructor.
         */
        KeyFrameFinder();

        /**
         * The destructor.
         */
        ~KeyFrameFinder();

        /**
         * Locates the key number to use for interpolation at a given time value.
         * You will interpolate between the returned keyframe number and the one after that.
         * @param timeValue The time value you want to calculate a value at.
         * @param keyTrack The keyframe array to perform the search on.
         * @param numKeys The number of keyframes stored inside the keyTrack parameter buffer.
         * @result The key number, or MCORE_INVALIDINDEX32 when no valid key could be found.
         */
        static uint32 FindKey(float timeValue, const KeyFrame<ReturnType, StorageType>* keyTrack, uint32 numKeys);
    };

    // include inline code
#include "KeyFrameFinder.inl"
} // namespace EMotionFX
