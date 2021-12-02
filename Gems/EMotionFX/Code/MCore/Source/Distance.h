/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
            : m_distance(0.0)
            , m_distanceMeters(0.0)
            , m_unitType(UNITTYPE_METERS)      { }
        MCORE_INLINE Distance(double units, EUnitType unitType)
            : m_distance(units)
            , m_distanceMeters(0.0)
            , m_unitType(unitType)           { UpdateDistanceMeters(); }
        MCORE_INLINE Distance(float units, EUnitType unitType)
            : m_distance(units)
            , m_distanceMeters(0.0)
            , m_unitType(unitType)           { UpdateDistanceMeters(); }
        MCORE_INLINE Distance(const Distance& other)
            : m_distance(other.m_distance)
            , m_distanceMeters(other.m_distanceMeters)
            , m_unitType(other.m_unitType) {}

        const Distance& ConvertTo(EUnitType targetUnitType);
        MCORE_INLINE Distance ConvertedTo(EUnitType targetUnitType) const               { Distance result(*this); result.ConvertTo(targetUnitType); return result; }

        static double GetConversionFactorToMeters(EUnitType unitType);
        static double GetConversionFactorFromMeters(EUnitType unitType);
        static double GetConversionFactor(EUnitType sourceType, EUnitType targetType);
        static double ConvertValue(float value, EUnitType sourceType, EUnitType targetType);

        static const char* UnitTypeToString(EUnitType unitType);
        static bool StringToUnitType(const AZStd::string& str, EUnitType* outUnitType);

        MCORE_INLINE double GetDistance() const                                         { return m_distance; }
        MCORE_INLINE EUnitType GetUnitType() const                                      { return m_unitType; }

        MCORE_INLINE void Set(double dist, EUnitType unitType)                          { m_distance = dist; m_unitType = unitType; UpdateDistanceMeters(); }
        MCORE_INLINE void SetDistance(double dist)                                      { m_distance = dist; UpdateDistanceMeters(); }
        MCORE_INLINE void SetUnitType(EUnitType unitType)                               { m_unitType = unitType; UpdateDistanceMeters(); }

        MCORE_INLINE double CalcDistanceInUnitType(EUnitType targetUnitType) const      { return m_distanceMeters * GetConversionFactorFromMeters(targetUnitType); }
        MCORE_INLINE double CalcNumMillimeters() const                                  { return m_distanceMeters * 1000.0; }
        MCORE_INLINE double CalcNumCentimeters() const                                  { return m_distanceMeters * 100.0; }
        MCORE_INLINE double CalcNumDecimeters() const                                   { return m_distanceMeters * 10.0; }
        MCORE_INLINE double CalcNumMeters() const                                       { return m_distanceMeters; }
        MCORE_INLINE double CalcNumKilometers() const                                   { return m_distanceMeters * 0.001; }
        MCORE_INLINE double CalcNumInches() const                                       { return m_distanceMeters * 39.370078740157; }
        MCORE_INLINE double CalcNumFeet() const                                         { return m_distanceMeters * 3.2808398950131; }
        MCORE_INLINE double CalcNumYards() const                                        { return m_distanceMeters * 1.0936132983377; }
        MCORE_INLINE double CalcNumMiles() const                                        { return m_distanceMeters * 0.00062137119223733; }

        MCORE_INLINE Distance operator - () const                                       { return Distance(-m_distance, m_unitType); }
        MCORE_INLINE const Distance& operator = (const Distance& other)                 { m_distance = other.m_distance; m_distanceMeters = other.m_distanceMeters; m_unitType = other.m_unitType; return *this; }

        MCORE_INLINE const Distance& operator *= (double f)                             { m_distance *= f; UpdateDistanceMeters(); return *this; }
        MCORE_INLINE const Distance& operator /= (double f)                             { m_distance /= f; UpdateDistanceMeters(); return *this; }
        MCORE_INLINE const Distance& operator += (double f)                             { m_distance += f; UpdateDistanceMeters(); return *this; }
        MCORE_INLINE const Distance& operator -= (double f)                             { m_distance -= f; UpdateDistanceMeters(); return *this; }

        MCORE_INLINE const Distance& operator *= (float f)                              { m_distance *= f; UpdateDistanceMeters(); return *this; }
        MCORE_INLINE const Distance& operator /= (float f)                              { m_distance /= f; UpdateDistanceMeters(); return *this; }
        MCORE_INLINE const Distance& operator += (float f)                              { m_distance += f; UpdateDistanceMeters(); return *this; }
        MCORE_INLINE const Distance& operator -= (float f)                              { m_distance -= f; UpdateDistanceMeters(); return *this; }

        MCORE_INLINE const Distance& operator *= (const Distance& other)                { m_distance *= other.ConvertedTo(m_unitType).GetDistance(); UpdateDistanceMeters(); return *this; }
        MCORE_INLINE const Distance& operator /= (const Distance& other)                { m_distance /= other.ConvertedTo(m_unitType).GetDistance(); UpdateDistanceMeters(); return *this; }
        MCORE_INLINE const Distance& operator += (const Distance& other)                { m_distance += other.ConvertedTo(m_unitType).GetDistance(); UpdateDistanceMeters(); return *this; }
        MCORE_INLINE const Distance& operator -= (const Distance& other)                { m_distance -= other.ConvertedTo(m_unitType).GetDistance(); UpdateDistanceMeters(); return *this; }

    private:
        double      m_distance;              /**< The actual distance in the current unit type. */
        double      m_distanceMeters;        /**< The distance in meters. */
        EUnitType   m_unitType;              /**< The actual unit type. */

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
