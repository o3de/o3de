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

#pragma once

// include the required headers
#include <AzCore/std/string/string.h>
#include "StandardHeaders.h"
#include "MemoryManager.h"

namespace MCore
{
    /**
     * The distance class, which can be used to convert between different unit types.
     * A unit type being for example centimeters, inches, meters, etc.
     * You can use the ConvertTo() and ConvertedTo() methods or the CalcNumCentimeters() and similar methods or the CalcDistanceInUnitType() function to get a conversion.
     */
    class MCORE_API Distance
    {
    public:
        // a unit type
        enum EUnitType : uint8
        {
            UNITTYPE_INCHES             = 0,
            UNITTYPE_FEET               = 1,
            UNITTYPE_YARDS              = 2,
            UNITTYPE_MILES              = 3,
            UNITTYPE_MILLIMETERS        = 4,
            UNITTYPE_CENTIMETERS        = 5,
            UNITTYPE_DECIMETERS         = 6,
            UNITTYPE_METERS             = 7,
            UNITTYPE_KILOMETERS         = 8
        };

        MCORE_INLINE Distance()
            : mDistance(0.0)
            , mDistanceMeters(0.0)
            , mUnitType(UNITTYPE_METERS)      { }
        MCORE_INLINE Distance(double units, EUnitType unitType)
            : mDistance(units)
            , mDistanceMeters(0.0)
            , mUnitType(unitType)           { UpdateDistanceMeters(); }
        MCORE_INLINE Distance(float units, EUnitType unitType)
            : mDistance(units)
            , mDistanceMeters(0.0)
            , mUnitType(unitType)           { UpdateDistanceMeters(); }
        MCORE_INLINE Distance(const Distance& other)
            : mDistance(other.mDistance)
            , mDistanceMeters(other.mDistanceMeters)
            , mUnitType(other.mUnitType) {}

        const Distance& ConvertTo(EUnitType targetUnitType);
        MCORE_INLINE Distance ConvertedTo(EUnitType targetUnitType) const               { Distance result(*this); result.ConvertTo(targetUnitType); return result; }

        static double GetConversionFactorToMeters(EUnitType unitType);
        static double GetConversionFactorFromMeters(EUnitType unitType);
        static double GetConversionFactor(EUnitType sourceType, EUnitType targetType);
        static double ConvertValue(float value, EUnitType sourceType, EUnitType targetType);

        static const char* UnitTypeToString(EUnitType unitType);
        static bool StringToUnitType(const AZStd::string& str, EUnitType* outUnitType);

        MCORE_INLINE double GetDistance() const                                         { return mDistance; }
        MCORE_INLINE EUnitType GetUnitType() const                                      { return mUnitType; }

        MCORE_INLINE void Set(double dist, EUnitType unitType)                          { mDistance = dist; mUnitType = unitType; UpdateDistanceMeters(); }
        MCORE_INLINE void SetDistance(double dist)                                      { mDistance = dist; UpdateDistanceMeters(); }
        MCORE_INLINE void SetUnitType(EUnitType unitType)                               { mUnitType = unitType; UpdateDistanceMeters(); }

        MCORE_INLINE double CalcDistanceInUnitType(EUnitType targetUnitType) const      { return mDistanceMeters * GetConversionFactorFromMeters(targetUnitType); }
        MCORE_INLINE double CalcNumMillimeters() const                                  { return mDistanceMeters * 1000.0; }
        MCORE_INLINE double CalcNumCentimeters() const                                  { return mDistanceMeters * 100.0; }
        MCORE_INLINE double CalcNumDecimeters() const                                   { return mDistanceMeters * 10.0; }
        MCORE_INLINE double CalcNumMeters() const                                       { return mDistanceMeters; }
        MCORE_INLINE double CalcNumKilometers() const                                   { return mDistanceMeters * 0.001; }
        MCORE_INLINE double CalcNumInches() const                                       { return mDistanceMeters * 39.370078740157; }
        MCORE_INLINE double CalcNumFeet() const                                         { return mDistanceMeters * 3.2808398950131; }
        MCORE_INLINE double CalcNumYards() const                                        { return mDistanceMeters * 1.0936132983377; }
        MCORE_INLINE double CalcNumMiles() const                                        { return mDistanceMeters * 0.00062137119223733; }

        MCORE_INLINE Distance operator - () const                                       { return Distance(-mDistance, mUnitType); }
        MCORE_INLINE const Distance& operator = (const Distance& other)                 { mDistance = other.mDistance; mDistanceMeters = other.mDistanceMeters; mUnitType = other.mUnitType; return *this; }

        MCORE_INLINE const Distance& operator *= (double f)                             { mDistance *= f; UpdateDistanceMeters(); return *this; }
        MCORE_INLINE const Distance& operator /= (double f)                             { mDistance /= f; UpdateDistanceMeters(); return *this; }
        MCORE_INLINE const Distance& operator += (double f)                             { mDistance += f; UpdateDistanceMeters(); return *this; }
        MCORE_INLINE const Distance& operator -= (double f)                             { mDistance -= f; UpdateDistanceMeters(); return *this; }

