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
            //convert to the new class
            classElement.Convert<SimulatedBodyConfiguration>(context);
            //add the captured name
            classElement.AddElementWithData(context, "name", name);
            return true;
        }
    }

    AZ_CLASS_ALLOCATOR_IMPL(SimulatedBodyConfiguration, AZ::SystemAllocator, 0);

    void SimulatedBodyConfiguration::Reflect(AZ::ReflectContext* context)
    {
        if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->ClassDeprecate("WorldBodyConfiguration", "{6EEB377C-DC60-4E10-AF12-9626C0763B2D}", &Internal::DeprecateWorldBodyConfiguration);
            serializeContext->Class<SimulatedBodyConfiguration>()
                ->Version(1)
                ->Field("name", &SimulatedBodyConfiguration::m_debugName)
                ->Field("position", &SimulatedBodyConfiguration::m_position)
                ->Field("orientation", &SimulatedBodyConfiguration::m_orientation)
                ->Field("scale", &SimulatedBodyConfiguration::m_scale)
                ->Field("entityId", &SimulatedBodyConfiguration::m_entityId)
                ->Field("startSimulationEnabled", &SimulatedBodyConfiguration::m_startSimulationEnabled)
                ;
        }
    }
}
