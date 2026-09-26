/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzFramework/Physics/Wheel.h>

namespace Physics
{
    void WheelConfiguration::Reflect(AZ::ReflectContext* context)
    {
        if (auto* serializeContext = azdynamic_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<WheelConfiguration, AzPhysics::SimulatedBodyConfiguration>()
                ->Version(1)
                ->Field("Radius", &WheelConfiguration::m_radius)
                ->Field("Width", &WheelConfiguration::m_width)
                ->Field("Mass", &WheelConfiguration::m_mass)
                ->Field("MOI", &WheelConfiguration::m_moi)
                ->Field("DampingRate", &WheelConfiguration::m_dampingRate)
                ->Field("SuspensionDirection", &WheelConfiguration::m_suspensionDirection)
                ->Field("SuspensionMaxCompressionLength", &WheelConfiguration::m_suspensionMaxCompressionLength)
                ->Field("SuspensionMaxDroopLength", &WheelConfiguration::m_suspensionMaxDroopLength)
                ->Field("SuspensionStiffness", &WheelConfiguration::m_suspensionStiffness)
                ->Field("SuspensionDamping", &WheelConfiguration::m_suspensionDamping)
                ;
        }
    }
}
