/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#pragma once

#include <AudioAllocators.h>

#include <AzCore/Console/ILogger.h>
#include <AzCore/Math/MathUtils.h>
#include <AzCore/Math/Random.h>
#include <AzCore/std/string/string_view.h>
#include <AzCore/std/typetraits/is_integral.h>
#include <AzCore/std/typetraits/is_unsigned.h>
#include <AzCore/std/time.h>

#define ATL_FLOAT_EPSILON (1.0e-6)


namespace Audio
{
    ///////////////////////////////////////////////////////////////////////////////////////////////////
    inline float frand_symm()
    {
        static AZ::SimpleLcgRandom rand(AZStd::GetTimeNowMicroSecond());
        // return random float [-1.f, 1.f)
        return rand.GetRandomFloat() * 2.f - 1.f;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    template<typename TMap, typename TKey>
    bool FindPlace(TMap& map, const TKey& key, typename TMap::iterator& iPlace)
    {
        iPlace = map.find(key);
        return (iPlace != map.end());
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    template<typename TMap, typename TKey>
    bool FindPlaceConst(const TMap& map, const TKey& key, typename TMap::const_iterator& iPlace)
    {
        iPlace = map.find(key);
        return (iPlace != map.end());
    }

#if !defined(AUDIO_RELEASE)
    bool AudioDebugDrawFilter(const AZStd::string_view objectName, const AZStd::string_view filter);
#endif // !AUDIO_RELEASE

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    template <typename ObjType, typename IDType = size_t>
    class CInstanceManager
    {
    public:
        ~CInstanceManager() {}

        using TPointerContainer = AZStd::vector<ObjType*, Audio::AudioSystemStdAllocator>;

        TPointerContainer m_cReserved;
        IDType m_nIDCounter;
        const size_t m_nReserveSize;
        const IDType m_nMinCounterValue;

        CInstanceManager(const size_t nReserveSize, const IDType nMinCounterValue)
            : m_nIDCounter(nMinCounterValue)
            , m_nReserveSize(nReserveSize)
            , m_nMinCounterValue(nMinCounterValue)
        {
            m_cReserved.reserve(m_nReserveSize);
        }

        IDType GetNextID()
        {
            if (m_nIDCounter >= m_nMinCounterValue)
            {
                return m_nIDCounter++;
            }
            else
            {
                AZLOG_WARN("InstanceManager ID counter wrapped around");
                m_nIDCounter = m_nMinCounterValue;
                return m_nIDCounter;
            }
        }
    };

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    class CSmoothFloat
    {
    public:
        explicit CSmoothFloat(const float smoothFactor, const float precision, const float initialValue = 0.0f)
            : m_value(initialValue)
            , m_target(initialValue)
            , m_isActive(false)
            , m_smoothFactor(AZ::GetAbs(smoothFactor))
            , m_precision(AZ::GetAbs(precision))
        {}

        ~CSmoothFloat() {}

        void Update(const float smoothFactorOverride = -1.f)
        {
            if (m_isActive)
            {
                if (AZ::GetAbs(m_target - m_value) > m_precision)
                {
                    // still haven't reached the target within the specified precision, smooth towards it.
                    const float smoothFactor = (smoothFactorOverride < 0.f) ? m_smoothFactor : smoothFactorOverride;
                    m_value += (m_target - m_value) / (smoothFactor * smoothFactor + 1.f);
                }
                else
                {
                    // reached the target (within precision) this update, disable smoothing until a new target is set.
                    m_value = m_target;
                    m_isActive = false;
                }
            }
        }

        float GetCurrent() const
        {
            return m_value;
        }

        void SetNewTarget(const float newTarget, const bool reset = false)
        {
            if (reset)
            {
                Reset(newTarget);
            }
            else if (AZ::GetAbs(newTarget - m_target) > m_precision)
            {
                m_target = newTarget;
                m_isActive = true;
            }
        }

        void Reset(const float initialValue = 0.0f)
        {
            m_value = m_target = initialValue;
            m_isActive = false;
        }

    private:
        float m_value;
        float m_target;
        bool m_isActive;
        const float m_smoothFactor;
        const float m_precision;
    };

    /*!
     * Flags
     * Used for storing, checking, setting, and clearing related bits together.
     */
    template<typename StoredType,
        typename = AZStd::enable_if_t<AZStd::is_integral<StoredType>::value
                                      && AZStd::is_unsigned<StoredType>::value>>
    class Flags
    {
    public:
        Flags(const StoredType flags = 0)
            : m_storedFlags(flags)
        {}

        void AddFlags(const StoredType flags)
        {
            m_storedFlags |= flags;
        }

        void ClearFlags(const StoredType flags)
        {
            m_storedFlags &= ~flags;
        }

        bool AreAllFlagsActive(const StoredType flags) const
        {
            return (m_storedFlags & flags) == flags;
        }

        bool AreAnyFlagsActive(const StoredType flags) const
        {
            return (m_storedFlags & flags) != 0;
        }

        bool AreMultipleFlagsActive() const
        {
            return (m_storedFlags & (m_storedFlags - 1)) != 0;
        }

        bool IsOneFlagActive() const
        {
            return m_storedFlags != 0 && !AreMultipleFlagsActive();
        }

        void ClearAllFlags()
        {
            m_storedFlags = 0;
        }

        void SetFlags(StoredType flags, const bool enable)
        {
            enable ? AddFlags(flags) : ClearFlags(flags);
        }

        StoredType GetRawFlags() const
        {
            return m_storedFlags;
        }

        bool operator==(const Flags& other) const
        {
            return m_storedFlags == other.m_storedFlags;
        }

        bool operator!=(const Flags& other) const
        {
            return m_storedFlags != other.m_storedFlags;
        }

    private:
        StoredType m_storedFlags = 0;
    };

} // namespace Audio
