/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <SceneAPI/SceneCore/DataTypes/Groups/IGroup.h>
#include <SceneAPI/SceneCore/Events/ManifestMetaInfoBus.h>
#include <SceneAPIExt/Rules/MotionMetaDataRule.h>

namespace EMotionFX::Pipeline::Rule
{
    void MotionMetaData::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (!serializeContext)
        {
            return;
        }

        serializeContext->Class<MotionMetaData>()
            ->Version(1)
            ->Field("motionEventTable", &MotionMetaData::m_motionEventTable)
            ->Field("motionExtractionFlags", &MotionMetaData::m_motionExtractionFlags)
            ;
    }

    MotionMetaData::MotionMetaData(EMotionFX::EMotionExtractionFlags extractionFlags, EMotionFX::MotionEventTable* eventTable)
        : m_motionExtractionFlags(extractionFlags)
    {
        m_motionEventTable = CloneMotionEventTable(eventTable);
    }

    MotionMetaData::MotionMetaData()
        : m_motionExtractionFlags(static_cast<EMotionFX::EMotionExtractionFlags>(0))
    {
    }

    AZStd::unique_ptr<EMotionFX::MotionEventTable> MotionMetaData::GetClonedEventTable(EMotionFX::Motion* targetMotion) const
    {
        AZStd::unique_ptr<EMotionFX::MotionEventTable> clonedEventTable = CloneMotionEventTable(m_motionEventTable.get());
        clonedEventTable->InitAfterLoading(targetMotion);
        return clonedEventTable;
    }

    AZStd::unique_ptr<EMotionFX::MotionEventTable> MotionMetaData::CloneMotionEventTable(EMotionFX::MotionEventTable* sourceEventTable)
    {
        AZ::SerializeContext* serializeContext = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(serializeContext, &AZ::ComponentApplicationBus::Events::GetSerializeContext);
        if (!serializeContext)
        {
            AZ_Error("EMotionFX", false, "Cannot clone motion event table for motion meta data. Can't get serialize context from component application.");
            return {};
        }

        AZStd::unique_ptr<EMotionFX::MotionEventTable> clonedEventTable(serializeContext->CloneObject<EMotionFX::MotionEventTable>(sourceEventTable));
        return clonedEventTable;
    }

    MotionMetaDataRule::MotionMetaDataRule()
        : ExternalToolRule<AZStd::shared_ptr<MotionMetaData>>()
    {
    }

    MotionMetaDataRule::MotionMetaDataRule(const AZStd::shared_ptr<MotionMetaData>& data)
        : m_data(data)
    {
    }

    void MotionMetaDataRule::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (serializeContext)
        {
            serializeContext->Class<MotionMetaDataRule>()
                ->Version(1)
                ->Field("data", &MotionMetaDataRule::m_data)
                ;
        }
    }
} // EMotionFX::Pipeline::Rule
