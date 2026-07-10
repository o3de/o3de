/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RPI.Reflect/GpuQuerySystemDescriptor.h>

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzFramework/Translation/TranslationDef.h>

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
                    ec->Class<GpuQuerySystemDescriptor>(QT_TRANSLATE_NOOP("Atom::RPI", "Gpu Query System Settings"), QT_TRANSLATE_NOOP("Atom::RPI", "Settings for the Gpu Query System"))
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->DataElement(AZ::Edit::UIHandlers::Default, &GpuQuerySystemDescriptor::m_occlusionQueryCount, QT_TRANSLATE_NOOP("Atom::RPI", "Occlusion GPU Query Count"), QT_TRANSLATE_NOOP("Atom::RPI", "The amount of available queries for Occlusion"))
                        ->DataElement(AZ::Edit::UIHandlers::Default, &GpuQuerySystemDescriptor::m_statisticsQueryCount, QT_TRANSLATE_NOOP("Atom::RPI", "Statistics GPU Query Count"), QT_TRANSLATE_NOOP("Atom::RPI", "The amount of available queries for Pipeline Statistics"))
                        ->DataElement(AZ::Edit::UIHandlers::Default, &GpuQuerySystemDescriptor::m_timestampQueryCount, QT_TRANSLATE_NOOP("Atom::RPI", "Timestamp GPU Query Count"), QT_TRANSLATE_NOOP("Atom::RPI", "The amount of available queries for Timestamps"))
                        ->DataElement(AZ::Edit::UIHandlers::Default, &GpuQuerySystemDescriptor::m_statisticsQueryFlags, QT_TRANSLATE_NOOP("Atom::RPI", "StatisticsQueryFlags"), QT_TRANSLATE_NOOP("Atom::RPI", "Flags that determine which values to readback from the Pipeline Statistics queries"))
                        ;
                }
            }
        }
    } // namespace RPI
} // namespace AZ
