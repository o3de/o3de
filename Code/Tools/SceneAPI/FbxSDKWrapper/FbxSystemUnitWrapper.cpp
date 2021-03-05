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

#include <AzCore/Casting/numeric_cast.h>
#include <AzCore/Debug/Trace.h>
#include <SceneAPI/SceneCore/Utilities/Reporting.h>
#include <SceneAPI/FbxSDKWrapper/FbxSystemUnitWrapper.h>

namespace AZ
{
    namespace FbxSDKWrapper
    {
        FbxSystemUnitWrapper::FbxSystemUnitWrapper(const FbxSystemUnit& fbxSystemUnit)
            : m_fbxSystemUnit(fbxSystemUnit)
        {
        }

        FbxSystemUnitWrapper::Unit FbxSystemUnitWrapper::GetUnit() const
        {
            if (m_fbxSystemUnit == FbxSystemUnit::mm)
            {
                return mm;
            }
            if (m_fbxSystemUnit == FbxSystemUnit::dm)
            {
                return dm;
            }
            if (m_fbxSystemUnit == FbxSystemUnit::cm)
            {
                return cm;
            }
            if (m_fbxSystemUnit == FbxSystemUnit::m)
            {
                return m;
            }
            if (m_fbxSystemUnit == FbxSystemUnit::km)
            {
                return km;
            }
            if (m_fbxSystemUnit == FbxSystemUnit::Inch)
            {
                return inch;
            }
            if (m_fbxSystemUnit == FbxSystemUnit::Foot)
            {
                return foot;
            }
            if (m_fbxSystemUnit == FbxSystemUnit::Mile)
            {
                return mile;
            }
            if (m_fbxSystemUnit == FbxSystemUnit::Yard)
            {
                return yard;
            }
            AZ_TracePrintf(SceneAPI::Utilities::WarningWindow, "Unrecognized current unit type");
            return unknown;
        }

        float FbxSystemUnitWrapper::GetConversionFactorTo(Unit to)
        {
            FbxSystemUnit targetUnit;
            switch (to)
            {
            case mm:
                targetUnit = FbxSystemUnit::mm;
                break;
            case dm:
                targetUnit = FbxSystemUnit::dm;
                break;
            case cm:
                targetUnit = FbxSystemUnit::cm;
                break;
            case m:
                targetUnit = FbxSystemUnit::m;
                break;
            case km:
                targetUnit = FbxSystemUnit::km;
                break;
            case inch:
                targetUnit = FbxSystemUnit::Inch;
                break;
            case foot:
                targetUnit = FbxSystemUnit::Foot;
                break;
            case mile:
                targetUnit = FbxSystemUnit::Mile;
                break;
            case yard:
                targetUnit = FbxSystemUnit::Yard;
                break;
            default:
                AZ_TracePrintf(SceneAPI::Utilities::WarningWindow, "Unrecognized unit conversion target type");
                targetUnit = FbxSystemUnit::m;
            }
            return aznumeric_caster(m_fbxSystemUnit.GetConversionFactorTo(targetUnit));
        }
    } // namespace FbxSDKWrapper
} // namespace AZ
