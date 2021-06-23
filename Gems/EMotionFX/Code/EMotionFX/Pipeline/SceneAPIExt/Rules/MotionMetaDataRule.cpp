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

#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <SceneAPI/SceneCore/DataTypes/Groups/IGroup.h>
#include <SceneAPI/SceneCore/Events/ManifestMetaInfoBus.h>
#include <SceneAPIExt/Rules/MotionMetaDataRule.h>

namespace EMotionFX
{
    namespace Pipeline
    {
        namespace Rule
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
        } // Rule
    } // Pipeline
} // EMotionFX
