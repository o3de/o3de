/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzFramework/Physics/Vehicle.h>

namespace Physics
{
    void VehicleConfiguration::Reflect(AZ::ReflectContext* context)
    {
        DriveModelConfiguration::Reflect(context);
        EngineDriveModelConfiguration::Reflect(context);
        if (auto* serializeContext = azdynamic_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<AntiRollBarConfiguration>()
                ->Version(1)
                ->Field("LeftWheel", &AntiRollBarConfiguration::m_leftWheel)
                ->Field("RightWheel", &AntiRollBarConfiguration::m_rightWheel)
                ->Field("Stiffness", &AntiRollBarConfiguration::m_stiffness)
                ;

            serializeContext->Class<VehicleConfiguration, AzPhysics::RigidBodyConfiguration>()
                ->Version(1)
                ->Field("UpDirection", &VehicleConfiguration::m_upDirection)
                ->Field("ForwardDirection", &VehicleConfiguration::m_forwardDirection)
                ->Field("WheelConfigurations", &VehicleConfiguration::m_wheels)
                ->Field("AntiRollBars", &VehicleConfiguration::m_antiRollBars)
                ->Field("DriveModel", &VehicleConfiguration::m_driveModel)
                ;
        }
    }

    
    void DriveModelConfiguration::Reflect(AZ::ReflectContext* context)
    {
        if (auto* serializeContext = azdynamic_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<DriveModelConfiguration>()
                ->Version(1)
                ;
        }
    }

    void EngineDriveModelConfiguration::Reflect(AZ::ReflectContext* context)
    {
        if (auto* serializeContext = azdynamic_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<EngineDriveModelConfiguration, DriveModelConfiguration>()
                ->Version(1)
                ->Field("MomentOfInertia", &EngineDriveModelConfiguration::m_moi)
                ->Field("MinRotationSpeed", &EngineDriveModelConfiguration::m_minRotationSpeed)
                ->Field("MaxRotationSpeed", &EngineDriveModelConfiguration::m_maxRotationSpeed)
                ->Field("PeakTorque", &EngineDriveModelConfiguration::m_peakTorque)
                ->Field("TorqueCurve", &EngineDriveModelConfiguration::m_torqueCurve)
                ->Field("GearRatios", &EngineDriveModelConfiguration::m_gearRatios)
                ->Field("NeutralGearIndex", &EngineDriveModelConfiguration::m_neutralGearIndex)
                ->Field("SwitchTime", &EngineDriveModelConfiguration::m_switchTime)
                ;
        }
    }
}
