/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzFramework/Physics/Configuration/SimulatedBodyConfiguration.h>

#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/Serialization/SerializeContext.h>

namespace AzPhysics
{
    namespace Internal
    {
        bool DeprecateWorldBodyConfiguration(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement)
        {
            // WorldBodyConfiguration on serialized the name so capture that.
            AZStd::string name = "";
            classElement.GetChildData(AZ::Crc32("name"), name);
            // convert to the new class
            classElement.Convert<SimulatedBodyConfiguration>(context);
            // add the captured name
            classElement.AddElementWithData(context, "name", name);
            return true;
        }

        bool SimulatedBodyVersionConverter(
            [[maybe_unused]] AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement)
        {
            if (classElement.GetVersion() <= 1)
            {
                classElement.RemoveElementByName(AZ_CRC_CE("scale"));
            }

            return true;
        }
    } // namespace Internal

    AZ_CLASS_ALLOCATOR_IMPL(SimulatedBodyConfiguration, AZ::SystemAllocator);

    void SimulatedBodyConfiguration::Reflect(AZ::ReflectContext* context)
    {
        if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->ClassDeprecate(
                "WorldBodyConfiguration", AZ::Uuid("{6EEB377C-DC60-4E10-AF12-9626C0763B2D}"), &Internal::DeprecateWorldBodyConfiguration);
            serializeContext->Class<SimulatedBodyConfiguration>()
                ->Version(2, &Internal::SimulatedBodyVersionConverter)
                ->Field("name", &SimulatedBodyConfiguration::m_debugName)
                ->Field("position", &SimulatedBodyConfiguration::m_position)
                ->Field("orientation", &SimulatedBodyConfiguration::m_orientation)
                ->Field("entityId", &SimulatedBodyConfiguration::m_entityId)
                ->Field("startSimulationEnabled", &SimulatedBodyConfiguration::m_startSimulationEnabled)
                ;
        }
    }
}
