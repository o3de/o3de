/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/
// Original file Copyright Crytek GMBH or its affiliates, used under license.

#ifndef __animtime_h__
#define __animtime_h__

#include <IXml.h>
#include <AzCore/Casting/numeric_cast.h>
#include <Serialization/IArchive.h>

struct SAnimTime
{
    static const uint numTicksPerSecond = 6000;

    // List of possible frame rates (dividers of 6000). Most commonly used ones first.
    enum EFrameRate
    {
        // Common
        eFrameRate_30fps, eFrameRate_60fps, eFrameRate_120fps,

        // Possible
        eFrameRate_10fps, eFrameRate_12fps, eFrameRate_15fps, eFrameRate_24fps,
        eFrameRate_25fps, eFrameRate_40fps, eFrameRate_48fps, eFrameRate_50fps,
        eFrameRate_75fps, eFrameRate_80fps, eFrameRate_100fps, eFrameRate_125fps,
        eFrameRate_150fps, eFrameRate_200fps, eFrameRate_240fps, eFrameRate_250fps,
        eFrameRate_300fps, eFrameRate_375fps, eFrameRate_400fps, eFrameRate_500fps,
        eFrameRate_600fps, eFrameRate_750fps, eFrameRate_1000fps, eFrameRate_1200fps,
        eFrameRate_1500fps, eFrameRate_2000fps, eFrameRate_3000fps, eFrameRate_6000fps,

        eFrameRate_Num
    };

    SAnimTime()
        : m_ticks(0) {}
    explicit SAnimTime(int32 ticks)
        : m_ticks(ticks) {}
    explicit SAnimTime(float time)
        : m_ticks(aznumeric_caster(std::lround(static_cast<double>(time) * numTicksPerSecond))) {}

    static uint GetFrameRateValue(EFrameRate frameRate)
    {
        const uint frameRateValues[eFrameRate_Num] =
        {
            // Common
            30, 60, 120,

            // Possible
            10, 12, 15, 24, 25, 40, 48, 50, 75, 80, 100, 125,
            150, 200, 240, 250, 300, 375, 400, 500, 600, 750,
            1000, 1200, 1500, 2000, 3000, 6000
        };

        return frameRateValues[frameRate];
    }

    static const char* GetFrameRateName(EFrameRate frameRate)
    {
        const char* frameRateNames[eFrameRate_Num] =
        {
            // Common
            "30 fps", "60 fps", "120 fps",

            // Possible
            "10 fps", "12 fps", "15 fps", "24 fps",
            "25 fps", "40 fps", "48 fps", "50 fps",
            "75 fps", "80 fps", "100 fps", "125 fps",
            "150 fps", "200 fps", "240 fps", "250 fps",
            "300 fps", "375 fps", "400 fps", "500 fps",
            "600 fps", "750 fps", "1000 fps", "1200 fps",
            "1500 fps", "2000 fps", "3000 fps", "6000 fps"
        };

        return frameRateNames[frameRate];
    }

    float ToFloat() const { return static_cast<float>(m_ticks) / numTicksPerSecond; }

    void Serialize(Serialization::IArchive& ar)
    {
        ar(m_ticks, "ticks", "Ticks");
    }

    // Helper to serialize from ticks or old float time
    void Serialize(XmlNodeRef keyNode, bool bLoading, const char* pName, const char* pLegacyName)
    {
        if (bLoading)
        {
            int32 ticks;
            if (!keyNode->getAttr(pName, ticks))
            {
                // Backwards compatibility
                float time = 0.0f;
                keyNode->getAttr(pLegacyName, time);
                *this = SAnimTime(time);
            }
            else
            {
                m_ticks = ticks;
            }
        }
        else if (m_ticks > 0)
        {
            keyNode->setAttr(pName, m_ticks);
        }
    }

    int32 GetTicks() const { return m_ticks; }

    static SAnimTime Min() { SAnimTime minTime; minTime.m_ticks = std::numeric_limits<int32>::lowest(); return minTime; }
    static SAnimTime Max() { SAnimTime maxTime; maxTime.m_ticks = (std::numeric_limits<int32>::max)(); return maxTime; }

    SAnimTime operator-() const { return SAnimTime(-m_ticks); }
    SAnimTime operator-(SAnimTime r) const { SAnimTime temp = *this; temp.m_ticks -= r.m_ticks; return temp; }
    SAnimTime operator+(SAnimTime r) const { SAnimTime temp = *this; temp.m_ticks += r.m_ticks; return temp; }
    SAnimTime operator*(SAnimTime r) const { SAnimTime temp = *this; temp.m_ticks *= r.m_ticks; return temp; }
    SAnimTime operator/(SAnimTime r) const { SAnimTime temp; temp.m_ticks = static_cast<int32>((static_cast<int64>(m_ticks) * numTicksPerSecond) / r.m_ticks); return temp; }
    SAnimTime operator%(SAnimTime r) const { SAnimTime temp = *this; temp.m_ticks %= r.m_ticks; return temp; }
    SAnimTime operator*(float r) const { SAnimTime temp; temp.m_ticks = aznumeric_caster(std::lround(static_cast<double>(m_ticks) * r)); return temp; }
    SAnimTime operator/(float r) const { SAnimTime temp; temp.m_ticks = aznumeric_caster(std::lround(static_cast<double>(m_ticks) / r)); return temp; }
    SAnimTime& operator+=(SAnimTime r) { *this = *this + r; return *this; }
    SAnimTime& operator-=(SAnimTime r) { *this = *this - r; return *this; }
    SAnimTime& operator*=(SAnimTime r) { *this = *this * r; return *this; }
    SAnimTime& operator/=(SAnimTime r) { *this = *this / r; return *this; }
    SAnimTime& operator%=(SAnimTime r) { *this = *this % r; return *this; }
    SAnimTime& operator*=(float r) { *this = *this * r; return *this; }
    SAnimTime& operator/=(float r) { *this = *this / r; return *this; }

    bool operator<(SAnimTime r) const { return m_ticks < r.m_ticks; }
    bool operator<=(SAnimTime r) const { return m_ticks <= r.m_ticks; }
    bool operator>(SAnimTime r) const { return m_ticks > r.m_ticks; }
    bool operator>=(SAnimTime r) const { return m_ticks >= r.m_ticks; }
    bool operator==(SAnimTime r) const { return m_ticks == r.m_ticks; }
    bool operator!=(SAnimTime r) const { return m_ticks != r.m_ticks; }

    // Snap to nearest multiple of given frame rate
    SAnimTime SnapToNearest(const EFrameRate frameRate)
    {
        const int sign = sgn(m_ticks);
        const int32 absTicks = abs(m_ticks);

        const int framesMod = numTicksPerSecond / GetFrameRateValue(frameRate);
        const int32 remainder = absTicks % framesMod;
        const bool bNextMultiple = remainder >= (framesMod / 2);
        return SAnimTime(sign * ((absTicks - remainder) + (bNextMultiple ? framesMod : 0)));
    }

private:
    int32 m_ticks;

    friend bool Serialize(Serialization::IArchive& ar, SAnimTime& animTime, const char* name, const char* label);
};

inline bool Serialize(Serialization::IArchive& ar, SAnimTime& animTime, const char* name, const char* label)
{
    return ar(animTime.m_ticks, name, label);
}

inline SAnimTime abs(SAnimTime time)
{
    return (time >= SAnimTime(0)) ? time : -time;
}

#endif
