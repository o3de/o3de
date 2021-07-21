/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

// the constructor
template <class ReturnType, class StorageType>
KeyFrameFinder<ReturnType, StorageType>::KeyFrameFinder()
{
}



// the destructor
template <class ReturnType, class StorageType>
KeyFrameFinder<ReturnType, StorageType>::~KeyFrameFinder()
{
}


// returns the keyframe number to use for interpolation
template <class ReturnType, class StorageType>
uint32 KeyFrameFinder<ReturnType, StorageType>::FindKey(float timeValue, const KeyFrame<ReturnType, StorageType>* keyTrack, uint32 numKeys)
{
    // if we haven't got any keys, return MCORE_INVALIDINDEX32, which means no key found
    if (numKeys == 0)
    {
        return MCORE_INVALIDINDEX32;
    }

    uint32 low      = 0;
    uint32 high     = numKeys - 1;
    float lowValue  = keyTrack[low].GetTime();
    float highValue = keyTrack[high].GetTime();

    // these can go if you're sure the value is going to be valid (between the min and max key's values)
    if (timeValue < lowValue || timeValue >= highValue)
    {
        return MCORE_INVALIDINDEX32;
    }

    for (;; )
    {
        // calculate the interpolated index
        const uint32 mid = low + (int)((timeValue - lowValue) / (highValue - lowValue) * (high - low));

        if (keyTrack[mid].GetTime() <= timeValue)
        {
            const float tempLow = keyTrack[mid + 1].GetTime();
            if (tempLow > timeValue)
            {
                return mid;
            }

            low         = mid + 1;
            lowValue    = tempLow;
        }
        else // if ( myArray [ mid ] > value )
        {
            const float tempHigh = keyTrack[mid - 1].GetTime();
            if (tempHigh <= timeValue)
            {
                return mid - 1;
            }

            high        = mid - 1;
            highValue   = tempHigh;
        }
    }
}
