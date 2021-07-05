/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

// default constructor
template <class ReturnType, class StorageType>
KeyTrackLinearDynamic<ReturnType, StorageType>::KeyTrackLinearDynamic()
{
}


// extended constructor
template <class ReturnType, class StorageType>
KeyTrackLinearDynamic<ReturnType, StorageType>::KeyTrackLinearDynamic(uint32 nrKeys)
{
    SetNumKeys(nrKeys);
}


// destructor
template <class ReturnType, class StorageType>
KeyTrackLinearDynamic<ReturnType, StorageType>::~KeyTrackLinearDynamic()
{
    ClearKeys();
}

template <class ReturnType, class StorageType>
void KeyTrackLinearDynamic<ReturnType, StorageType>::Reflect(AZ::ReflectContext* context)
{
    AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
    if (!serializeContext)
    {
        return;
    }

    serializeContext->Class<KeyTrackLinearDynamic<ReturnType, StorageType>>()
        ->Version(1)
        ->Field("keyValues", &KeyTrackLinearDynamic<ReturnType, StorageType>::mKeys)
        ;
}

// clear all keys
template <class ReturnType, class StorageType>
void KeyTrackLinearDynamic<ReturnType, StorageType>::ClearKeys()
{
    mKeys.clear();
}


// init the keytrack
template <class ReturnType, class StorageType>
void KeyTrackLinearDynamic<ReturnType, StorageType>::Init()
{
    // check all key time values, so we are sure the first key start at time 0
    if (mKeys.empty())
    {
        return;
    }

    // get the time value of the first key, which is our minimum time
    const float minTime = mKeys[0].GetTime();

    // if it's not equal to zero, we have to correct it (and all other keys as well)
    if (minTime > 0.0f)
    {
        const size_t numKeys = mKeys.size();
        for (uint32 i = 0; i < numKeys; ++i)
        {
            mKeys[i].SetTime(mKeys[i].GetTime() - minTime);
        }
    }
}


template <class ReturnType, class StorageType>
MCORE_INLINE KeyFrame<ReturnType, StorageType>* KeyTrackLinearDynamic<ReturnType, StorageType>::GetKey(uint32 nr)
{
    MCORE_ASSERT(nr < mKeys.size());
    return &mKeys[nr];
}


template <class ReturnType, class StorageType>
MCORE_INLINE KeyFrame<ReturnType, StorageType>* KeyTrackLinearDynamic<ReturnType, StorageType>::GetFirstKey()
{
    return (mKeys.size() > 0) ? &mKeys[0] : nullptr;
}


template <class ReturnType, class StorageType>
MCORE_INLINE KeyFrame<ReturnType, StorageType>* KeyTrackLinearDynamic<ReturnType, StorageType>::GetLastKey()
{
    return (mKeys.size() > 0) ? &mKeys.back() : nullptr;
}



template <class ReturnType, class StorageType>
MCORE_INLINE const KeyFrame<ReturnType, StorageType>* KeyTrackLinearDynamic<ReturnType, StorageType>::GetKey(uint32 nr) const
{
    MCORE_ASSERT(nr < mKeys.size());
    return &mKeys[nr];
}


template <class ReturnType, class StorageType>
MCORE_INLINE const KeyFrame<ReturnType, StorageType>* KeyTrackLinearDynamic<ReturnType, StorageType>::GetFirstKey() const
{
    return (mKeys.size() > 0) ? &mKeys[0] : nullptr;
}


template <class ReturnType, class StorageType>
MCORE_INLINE const KeyFrame<ReturnType, StorageType>* KeyTrackLinearDynamic<ReturnType, StorageType>::GetLastKey() const
{
    return (mKeys.size() > 0) ? &mKeys.back() : nullptr;
}


template <class ReturnType, class StorageType>
MCORE_INLINE float KeyTrackLinearDynamic<ReturnType, StorageType>::GetFirstTime() const
{
    const KeyFrame<ReturnType, StorageType>* key = GetFirstKey();
    return (key) ? key->GetTime() : 0.0f;
}


template <class ReturnType, class StorageType>
MCORE_INLINE float KeyTrackLinearDynamic<ReturnType, StorageType>::GetLastTime() const
{
    const KeyFrame<ReturnType, StorageType>* key = GetLastKey();
    return (key) ? key->GetTime() : 0.0f;
}


template <class ReturnType, class StorageType>
MCORE_INLINE uint32 KeyTrackLinearDynamic<ReturnType, StorageType>::GetNumKeys() const
{
    return static_cast<uint32>(mKeys.size());
}


