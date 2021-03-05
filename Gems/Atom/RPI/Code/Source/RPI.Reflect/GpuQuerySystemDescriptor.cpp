/**
 * All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
 * its licensors.
 *
 * For complete copyright and license terms please see the LICENSE at the root of this
 * distribution (the "License"). All use of this software is governed by the License,
 * or, if provided, by the license below or the license accompanying this file. Do not
 * remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 */

#include <Atom/RPI.Reflect/GpuQuerySystemDescriptor.h>

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>

namespace AZ
{
    namespace RPI
    {
        void GpuQuerySystemDescriptor::Reflect(AZ::ReflectContext* context)
        {
            if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<GpuQuerySystemDescriptor>()
                    ->Version(0)
                    ->Field("OcclusionQueryCount", &GpuQuerySystemDescriptor::m_occlusionQueryCount)
                    ->Field("StatisticsQueryCount", &GpuQuerySystemDescriptor::m_statisticsQueryCount)
                    ->Field("TimestampQueryCount", &GpuQuerySystemDescriptor::m_timestampQueryCount)
                    ->Field("StatisticsQueryFlags", &GpuQuerySystemDescriptor::m_statisticsQueryFlags)
                    ;

                if (AZ::EditContext* ec = serializeContext->GetEditContext())
                {
                    ec->Class<GpuQuerySystemDescriptor>("Gpu Query System Settings", "Settings for the Gpu Query System")
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("System", 0xc94d118b))
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->DataElement(AZ::Edit::UIHandlers::Default, &GpuQuerySystemDescriptor::m_occlusionQueryCount, "Occlusion GPU Query Count", "The amount of available queries for Occlusion")
                        ->DataElement(AZ::Edit::UIHandlers::Default, &GpuQuerySystemDescriptor::m_statisticsQueryCount, "Statistics GPU Query Count", "The amount of available queries for Pipeline Statistics")
                        ->DataElement(AZ::Edit::UIHandlers::Default, &GpuQuerySystemDescriptor::m_timestampQueryCount, "Timestamp GPU Query Count", "The amount of available queries for Timestamps")
                        ->DataElement(AZ::Edit::UIHandlers::Default, &GpuQuerySystemDescriptor::m_statisticsQueryFlags, "StatisticsQueryFlags", "Flags that determine which values to readback from the Pipeline Statistics queries")
                        ;
                }
            }
        }
    } // namespace RPI
} // namespace AZ
