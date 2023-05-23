/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <AzCore/Serialization/SerializeContext.h>

namespace EMotionFX
{
    // extended constructor
    template <class ReturnType, class StorageType>
    KeyTrackLinearDynamic<ReturnType, StorageType>::KeyTrackLinearDynamic(size_t nrKeys)
    {
        SetNumKeys(nrKeys);
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
            ->Field("keyValues", &KeyTrackLinearDynamic<ReturnType, StorageType>::m_keys)
            ;
    }

    // clear all keys
    template <class ReturnType, class StorageType>
    void KeyTrackLinearDynamic<ReturnType, StorageType>::ClearKeys()
    {
        m_keys.clear();
    }


    // init the keytrack
    template <class ReturnType, class StorageType>
    void KeyTrackLinearDynamic<ReturnType, StorageType>::Init()
    {
        // check all key time values, so we are sure the first key start at time 0
        if (m_keys.empty())
        {
            return;
        }

        // get the time value of the first key, which is our minimum time
        const float minTime = m_keys[0].GetTime();

        // if it's not equal to zero, we have to correct it (and all other keys as well)
        if (minTime > 0.0f)
        {
            for (KeyFrame<ReturnType, StorageType>& key : m_keys)
            {
                key.SetTime(key.GetTime() - minTime);
            }
        }
    }


    template <class ReturnType, class StorageType>
    MCORE_INLINE KeyFrame<ReturnType, StorageType>* KeyTrackLinearDynamic<ReturnType, StorageType>::GetKey(size_t nr)
    {
        MCORE_ASSERT(nr < m_keys.size());
        return &m_keys[nr];
    }


    template <class ReturnType, class StorageType>
    MCORE_INLINE KeyFrame<ReturnType, StorageType>* KeyTrackLinearDynamic<ReturnType, StorageType>::GetFirstKey()
    {
        return !m_keys.empty() ? &m_keys[0] : nullptr;
    }


    template <class ReturnType, class StorageType>
    MCORE_INLINE KeyFrame<ReturnType, StorageType>* KeyTrackLinearDynamic<ReturnType, StorageType>::GetLastKey()
    {
        return !m_keys.empty() ? &m_keys.back() : nullptr;
    }



    template <class ReturnType, class StorageType>
    MCORE_INLINE const KeyFrame<ReturnType, StorageType>* KeyTrackLinearDynamic<ReturnType, StorageType>::GetKey(size_t nr) const
    {
        MCORE_ASSERT(nr < m_keys.size());
        return &m_keys[nr];
    }


    template <class ReturnType, class StorageType>
    MCORE_INLINE const KeyFrame<ReturnType, StorageType>* KeyTrackLinearDynamic<ReturnType, StorageType>::GetFirstKey() const
    {
        return !m_keys.empty() ? &m_keys[0] : nullptr;
    }