template <class ReturnType, class StorageType>
MCORE_INLINE void KeyTrackLinearDynamic<ReturnType, StorageType>::AddKey(float time, const ReturnType& value, bool smartPreAlloc)
{
    #ifdef MCORE_DEBUG
    if (mKeys.size() > 0)
    {
        MCORE_ASSERT(time >= mKeys.back().GetTime());
    }
    #endif

    // if we need to prealloc
    if (mKeys.capacity() == mKeys.size() && smartPreAlloc == true)
    {
        const size_t numToReserve = mKeys.size() / 4;
        mKeys.reserve(mKeys.capacity() + numToReserve);
    }

    // not the first key, so add on the end
    mKeys.emplace_back(KeyFrame<ReturnType, StorageType>(time, value));
}


// find a key at a given time
template <class ReturnType, class StorageType>
MCORE_INLINE uint32 KeyTrackLinearDynamic<ReturnType, StorageType>::FindKeyNumber(float curTime) const
{
    return KeyFrameFinder<ReturnType, StorageType>::FindKey(curTime, &mKeys.front(), mKeys.size());
}


// find a key at a given time
template <class ReturnType, class StorageType>
MCORE_INLINE KeyFrame<ReturnType, StorageType>* KeyTrackLinearDynamic<ReturnType, StorageType>::FindKey(float curTime)  const
{
    // find the key number
    const uint32 keyNumber = KeyFrameFinder<ReturnType, StorageType>::FindKey(curTime, &mKeys.front(), mKeys.size());

    // if no key was found
    return (keyNumber != MCORE_INVALIDINDEX32) ? &mKeys[keyNumber] : nullptr;
}


// returns the interpolated value at a given time
template <class ReturnType, class StorageType>
ReturnType KeyTrackLinearDynamic<ReturnType, StorageType>::GetValueAtTime(float currentTime, uint32* cachedKey, uint8* outWasCacheHit, bool interpolate) const
{
    MCORE_ASSERT(currentTime >= 0.0);
    MCORE_ASSERT(mKeys.size() > 0);

    // make a local copy of the cached key value
    uint32 localCachedKey = (cachedKey) ? *cachedKey : MCORE_INVALIDINDEX32;

    // find the first key to start interpolating from (between this one and the next)
    uint32 keyNumber = MCORE_INVALIDINDEX32;

    // prevent searching in the set of keyframes when a cached key is available
    // of course we need to check first if the cached key is actually still valid or not
    if (localCachedKey == MCORE_INVALIDINDEX32) // no cached key has been set, so simply perform a search
    {
        if (outWasCacheHit)
        {
            *outWasCacheHit = 0;
        }

        keyNumber = KeyFrameFinder<ReturnType, StorageType>::FindKey(currentTime, &mKeys.front(), static_cast<uint32>(mKeys.size()));

        if (cachedKey)
        {
            *cachedKey = keyNumber;
        }
    }
    else
    {
        // make sure we dont go out of bounds when checking
        if (localCachedKey >= mKeys.size() - 2)
        {
            if (mKeys.size() > 2)
            {
                localCachedKey = static_cast<uint32>(mKeys.size()) - 3;
            }
            else
            {
                localCachedKey = 0;
            }
        }

        // check if the cached key is still valid (cache hit)
        if ((mKeys[localCachedKey].GetTime() <= currentTime) && (mKeys[localCachedKey + 1].GetTime() >= currentTime))
        {
            keyNumber = localCachedKey;
            if (outWasCacheHit)
            {
                *outWasCacheHit = 1;
            }
        }
        else
        {
            if (localCachedKey < mKeys.size() - 2 && (mKeys[localCachedKey + 1].GetTime() <= currentTime) && (mKeys[localCachedKey + 2].GetTime() >= currentTime))
            {
                if (outWasCacheHit)
                {
                    *outWasCacheHit = 1;
                }

                keyNumber = localCachedKey + 1;
            }
            else    // the cached key is invalid, so perform a real search (cache miss)
            {
                if (outWasCacheHit)
                {
                    *outWasCacheHit = 0;
                }

                keyNumber = KeyFrameFinder<ReturnType, StorageType>::FindKey(currentTime, &mKeys.front(), static_cast<uint32>(mKeys.size()));

                if (cachedKey)
                {
                    *cachedKey = keyNumber;
                }
            }
        }
    }

    // if no key could be found
    if (keyNumber == MCORE_INVALIDINDEX32)
    {
        // if there are no keys at all, simply return an empty object
        if (mKeys.size() == 0)
        {
            // return an empty object
            return ReturnType();
        }

        // return the last key
        return mKeys.back().GetValue();
    }

    // check if we didn't reach the end of the track
    if ((keyNumber + 1) > (mKeys.size() - 1))
    {
        return mKeys.back().GetValue();
    }

    // perform interpolation
    if (interpolate)
    {
        return Interpolate(keyNumber, currentTime);
    }
    else
    {
        return mKeys[keyNumber].GetValue();
    }
}


