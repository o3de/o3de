/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#ifndef GM_REPLICA_THROTTLING_H
#define GM_REPLICA_THROTTLING_H


#include <AzCore/Math/Quaternion.h>
#include <AzCore/Math/Vector2.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/Math/Vector4.h>

namespace GridMate
{
    //-----------------------------------------------------------------------------
    // BasicThrottle
    // Returns prev == cur
    // Uses operator==
    //-----------------------------------------------------------------------------
    template<typename DataType>
    class BasicThrottle
    {
    public:
        AZ_TYPE_INFO_LEGACY(BasicThrottle, "{394E41BF-D60C-4917-8810-251E2E3D3EAF}", DataType);
        
        bool WithinThreshold(const DataType& newValue) const
        {
            return newValue == m_baseline;
        }

        void UpdateBaseline(const DataType& baseline)
        {
            m_baseline = baseline;
        }
   
    private:
        DataType m_baseline;
    };

    template<>
    class BasicThrottle<AZ::Quaternion>
    {
    public:
        AZ_TYPE_INFO_LEGACY(BasicThrottle, "{6795C9AD-412F-4164-B025-0CBA82194A44}", AZ::Quaternion);
        
        bool WithinThreshold(const AZ::Quaternion& newValue) const
        {
            return newValue.IsClose(m_baseline);
        }

        void UpdateBaseline(const AZ::Quaternion& baseline)
        {
            m_baseline = baseline;
        }

    private:
        AZ::Quaternion m_baseline;
    };
    //-----------------------------------------------------------------------------

    //-----------------------------------------------------------------------------
    // EpsilonThrottle
    // Returns (second - first)^2 < e^2;
    //-----------------------------------------------------------------------------
    template<typename T>
    class EpsilonThrottle
    {
        T m_epsilon2;

    public:
        EpsilonThrottle()
            : m_epsilon2(0) { }

        bool WithinThreshold(const T& newValue) const
        {
            T diff = m_baseline - newValue;
            return diff * diff < m_epsilon2;
        }

        void SetBaseline(const T& baseline)
        {
            m_baseline = baseline;
        }
        
        void UpdateBaseline(const T& baseline)
        {
            m_baseline = baseline;
        }
        
        void SetThreshold(const T& e)
        {
            m_epsilon2 = e * e;
        }
        
    private:
        T m_baseline;
            
    };
    
    template<>
    class EpsilonThrottle<AZ::Vector2>
    {
        AZ::Vector2 m_epsilon2;
        
    public:
        EpsilonThrottle()
                : m_epsilon2(0) { }
                
        bool WithinThreshold(const AZ::Vector2& newValue) const
        {
            const float sqDiff = m_baseline.GetDistanceSq(newValue);
            return sqDiff < m_epsilon2.GetLength();
        }

        void SetBaseline(const AZ::Vector2& baseline)
        {
            m_baseline = baseline;
        }
                
        void UpdateBaseline(const AZ::Vector2& baseline)
        {
            m_baseline = baseline;
        }
                
        void SetThreshold(const AZ::Vector2& e)
        {
            m_epsilon2 = e * e;
        }
                
    private:
        AZ::Vector2 m_baseline;
    };
    
    template<>
    class EpsilonThrottle<AZ::Vector3>
    {
        AZ::Vector3 m_epsilon2;
        
    public:
        EpsilonThrottle()
                : m_epsilon2(0) { }
                
        bool WithinThreshold(const AZ::Vector3& newValue) const
        {
            const float sqDiff = (m_baseline - newValue).GetLengthSq();
            return sqDiff < m_epsilon2.GetLength();
        }

                
        void SetBaseline(const AZ::Vector3& baseline)
        {
            m_baseline = baseline;
        }
                
        void UpdateBaseline(const AZ::Vector3& baseline)
        {
                m_baseline = baseline;
        }
                
        void SetThreshold(const AZ::Vector3& e)
        {
            m_epsilon2 = e * e;
        }
                
    private:
        AZ::Vector3 m_baseline;
    };
    
    template<>
    class EpsilonThrottle<AZ::Vector4>
    {
        AZ::Vector4 m_epsilon2;
        
    public:
        EpsilonThrottle()
                : m_epsilon2(0) { }
                
        bool WithinThreshold(const AZ::Vector4& newValue) const
        {
            const float sqDiff = (m_baseline - newValue).GetLengthSq();
            return sqDiff < m_epsilon2.GetLength();
        }

        void SetBaseline(const AZ::Vector4& baseline)
        {
            m_baseline = baseline;
        }

        void UpdateBaseline(const AZ::Vector4& baseline)
        {
            m_baseline = baseline;
        }

        void SetThreshold(const AZ::Vector4& e)
        {
            m_epsilon2 = e * e;
        }

    private:
        AZ::Vector4 m_baseline;
    };
    //-----------------------------------------------------------------------------
} // namespace GridMate

#endif // GM_REPLICA_THROTTLING_H
