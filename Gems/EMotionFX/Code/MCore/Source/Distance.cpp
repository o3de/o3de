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

// include required headers
#include "Distance.h"
#include "StringConversions.h"


namespace MCore
{
    // convert it into another unit type
    const MCore::Distance& Distance::ConvertTo(EUnitType targetUnitType)
    {
        mDistance = mDistanceMeters * GetConversionFactorFromMeters(targetUnitType);
        mUnitType = targetUnitType;
        return *this;
    }


    // from a given unit type into meters
    double Distance::GetConversionFactorToMeters(EUnitType unitType)
    {
        switch (unitType)
        {
        case UNITTYPE_MILLIMETERS:
            return 0.001;
        case UNITTYPE_CENTIMETERS:
            return 0.01;
        case UNITTYPE_DECIMETERS:
            return 0.1;
        case UNITTYPE_METERS:
            return 1.0;
        case UNITTYPE_KILOMETERS:
            return 1000.0;
        case UNITTYPE_INCHES:
            return 0.0254;
        case UNITTYPE_FEET:
            return 0.3048;
        case UNITTYPE_YARDS:
            return 0.9144;
        case UNITTYPE_MILES:
            return 1609.344;
        default:
            MCORE_ASSERT(false);
            return 1.0;
        }
    }


    // from meters into a given unit type
    double Distance::GetConversionFactorFromMeters(EUnitType unitType)
    {
        switch (unitType)
        {
        case UNITTYPE_MILLIMETERS:
            return 1000.0;
        case UNITTYPE_CENTIMETERS:
            return 100.0;
        case UNITTYPE_DECIMETERS:
            return 10.0;
        case UNITTYPE_METERS:
            return 1.0;
        case UNITTYPE_KILOMETERS:
            return 0.001;
        case UNITTYPE_INCHES:
            return 39.370078740157;
        case UNITTYPE_FEET:
            return 3.2808398950131;
        case UNITTYPE_YARDS:
            return 1.0936132983377;
        case UNITTYPE_MILES:
            return 0.00062137119223733;
        default:
            MCORE_ASSERT(false);
            return 1.0;
        }
    }


    // convert the type into a string
    const char* Distance::UnitTypeToString(EUnitType unitType)
    {
        switch (unitType)
        {
        case UNITTYPE_MILLIMETERS:
            return "millimeters";
        case UNITTYPE_CENTIMETERS:
            return "centimeters";
        case UNITTYPE_DECIMETERS:
            return "decimeters";
        case UNITTYPE_METERS:
            return "meters";
        case UNITTYPE_KILOMETERS:
            return "kilometers";
        case UNITTYPE_INCHES:
            return "inches";
        case UNITTYPE_FEET:
            return "feet";
        case UNITTYPE_YARDS:
            return "yards";
        case UNITTYPE_MILES:
            return "miles";
        default:
            return "unknown";
        }
    }


    // convert a string into a unit type
    bool Distance::StringToUnitType(const AZStd::string& str, EUnitType* outUnitType)
    {
        if (AzFramework::StringFunc::Equal(str.c_str(), "millimeters", false /* no case */) || AzFramework::StringFunc::Equal(str.c_str(), "millimeter", false /* no case */)   || AzFramework::StringFunc::Equal(str.c_str(), "mm", false /* no case */)  )
        {
            *outUnitType = UNITTYPE_MILLIMETERS;
            return true;
        }
        if (AzFramework::StringFunc::Equal(str.c_str(), "centimeters", false /* no case */) || AzFramework::StringFunc::Equal(str.c_str(), "centimeter", false /* no case */)   || AzFramework::StringFunc::Equal(str.c_str(), "cm", false /* no case */)  )
        {
            *outUnitType = UNITTYPE_CENTIMETERS;
            return true;
        }
        if (AzFramework::StringFunc::Equal(str.c_str(), "meters", false /* no case */)      || AzFramework::StringFunc::Equal(str.c_str(), "meter", false /* no case */)        || AzFramework::StringFunc::Equal(str.c_str(), "m", false /* no case */)   )
        {
            *outUnitType = UNITTYPE_METERS;
            return true;
        }
        if (AzFramework::StringFunc::Equal(str.c_str(), "decimeters", false /* no case */)  || AzFramework::StringFunc::Equal(str.c_str(), "decimeter", false /* no case */)    || AzFramework::StringFunc::Equal(str.c_str(), "dm", false /* no case */)  )
        {
            *outUnitType = UNITTYPE_DECIMETERS;
            return true;
        }
        if (AzFramework::StringFunc::Equal(str.c_str(), "kilometers", false /* no case */)  || AzFramework::StringFunc::Equal(str.c_str(), "kilometer", false /* no case */)    || AzFramework::StringFunc::Equal(str.c_str(), "km", false /* no case */)  )
        {
            *outUnitType = UNITTYPE_KILOMETERS;
            return true;
        }
        if (AzFramework::StringFunc::Equal(str.c_str(), "inches", false /* no case */)      || AzFramework::StringFunc::Equal(str.c_str(), "inch", false /* no case */)         || AzFramework::StringFunc::Equal(str.c_str(), "in", false /* no case */)  )
        {
            *outUnitType = UNITTYPE_INCHES;
            return true;
        }
        if (AzFramework::StringFunc::Equal(str.c_str(), "feet", false /* no case */)        || AzFramework::StringFunc::Equal(str.c_str(), "foot", false /* no case */)         || AzFramework::StringFunc::Equal(str.c_str(), "ft", false /* no case */)  )
        {
            *outUnitType = UNITTYPE_FEET;
            return true;
        }
        if (AzFramework::StringFunc::Equal(str.c_str(), "yards", false /* no case */)       || AzFramework::StringFunc::Equal(str.c_str(), "yard", false /* no case */)         || AzFramework::StringFunc::Equal(str.c_str(), "yd", false /* no case */)  )
        {
            *outUnitType = UNITTYPE_YARDS;
            return true;
        }
        if (AzFramework::StringFunc::Equal(str.c_str(), "miles", false /* no case */)       || AzFramework::StringFunc::Equal(str.c_str(), "mile", false /* no case */)         || AzFramework::StringFunc::Equal(str.c_str(), "mi", false /* no case */)  )
        {
            *outUnitType = UNITTYPE_MILES;
            return true;
        }
        return false;
    }


    // update the distance in meters
    void Distance::UpdateDistanceMeters()
    {
        mDistanceMeters = mDistance * GetConversionFactorToMeters(mUnitType);
    }


    // get the conversion factor between two unit types
    double Distance::GetConversionFactor(EUnitType sourceType, EUnitType targetType)
    {
        Distance dist(1.0, sourceType);
        dist.ConvertTo(targetType);
        return dist.GetDistance();
    }


    // convert a singnle value quickly
    double Distance::ConvertValue(float value, EUnitType sourceType, EUnitType targetType)
    {
        return Distance(value, sourceType).ConvertTo(targetType).GetDistance();
    }
}   // namespace MCore