        MCORE_INLINE const Distance& operator *= (float f)                              { mDistance *= f; UpdateDistanceMeters(); return *this; }
        MCORE_INLINE const Distance& operator /= (float f)                              { mDistance /= f; UpdateDistanceMeters(); return *this; }
        MCORE_INLINE const Distance& operator += (float f)                              { mDistance += f; UpdateDistanceMeters(); return *this; }
        MCORE_INLINE const Distance& operator -= (float f)                              { mDistance -= f; UpdateDistanceMeters(); return *this; }

        MCORE_INLINE const Distance& operator *= (const Distance& other)                { mDistance *= other.ConvertedTo(mUnitType).GetDistance(); UpdateDistanceMeters(); return *this; }
        MCORE_INLINE const Distance& operator /= (const Distance& other)                { mDistance /= other.ConvertedTo(mUnitType).GetDistance(); UpdateDistanceMeters(); return *this; }
        MCORE_INLINE const Distance& operator += (const Distance& other)                { mDistance += other.ConvertedTo(mUnitType).GetDistance(); UpdateDistanceMeters(); return *this; }
        MCORE_INLINE const Distance& operator -= (const Distance& other)                { mDistance -= other.ConvertedTo(mUnitType).GetDistance(); UpdateDistanceMeters(); return *this; }

    private:
        double      mDistance;              /**< The actual distance in the current unit type. */
        double      mDistanceMeters;        /**< The distance in meters. */
        EUnitType   mUnitType;              /**< The actual unit type. */

        void UpdateDistanceMeters();
    };


    // global operators
    MCORE_INLINE Distance operator * (const Distance& dist, double f)                       { return Distance(dist.GetDistance() * f, dist.GetUnitType()); }
    MCORE_INLINE Distance operator / (const Distance& dist, double f)                       { return Distance(dist.GetDistance() / f, dist.GetUnitType()); }
    MCORE_INLINE Distance operator + (const Distance& dist, double f)                       { return Distance(dist.GetDistance() + f, dist.GetUnitType()); }
    MCORE_INLINE Distance operator - (const Distance& dist, double f)                       { return Distance(dist.GetDistance() - f, dist.GetUnitType()); }
    MCORE_INLINE Distance operator * (double f, const Distance& dist)                       { return Distance(dist.GetDistance() * f, dist.GetUnitType()); }
    MCORE_INLINE Distance operator / (double f, const Distance& dist)                       { return Distance(dist.GetDistance() / f, dist.GetUnitType()); }
    MCORE_INLINE Distance operator + (double f, const Distance& dist)                       { return Distance(dist.GetDistance() + f, dist.GetUnitType()); }
    MCORE_INLINE Distance operator - (double f, const Distance& dist)                       { return Distance(dist.GetDistance() - f, dist.GetUnitType()); }

    MCORE_INLINE Distance operator * (const Distance& dist, float f)                        { return Distance(dist.GetDistance() * f, dist.GetUnitType()); }
    MCORE_INLINE Distance operator / (const Distance& dist, float f)                        { return Distance(dist.GetDistance() / f, dist.GetUnitType()); }
    MCORE_INLINE Distance operator + (const Distance& dist, float f)                        { return Distance(dist.GetDistance() + f, dist.GetUnitType()); }
    MCORE_INLINE Distance operator - (const Distance& dist, float f)                        { return Distance(dist.GetDistance() - f, dist.GetUnitType()); }
    MCORE_INLINE Distance operator * (float f, const Distance& dist)                        { return Distance(dist.GetDistance() * f, dist.GetUnitType()); }
    MCORE_INLINE Distance operator / (float f, const Distance& dist)                        { return Distance(dist.GetDistance() / f, dist.GetUnitType()); }
    MCORE_INLINE Distance operator + (float f, const Distance& dist)                        { return Distance(dist.GetDistance() + f, dist.GetUnitType()); }
    MCORE_INLINE Distance operator - (float f, const Distance& dist)                        { return Distance(dist.GetDistance() - f, dist.GetUnitType()); }

    MCORE_INLINE Distance operator * (const Distance& distA, const Distance& distB)         { return Distance(distA.GetDistance() * distB.ConvertedTo(distA.GetUnitType()).GetDistance(), distA.GetUnitType()); }
    MCORE_INLINE Distance operator / (const Distance& distA, const Distance& distB)         { return Distance(distA.GetDistance() / distB.ConvertedTo(distA.GetUnitType()).GetDistance(), distA.GetUnitType()); }
    MCORE_INLINE Distance operator + (const Distance& distA, const Distance& distB)         { return Distance(distA.GetDistance() + distB.ConvertedTo(distA.GetUnitType()).GetDistance(), distA.GetUnitType()); }
    MCORE_INLINE Distance operator - (const Distance& distA, const Distance& distB)         { return Distance(distA.GetDistance() - distB.ConvertedTo(distA.GetUnitType()).GetDistance(), distA.GetUnitType()); }
} // namespace MCore