// perform interpolation
template <class ReturnType, class StorageType>
MCORE_INLINE ReturnType KeyTrackLinearDynamic<ReturnType, StorageType>::Interpolate(uint32 startKey, float currentTime) const
{
    // get the keys to interpolate between
    const KeyFrame<ReturnType, StorageType>& firstKey = mKeys[startKey];
    const KeyFrame<ReturnType, StorageType>& nextKey  = mKeys[startKey + 1];

    // calculate the time value in range of [0..1]
    const float t = (currentTime - firstKey.GetTime()) / (nextKey.GetTime() - firstKey.GetTime());

    // perform the interpolation
    return MCore::LinearInterpolate<ReturnType>(firstKey.GetValue(), nextKey.GetValue(), t);
}



template <>
MCORE_INLINE AZ::Quaternion KeyTrackLinearDynamic<AZ::Quaternion, MCore::Compressed16BitQuaternion>::Interpolate(uint32 startKey, float currentTime) const
{
    // get the keys to interpolate between
    const KeyFrame<AZ::Quaternion, MCore::Compressed16BitQuaternion>& firstKey = mKeys[startKey];
    const KeyFrame<AZ::Quaternion, MCore::Compressed16BitQuaternion>& nextKey  = mKeys[startKey + 1];

    // calculate the time value in range of [0..1]
    const float t = (currentTime - firstKey.GetTime()) / (nextKey.GetTime() - firstKey.GetTime());

    // lerp between them
    return MCore::NLerp(firstKey.GetValue(), nextKey.GetValue(), t);
}


template <>
MCORE_INLINE AZ::Quaternion KeyTrackLinearDynamic<AZ::Quaternion, AZ::Quaternion>::Interpolate(uint32 startKey, float currentTime) const
{
    // get the keys to interpolate between
    const KeyFrame<AZ::Quaternion, AZ::Quaternion>& firstKey = mKeys[startKey];
    const KeyFrame<AZ::Quaternion, AZ::Quaternion>& nextKey  = mKeys[startKey + 1];

    // calculate the time value in range of [0..1]
    const float t = (currentTime - firstKey.GetTime()) / (nextKey.GetTime() - firstKey.GetTime());

    // lerp between them
    return firstKey.GetValue().NLerp(nextKey.GetValue(), t);
}


// adds a keyframe with automatic sorting support
template <class ReturnType, class StorageType>
void KeyTrackLinearDynamic<ReturnType, StorageType>::AddKeySorted(float time, const ReturnType& value, bool smartPreAlloc)
{
    // if we need to prealloc
    if (smartPreAlloc)
    {
        if (mKeys.capacity() == mKeys.size())
        {
            const uint32 numToReserve = mKeys.size() / 4;
            mKeys.reserve(mKeys.capacity() + numToReserve);
        }
    }

    // if there are no keys yet, add it
    if (mKeys.empty())
    {
        mKeys.emplace_back(KeyFrame<ReturnType, StorageType>(time, value));
        return;
    }

    // get the keyframe time
    const float keyTime = time;

    // if we must add it at the end
    if (keyTime >= mKeys.back().GetTime())
    {
        mKeys.emplace_back(KeyFrame<ReturnType, StorageType>(time, value));
        return;
    }

    // if we have to add it in the front
    if (keyTime < mKeys.front().GetTime())
    {
        mKeys.insert(mKeys.begin(), KeyFrame<ReturnType, StorageType>(time, value));
        return;
    }

    // quickly find the location to insert, and insert it
    const uint32 place = KeyFrameFinder<ReturnType, StorageType>::FindKey(keyTime, &mKeys.front(), mKeys.size());
    mKeys.insert(mKeys.begin() + place + 1, KeyFrame<ReturnType, StorageType>(time, value));
}


template <class ReturnType, class StorageType>
MCORE_INLINE void KeyTrackLinearDynamic<ReturnType, StorageType>::RemoveKey(uint32 keyNr)
{
    mKeys.erase(AZStd::next(mKeys.begin(), keyNr));
}


template <class ReturnType, class StorageType>
void KeyTrackLinearDynamic<ReturnType, StorageType>::MakeLoopable(float fadeTime)
{
    MCORE_ASSERT(fadeTime > 0);

    if (mKeys.empty())
    {
        return;
    }

    const float lastTime = GetLastKey()->GetTime();
    ReturnType firstValue = GetFirstKey()->GetValue();
    AddKey(lastTime + fadeTime, firstValue);

    // re-init the track
    Init();
}