    template <class ReturnType, class StorageType>
    MCORE_INLINE const KeyFrame<ReturnType, StorageType>* KeyTrackLinearDynamic<ReturnType, StorageType>::GetLastKey() const
    {
        return !m_keys.empty() ? &m_keys.back() : nullptr;
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
    MCORE_INLINE size_t KeyTrackLinearDynamic<ReturnType, StorageType>::GetNumKeys() const
    {
        return m_keys.size();
    }


    template <class ReturnType, class StorageType>
    MCORE_INLINE void KeyTrackLinearDynamic<ReturnType, StorageType>::AddKey(float time, const ReturnType& value, bool smartPreAlloc)
    {
#ifdef MCORE_DEBUG
        if (!m_keys.empty())
        {
            MCORE_ASSERT(time >= m_keys.back().GetTime());
        }
#endif

        // if we need to prealloc
        if (m_keys.capacity() == m_keys.size() && smartPreAlloc == true)
        {
            const size_t numToReserve = m_keys.size() / 4;
            m_keys.reserve(m_keys.capacity() + numToReserve);
        }

        // not the first key, so add on the end
        m_keys.emplace_back(KeyFrame<ReturnType, StorageType>(time, value));
    }


    // find a key at a given time
    template <class ReturnType, class StorageType>
    MCORE_INLINE size_t KeyTrackLinearDynamic<ReturnType, StorageType>::FindKeyNumber(float curTime) const
    {
        return KeyFrameFinder<ReturnType, StorageType>::FindKey(curTime, &m_keys.front(), static_cast<uint32>(m_keys.size()));
    }


    // find a key at a given time
    template <class ReturnType, class StorageType>
    MCORE_INLINE KeyFrame<ReturnType, StorageType>* KeyTrackLinearDynamic<ReturnType, StorageType>::FindKey(float curTime)  const
    {
        // find the key number
        const size_t keyNumber = KeyFrameFinder<ReturnType, StorageType>::FindKey(curTime, &m_keys.front(), m_keys.size());

        // if no key was found
        return (keyNumber != InvalidIndex) ? &m_keys[keyNumber] : nullptr;
    }


    // returns the interpolated value at a given time
    template <class ReturnType, class StorageType>
    ReturnType KeyTrackLinearDynamic<ReturnType, StorageType>::GetValueAtTime(float currentTime, size_t* cachedKey, uint8* outWasCacheHit, bool interpolate) const
    {
        MCORE_ASSERT(currentTime >= 0.0);
        MCORE_ASSERT(!m_keys.empty());

        // make a local copy of the cached key value
        size_t localCachedKey = (cachedKey) ? *cachedKey : InvalidIndex;

        // find the first key to start interpolating from (between this one and the next)
        size_t keyNumber = InvalidIndex;

        // prevent searching in the set of keyframes when a cached key is available
        // of course we need to check first if the cached key is actually still valid or not
        if (localCachedKey == InvalidIndex) // no cached key has been set, so simply perform a search
        {
            if (outWasCacheHit)
            {
                *outWasCacheHit = 0;
            }

            keyNumber = KeyFrameFinder<ReturnType, StorageType>::FindKey(currentTime, &m_keys.front(), m_keys.size());

            if (cachedKey)
            {
                *cachedKey = keyNumber;
            }
        }
        else
        {
            // make sure we dont go out of bounds when checking
            if (localCachedKey >= m_keys.size() - 2)
            {
                if (m_keys.size() > 2)
                {
                    localCachedKey = m_keys.size() - 3;
                }
                else
                {
                    localCachedKey = 0;
                }
            }

            // check if the cached key is still valid (cache hit)
            if ((m_keys[localCachedKey].GetTime() <= currentTime) && (m_keys[localCachedKey + 1].GetTime() >= currentTime))
            {
                keyNumber = localCachedKey;
                if (outWasCacheHit)
                {
                    *outWasCacheHit = 1;
                }
            }
            else
            {
                if (localCachedKey < m_keys.size() - 2 && (m_keys[localCachedKey + 1].GetTime() <= currentTime) && (m_keys[localCachedKey + 2].GetTime() >= currentTime))
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

                    keyNumber = KeyFrameFinder<ReturnType, StorageType>::FindKey(currentTime, &m_keys.front(), m_keys.size());

                    if (cachedKey)
                    {
                        *cachedKey = keyNumber;
                    }
                }
            }
        }

        // if no key could be found
        if (keyNumber == InvalidIndex)
        {
            // if there are no keys at all, simply return an empty object
            if (m_keys.size() == 0)
            {
                // return an empty object
                return ReturnType();
            }

            // return the last key
            return m_keys.back().GetValue();
        }

        // check if we didn't reach the end of the track
        if ((keyNumber + 1) > (m_keys.size() - 1))
        {
            return m_keys.back().GetValue();
        }

        // perform interpolation
        if (interpolate)
        {
            return Interpolate(keyNumber, currentTime);
        }
        else
        {
            return m_keys[keyNumber].GetValue();
        }
    }


    // perform interpolation
    template <class ReturnType, class StorageType>
    MCORE_INLINE ReturnType KeyTrackLinearDynamic<ReturnType, StorageType>::Interpolate(size_t startKey, float currentTime) const
    {
        // get the keys to interpolate between
        const KeyFrame<ReturnType, StorageType>& firstKey = m_keys[startKey];
        const KeyFrame<ReturnType, StorageType>& nextKey = m_keys[startKey + 1];

        // calculate the time value in range of [0..1]
        const float t = (currentTime - firstKey.GetTime()) / (nextKey.GetTime() - firstKey.GetTime());

        // perform the interpolation
        return AZ::Lerp<ReturnType>(firstKey.GetValue(), nextKey.GetValue(), t);
    }



    template <>
    MCORE_INLINE AZ::Quaternion KeyTrackLinearDynamic<AZ::Quaternion, MCore::Compressed16BitQuaternion>::Interpolate(size_t startKey, float currentTime) const
    {
        // get the keys to interpolate between
        const KeyFrame<AZ::Quaternion, MCore::Compressed16BitQuaternion>& firstKey = m_keys[startKey];
        const KeyFrame<AZ::Quaternion, MCore::Compressed16BitQuaternion>& nextKey = m_keys[startKey + 1];

        // calculate the time value in range of [0..1]
        const float t = (currentTime - firstKey.GetTime()) / (nextKey.GetTime() - firstKey.GetTime());

        // lerp between them
        return MCore::NLerp(firstKey.GetValue(), nextKey.GetValue(), t);
    }


    template <>
    MCORE_INLINE AZ::Quaternion KeyTrackLinearDynamic<AZ::Quaternion, AZ::Quaternion>::Interpolate(size_t startKey, float currentTime) const
    {
        // get the keys to interpolate between
        const KeyFrame<AZ::Quaternion, AZ::Quaternion>& firstKey = m_keys[startKey];
        const KeyFrame<AZ::Quaternion, AZ::Quaternion>& nextKey = m_keys[startKey + 1];

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
            if (m_keys.capacity() == m_keys.size())
            {
                const size_t numToReserve = m_keys.size() / 4;
                m_keys.reserve(m_keys.capacity() + numToReserve);
            }
        }

