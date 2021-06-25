/*
 * Copyright (c) Contributors to the Open 3D Engine Project
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

    MotionMetaDataRule::MotionMetaDataRule()
        : ExternalToolRule<MotionMetaData>()
    {
    }

    MotionMetaDataRule::MotionMetaDataRule(const MotionMetaData& data)
        : MotionMetaDataRule()
    {
        m_data = data;
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