// optimize the keytrack
template <class ReturnType, class StorageType>
uint32 KeyTrackLinearDynamic<ReturnType, StorageType>::Optimize(float maxError)
{
    // if there aren't at least two keys, return, because we never remove the first and last key frames
    // and we'd need at least two keyframes to interpolate between
    if (mKeys.size() <= 2)
    {
        return 0;
    }

    // create a temparory copy of the keytrack data we're going to optimize
    KeyTrackLinearDynamic<ReturnType, StorageType> keyTrackCopy;
    keyTrackCopy.mKeys          = mKeys;
    keyTrackCopy.Init();

    // while we want to continue optimizing
    uint32 i = 1;
    uint32 numRemoved = 0;  // the number of removed keys
    do
    {
        // get the time of the current keyframe (starting from the second towards the last one)
        const float time = mKeys[i].GetTime();

        // remove the keyframe and reinit the keytrack (and interpolator's tangents etc)
        keyTrackCopy.RemoveKey(i);
        keyTrackCopy.Init();

        // get the interpolated value at the keyframe's time of the keytrack BEFORE we removed the keyframe
        ReturnType v1 = GetValueAtTime(time);

        // and get the interpolated value of the keytrack AFTER we removed the key
        ReturnType v2 = keyTrackCopy.GetValueAtTime(time);

        // if the points are close, so if the "visual" difference of removing the key is within our threshold
        // we can remove the key for real
        if (MCore::Compare<ReturnType>::CheckIfIsClose(v1, v2, maxError))
        {
            RemoveKey(i);   // remove the key for real
            Init();         // reinit the keytrack
            numRemoved++;
        }
        else    // if the "visual" difference is too high and we do not want ot remove the key, copy over the original keys again to restore it
        {
            keyTrackCopy.mKeys = mKeys;     // copy the keyframe array
            keyTrackCopy.Init();            // reinit the keytrack
            i++;                            // go to the next keyframe, and try ot remove that one
        }
    } while (i < mKeys.size() - 1);    // while we haven't reached the last keyframe (minus one)

    //mKeys.shrink_to_fit();
    return numRemoved;
}


// pre-alloc keys
template <class ReturnType, class StorageType>
void KeyTrackLinearDynamic<ReturnType, StorageType>::SetNumKeys(uint32 numKeys)
{
    // resize the array of keys
    mKeys.resize(numKeys);
}


// set a given key
template <class ReturnType, class StorageType>
MCORE_INLINE void KeyTrackLinearDynamic<ReturnType, StorageType>::SetKey(uint32 keyNr, float time, const ReturnType& value)
{
    // adjust the value and time of the key
    mKeys[keyNr].SetValue(value);
    mKeys[keyNr].SetTime(time);
}


// set a given key
template <class ReturnType, class StorageType>
MCORE_INLINE void KeyTrackLinearDynamic<ReturnType, StorageType>::SetStorageTypeKey(uint32 keyNr, float time, const StorageType& value)
{
    // adjust the value and time of the key
    mKeys[keyNr].SetStorageTypeValue(value);
    mKeys[keyNr].SetTime(time);
}


// check if the keytrack is animated
template <class ReturnType, class StorageType>
MCORE_INLINE bool KeyTrackLinearDynamic<ReturnType, StorageType>::CheckIfIsAnimated(const ReturnType& initialPose, float maxError) const
{
    // empty keytracks are never animated
    if (mKeys.size() == 0)
    {
        return false;
    }

    // get the number of keyframes and iterate through them
    const uint32 numKeyFrames = GetNumKeys();
    for (uint32 i = 0; i < numKeyFrames; ++i)
    {
        // if the sampled value is not within the given maximum distance/error of the initial pose, it means we have an animated track
        if (MCore::Compare<ReturnType>::CheckIfIsClose(initialPose, GetKey(i)->GetValue(), maxError) == false)
        {
            return true;
        }
    }

    return false;
}



// reserve memory for keys
template <class ReturnType, class StorageType>
MCORE_INLINE void KeyTrackLinearDynamic<ReturnType, StorageType>::Reserve(uint32 numKeys)
{
    mKeys.reserve(numKeys);
}


// calculate memory usage
template <class ReturnType, class StorageType>
uint32 KeyTrackLinearDynamic<ReturnType, StorageType>::CalcMemoryUsage(bool includeMembers) const
{
    return 0;
}


// minimize memory usage
template <class ReturnType, class StorageType>
void KeyTrackLinearDynamic<ReturnType, StorageType>::Shrink()
{
    mKeys.shrink_to_fit();
}