        // if there are no keys yet, add it
        if (m_keys.empty())
        {
            m_keys.emplace_back(KeyFrame<ReturnType, StorageType>(time, value));
            return;
        }

        // get the keyframe time
        const float keyTime = time;

        // if we must add it at the end
        if (keyTime >= m_keys.back().GetTime())
        {
            m_keys.emplace_back(KeyFrame<ReturnType, StorageType>(time, value));
            return;
        }

        // if we have to add it in the front
        if (keyTime < m_keys.front().GetTime())
        {
            m_keys.insert(m_keys.begin(), KeyFrame<ReturnType, StorageType>(time, value));
            return;
        }

        // quickly find the location to insert, and insert it
        const size_t place = KeyFrameFinder<ReturnType, StorageType>::FindKey(keyTime, &m_keys.front(), m_keys.size());
        m_keys.insert(m_keys.begin() + place + 1, KeyFrame<ReturnType, StorageType>(time, value));
    }


    template <class ReturnType, class StorageType>
    MCORE_INLINE void KeyTrackLinearDynamic<ReturnType, StorageType>::RemoveKey(size_t keyNr)
    {
        m_keys.erase(AZStd::next(m_keys.begin(), keyNr));
    }


    template <class ReturnType, class StorageType>
    void KeyTrackLinearDynamic<ReturnType, StorageType>::MakeLoopable(float fadeTime)
    {
        MCORE_ASSERT(fadeTime > 0);

        if (m_keys.empty())
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
    size_t KeyTrackLinearDynamic<ReturnType, StorageType>::Optimize(float maxError)
    {
        // if there aren't at least two keys, return, because we never remove the first and last key frames
        // and we'd need at least two keyframes to interpolate between
        if (m_keys.size() <= 2)
        {
            return 0;
        }

        // create a temparory copy of the keytrack data we're going to optimize
        KeyTrackLinearDynamic<ReturnType, StorageType> keyTrackCopy;
        keyTrackCopy.m_keys = m_keys;
        keyTrackCopy.Init();

        // while we want to continue optimizing
        size_t i = 1;
        size_t numRemoved = 0;  // the number of removed keys
        do
        {
            // get the time of the current keyframe (starting from the second towards the last one)
            const float time = m_keys[i].GetTime();

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
                keyTrackCopy.m_keys = m_keys;     // copy the keyframe array
                keyTrackCopy.Init();            // reinit the keytrack
                i++;                            // go to the next keyframe, and try ot remove that one
            }
        } while (i < m_keys.size() - 1);    // while we haven't reached the last keyframe (minus one)

        return numRemoved;
    }


    // pre-alloc keys
    template <class ReturnType, class StorageType>
    void KeyTrackLinearDynamic<ReturnType, StorageType>::SetNumKeys(size_t numKeys)
    {
        // resize the array of keys
        m_keys.resize(numKeys);
    }


    // set a given key
    template <class ReturnType, class StorageType>
    MCORE_INLINE void KeyTrackLinearDynamic<ReturnType, StorageType>::SetKey(size_t keyNr, float time, const ReturnType& value)
    {
        // adjust the value and time of the key
        m_keys[keyNr].SetValue(value);
        m_keys[keyNr].SetTime(time);
    }


    // set a given key
    template <class ReturnType, class StorageType>
    MCORE_INLINE void KeyTrackLinearDynamic<ReturnType, StorageType>::SetStorageTypeKey(size_t keyNr, float time, const StorageType& value)
    {
        // adjust the value and time of the key
        m_keys[keyNr].SetStorageTypeValue(value);
        m_keys[keyNr].SetTime(time);
    }


    // check if the keytrack is animated
    template <class ReturnType, class StorageType>
    MCORE_INLINE bool KeyTrackLinearDynamic<ReturnType, StorageType>::CheckIfIsAnimated(const ReturnType& initialPose, float maxError) const
    {
        return !m_keys.empty() && AZStd::any_of(begin(m_keys), end(m_keys), [&initialPose, maxError](const auto& key)
            {
                return !MCore::Compare<ReturnType>::CheckIfIsClose(initialPose, key.GetValue(), maxError);
            });
    }



    // reserve memory for keys
    template <class ReturnType, class StorageType>
    MCORE_INLINE void KeyTrackLinearDynamic<ReturnType, StorageType>::Reserve(size_t numKeys)
    {
        m_keys.reserve(numKeys);
    }


    // calculate memory usage
    template <class ReturnType, class StorageType>
    size_t KeyTrackLinearDynamic<ReturnType, StorageType>::CalcMemoryUsage([[maybe_unused]] bool includeMembers) const
    {
        return 0;
    }


    // minimize memory usage
    template <class ReturnType, class StorageType>
    void KeyTrackLinearDynamic<ReturnType, StorageType>::Shrink()
    {
        m_keys.shrink_to_fit();
    }
}
